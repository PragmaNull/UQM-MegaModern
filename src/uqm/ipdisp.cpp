//Copyright Paul Reiche, Fred Ford. 1992-2002

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "ipdisp.h"
#include "build.h"
#include "collide.h"
#include "controls.h"
#include "globdata.h"
#include "init.h"
#include "races.h"
#include "process.h"
#include "grpinfo.h"
#include "encount.h"
#include "planets/solarsys.h"
// for EncounterGroup, EncounterRace
#include "libs/mathlib.h"
#include "nameref.h"
#include "ships/slylandr/resinst.h"
#include "setup.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
bool legacySave;
uint8_t GTFO = 0;
extern FrameRef SpaceJunkFrame;

static inline int16_t RaceIPSpeed(RaceId Index)
{
	static int seedStamp = -1;
	const auto& defaultMap {RaceIpSpeed};
	const uint16_t numRaces = uqstl::size(defaultMap);
	static int16_t speedMap[uqstl::size(defaultMap)] = {0};
	int x {};
	if (!uqm::currentSeed().ships && (seedStamp != -1 || speedMap[0] == 0))
	{
		for (x = 0; x < numRaces; x++)
		{
			speedMap[x] = defaultMap[x];
		}
		seedStamp = -1;
		if (speedMap[0] == 0)
		{
			speedMap[0] = 1;
		}
	}

	if (uqm::currentSeed().ships && (seedStamp != uqm::currentSeed().value || speedMap[0] == 0))
	{
		FleetInfoHandle hFleet {};
		for (x = 0; x < numRaces; x++)
		{
			hFleet = GetSeededFleetFromIndex(x);
			if (hFleet)
			{
				speedMap[x] = defaultMap[GetIndexFromStarShip(
					&uqm::gameState().game().avail_race_q, hFleet)];
			}
			else
			{
				speedMap[x] = defaultMap[x];
			}
		}
		if (speedMap[0] == 0)
		{
			speedMap[0] = 1;
		}
	}
	return speedMap[*Index];
}

void NotifyOthers(uint16_t which_race, uint8_t target_loc)
{
	ShipFragmentHandle hGroup {};
	ShipFragmentHandle hNextGroup {};

	// NOTE: "Others" includes the group causing the notification too.

	for (hGroup = GetHeadLink(&uqm::gameState().game().ip_group_q);
		 hGroup; hGroup = hNextGroup)
	{
		InterplanetaryGroup* GroupPtr {LockIpGroup(&uqm::gameState().game().ip_group_q, hGroup)};
		hNextGroup = _GetSuccLink(GroupPtr);

		if (GroupPtr->race_id != which_race)
		{
			UnlockIpGroup(&uqm::gameState().game().ip_group_q, hGroup);
			continue;
		}

		if (target_loc == IpnlInterceptPlayer)
		{
			GroupPtr->task &= ~GroupTaskFlag::IgnoreFlagship;
			// XXX: orbit_pos is abused here to store the previous
			//   group destination, before the intercept task.
			//   Returned to dest_loc below.
			GroupPtr->orbit_pos = GroupPtr->dest_loc;
			GroupPtr->dest_loc = IpnlInterceptPlayer;
		}
		else if (target_loc == IpnlAllClear)
		{
			GroupPtr->task |= GroupTaskFlag::IgnoreFlagship;

			if (GroupPtr->dest_loc == IpnlInterceptPlayer)
			{ // The group was intercepting, so send it back where it came
				// XXX: orbit_pos was abused to store the previous
				//   group destination, before the intercept task.
				GroupPtr->dest_loc = GroupPtr->orbit_pos;
				GroupPtr->orbit_pos = NormalizeFacing(uqm::random().next());
#ifdef OLD
				GroupPtr->dest_loc = (uint8_t)(((uint16_t)uqm::random().next()
												% pSolarSysState->SunDesc[0].NumPlanets)
											   + 1);
#endif /* OLD */
			}
			// If the group wasn't intercepting, it will just continue
			// going about its business.

			if (!(GroupPtr->task & GroupTaskFlag::ReformGroup))
			{
				if ((GroupPtr->task & ~GroupTaskFlag::IgnoreFlagship) != GroupTask::Explore)
				{
					GroupPtr->group_counter = 0;
				}
				else
				{
					GroupPtr->group_counter = ((uint16_t)uqm::random().next()
											   % MaxRevolutions)
										   << FacingShift;
				}
			}
		}
		else
		{ // Send the group to the location.
			// XXX: There is currently no use of such notify that I know of.
			GroupPtr->task &= GroupTaskFlag::ReformGroup;
			GroupPtr->task |= *GroupTask::Flee | GroupTaskFlag::IgnoreFlagship;
			GroupPtr->dest_loc = 0;
		}

		UnlockIpGroup(&uqm::gameState().game().ip_group_q, hGroup);
	}
}

static int16_t zoomRadiusForLocation(uint8_t location)
{
	if (location == 0)
	{ // In outer system view; use current zoom radius
		return pSolarSysState->SunDesc[0].radius;
	}
	else
	{ // In inner system view; always max zoom
		return MaxZoomRadius;
	}
}

static inline void adjustDeltaVforZoom(int16_t zoom, int16_t* dx, int16_t* dy)
{
	if (zoom == MinZoomRadius)
	{
		*dx >>= 2;
		*dy >>= 2;
	}
	else if (zoom < MaxZoomRadius)
	{
		*dx >>= 1;
		*dy >>= 1;
	}
}

static uint8_t getFlagshipLocation(void)
{
	if (!playerInInnerSystem())
	{
		return 0;
	}
	else
	{
		return 1 + planetIndex(pSolarSysState, pSolarSysState->pOrbitalDesc);
	}
}

// These need to be global in case something else uses them
// after shipseed write to them
FrameRef Xform = NullResource;
FrameRef Ywing = NullResource;

static constexpr auto TrackWait {5};
static constexpr auto OrbitSpeed {60};

static void ip_group_preprocess(Element* ElementPtr)
{
	uint8_t task {};
	uint8_t group_loc {};
	GfxPoint dest_pt;
	int16_t vdx {};
	int16_t vdy {};
	InterplanetaryGroup* GroupPtr {};

	Element* EPtr {ElementPtr};
	EPtr->state_flags &= ~(Disappearing | Nonsolid); // "I'm not quite dead"
	++EPtr->life_span;								 // so that it will 'die' again next time

	*(&GroupPtr) = (InterplanetaryGroup*)(EPtr)->pParent;
	group_loc = GroupPtr->sys_loc; // save old location
	DisplayArray[EPtr->PrimIndex].Object.Point = GroupPtr->loc;

	int16_t radius {zoomRadiusForLocation(group_loc)};
	dest_pt = locationToDisplay(GroupPtr->loc, radius);
	EPtr->current.location.x = DisplayToWorld(dest_pt.x)
							 + (int16_t)(LogSpaceWidth() >> 1)
							 - (LogSpaceWidth() >> (MaxReduction + 1));
	EPtr->current.location.y = DisplayToWorld(dest_pt.y)
							 + (int16_t)(LogSpaceHeight() >> 1)
							 - (LogSpaceHeight() >> (MaxReduction + 1));

	InitIntersectStartPoint(EPtr);

	const uint8_t flagship_loc {getFlagshipLocation()};

	if ((uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY)
		 && CheckAlliance((RaceId)GroupPtr->race_id) == RaceAllegiance::Dead)
		|| (GTFO == 1 && GroupPtr->race_id == RaceId::Ilwrath))
	{
		GTFO = 0;
		NotifyOthers(GroupPtr->race_id, IpnlFlee);
	}

	if (GTFO == 2 && GroupPtr->race_id == RaceId::Ilwrath)
	{
		GTFO = 0;
		NotifyOthers(*RaceId::Ilwrath, IpnlInterceptPlayer);
	}

	task = GroupPtr->task;

	if ((task & GroupTaskFlag::ReformGroup) && --GroupPtr->group_counter == 0)
	{ // Finished reforming the group
		task &= ~GroupTaskFlag::ReformGroup;
		GroupPtr->task = task;
		if ((task & ~GroupTaskFlag::IgnoreFlagship) != GroupTask::Explore)
		{
			GroupPtr->group_counter = 0;
		}
		else
		{
			GroupPtr->group_counter = ((uint16_t)uqm::random().next()
									   % MaxRevolutions)
								   << FacingShift;
		}
	}

	// If fleeing *and* ignoring flagship
	if ((task & ~(GroupTaskFlag::IgnoreFlagship | GroupTaskFlag::ReformGroup)) == GroupTask::Flee
		&& (task & GroupTaskFlag::IgnoreFlagship))
	{ // Make fleeing groups non-collidable
		EPtr->state_flags |= Nonsolid;
	}

	uint8_t target_loc {GroupPtr->dest_loc};
	if (!(task & (GroupTaskFlag::IgnoreFlagship | GroupTaskFlag::ReformGroup)))
	{
		if (target_loc == IpnlInterceptPlayer && task != GroupTask::Flee)
		{
			/* if intercepting flagship */
			target_loc = flagship_loc;
			if (EPtr->thrust_wait > TrackWait)
			{
				EPtr->thrust_wait = 0;
				ZeroVelocityComponents(&EPtr->velocity);
			}
		}
		else if (group_loc == flagship_loc)
		{
			int16_t detect_dist = 1200;

			if (IsExtended() && CheckAlliance((RaceId)GroupPtr->race_id) == RaceAllegiance::Good)
			{
				detect_dist = 0;
			}

			if (group_loc != 0) /* if in planetary views */
			{
				detect_dist *= (MaxZoomRadius / MinZoomRadius);
				if (GroupPtr->race_id == RaceId::UrquanDrone)
				{
					detect_dist <<= 1;
				}
			}
			vdx = uqm::flagship().ipLocation.x - GroupPtr->loc.x;
			vdy = uqm::flagship().ipLocation.y - GroupPtr->loc.y;
			if ((long)vdx * vdx
					+ (long)vdy * vdy
				< (long)detect_dist * detect_dist)
			{
				EPtr->thrust_wait = 0;
				ZeroVelocityComponents(&EPtr->velocity);

				NotifyOthers(GroupPtr->race_id, IpnlInterceptPlayer);
				task = GroupPtr->task;
				target_loc = GroupPtr->dest_loc;
				if (target_loc == IpnlInterceptPlayer)
				{
					target_loc = flagship_loc;
				}
			}
		}
	}

	GetCurrentVelocityComponents(&EPtr->velocity, &vdx, &vdy);

	task &= ~GroupTaskFlag::IgnoreFlagship;
	// Make sure we have images for the xform and ywing
	if (uqm::currentSeed().ships)
	{
		if (!Xform)
		{
			Xform = CaptureDrawable(
				LoadGraphic("ship.mmrnmhrm.meleeicons"));
		}
		if (!Ywing)
		{
			Ywing = CaptureDrawable(
				LoadGraphic("ship.mmrnmhrm.meleeicons.y"));
		}
	}

#ifdef NEVER
	if (task <= GroupTask::Flee || (task == GroupTask::OnStation && GroupPtr->dest_loc == 0))
#else
	if (task <= GroupTask::OnStation)
#endif /* NEVER */
	{
		int16_t delta_x {};
		int16_t delta_y {};
		uint16_t angle {};
		FrameRef suggestedFrame {};
		const bool FilthyCheater =
			(GroupPtr->race_id == RaceId::UrquanDrone
			 && (uqm::UQMOptions::read().bubbleWarp
				 || CountSISPieces(*ModuleType::FusionThruster) > 6));

		bool Transition {false};
		bool isOrbiting {false};
		if (task == GroupTask::Flee)
		{
			dest_pt.x = GroupPtr->loc.x << 1;
			dest_pt.y = GroupPtr->loc.y << 1;
		}
		else if (((task != GroupTask::OnStation || GroupPtr->dest_loc == IpnlInterceptPlayer)
				  && group_loc == target_loc)
				 || (task == GroupTask::OnStation && GroupPtr->dest_loc != IpnlInterceptPlayer
					 && group_loc == 0))
		{
			if (GroupPtr->dest_loc == IpnlInterceptPlayer)
			{
				dest_pt = uqm::flagship().ipLocation;
			}
			// ship is circling around a planet.
			else
			{
				uint16_t orbit_dist {};
				GfxPoint org;

				isOrbiting = true;
				if (task != GroupTask::OnStation)
				{
					orbit_dist = OrbitRadius;
					org.x = org.y = 0;
				}
				else
				{
					orbit_dist = StationRadius;
					org = planetOuterLocation(target_loc - 1);
				}

				angle = FacingToAngle(GroupPtr->orbit_pos + 1);
				dest_pt.x = org.x + Cosine(angle, orbit_dist);
				dest_pt.y = org.y + Sine(angle, orbit_dist);
				if (GroupPtr->loc.x == dest_pt.x
					&& GroupPtr->loc.y == dest_pt.y)
				{
					uint8_t next_loc {};

					GroupPtr->orbit_pos = NormalizeFacing(
						AngleToFacing(angle));
					angle += FacingToAngle(1);
					dest_pt.x = org.x + Cosine(angle, orbit_dist);
					dest_pt.y = org.y + Sine(angle, orbit_dist);

					EPtr->thrust_wait = (uint8_t)~0;
					if (GroupPtr->group_counter)
					{
						--GroupPtr->group_counter;
					}
					else if (task == GroupTask::Explore
							 && (next_loc = (uint8_t)(((uint16_t)uqm::random().next()
													   % pSolarSysState->SunDesc[0].NumPlanets)
													  + 1))
									!= target_loc)
					{
						EPtr->thrust_wait = 0;
						target_loc = next_loc;
						GroupPtr->dest_loc = next_loc;
					}
				}
			}
		}
		else if (group_loc == 0)
		{
			if (GroupPtr->dest_loc == IpnlInterceptPlayer)
			{
				dest_pt = pSolarSysState->SunDesc[0].location;
			}
			else
			{
				dest_pt = planetOuterLocation(target_loc - 1);
			}
		}
		else
		{
			if (task == GroupTask::OnStation)
			{
				target_loc = 0;
			}

			dest_pt.x = GroupPtr->loc.x << 1;
			dest_pt.y = GroupPtr->loc.y << 1;
		}

		delta_x = dest_pt.x - GroupPtr->loc.x;
		delta_y = dest_pt.y - GroupPtr->loc.y;
		angle = ARCTAN(delta_x, delta_y);

		if (EPtr->thrust_wait && EPtr->thrust_wait != (uint8_t)~0)
		{
			--EPtr->thrust_wait;
		}
		else if ((vdx == 0 && vdy == 0)
				 || angle != GetVelocityTravelAngle(&EPtr->velocity))
		{
			int16_t speed {};

			if (EPtr->thrust_wait && GroupPtr->dest_loc != IpnlInterceptPlayer)
			{
				speed = OrbitSpeed;
				if (task == GroupTask::OnStation)
				{
					speed >>= 1;
				}
			}
			else
			{
				speed = RaceIPSpeed((RaceId)GroupPtr->race_id);
				EPtr->thrust_wait = TrackWait;
			}

			vdx = Cosine(angle, speed);
			vdy = Sine(angle, speed);
			SetVelocityComponents(&EPtr->velocity, vdx, vdy);
		}

		int16_t dx {vdx};
		int16_t dy {vdy};
		if (group_loc == target_loc)
		{
			if (target_loc == 0)
			{
				if (task == GroupTask::Flee)
				{
					goto CheckGetAway;
				}
			}
			else if (target_loc == GroupPtr->dest_loc)
			{
PartialRevolution:
				if ((long)((uint16_t)(dx * dx) + (uint16_t)(dy * dy))
					>= (long)delta_x * delta_x + (long)delta_y * delta_y)
				{
					GroupPtr->loc = dest_pt;
					vdx = 0;
					vdy = 0;
					ZeroVelocityComponents(&EPtr->velocity);
				}
			}
		}
		else
		{
			if (group_loc == 0)
			{ // In outer system
				adjustDeltaVforZoom(radius, &dx, &dy);

				if (task == GroupTask::OnStation && GroupPtr->dest_loc)
				{
					goto PartialRevolution;
				}
				else if ((long)((uint16_t)(dx * dx) + (uint16_t)(dy * dy))
						 >= (long)delta_x * delta_x + (long)delta_y * delta_y)
				{
					Transition = true;
				}
			}
			else
			{ // In inner system; also leaving outer CheckGetAway hack
CheckGetAway:
				dest_pt = displayToLocation(
					MAKE_POINT(SisScreenWidth(), SisScreenHeight()),
					MaxZoomRadius);
				if (!((GroupPtr->loc.x > -dest_pt.x
					   && GroupPtr->loc.y > -dest_pt.y)
					  && (GroupPtr->loc.x < dest_pt.x
						  && GroupPtr->loc.y < dest_pt.y))
					|| FilthyCheater)
				{
					Transition = true;
				}
			}

			if (Transition)
			{
				/* no collisions during transition */
				EPtr->state_flags |= Nonsolid;

				vdx = 0;
				vdy = 0;
				ZeroVelocityComponents(&EPtr->velocity);
				if (group_loc != 0)
				{
					GroupPtr->loc = planetOuterLocation(group_loc - 1);
					group_loc = 0;
					GroupPtr->sys_loc = 0;
				}
				else if (target_loc == 0)
				{
					/* Group completely left the star system */
					EPtr->life_span = 0;
					EPtr->state_flags |= Disappearing | Nonsolid;
					GroupPtr->in_system = 0;
					return;
				}
				else
				{
					GfxPoint entryPt;

					if (target_loc == GroupPtr->dest_loc)
					{
						GroupPtr->orbit_pos = NormalizeFacing(
							AngleToFacing(angle + HalfCircle));
						GroupPtr->group_counter =
							((uint16_t)uqm::random().next() % MaxRevolutions)
							<< FacingShift;
					}
					// The group enters inner system exactly on the edge of
					// a circle with radius = 9/16 * window-dim, which is
					// different from how the flagship enters, but similar
					// in the way that the group will never show up in any
					// of the corners.
					entryPt.x = ResScale(OrigSisScreenWidth() >> 1)
							  - Cosine(angle, SisScreenWidth() * 9 / 16);
					entryPt.y = ResScale(OrigSisScreenHeight() >> 1)
							  - Sine(angle, SisScreenHeight() * 9 / 16);
					GroupPtr->loc = displayToLocation(entryPt,
													  MaxZoomRadius);
					group_loc = target_loc;
					GroupPtr->sys_loc = target_loc;
				}
			}
		}

		if (uqm::UQMOptions::read().shipDirectionIP)
		{
			if (GroupPtr->flags & Rotates)
			{ // BW : make IP ships face the direction they're going into
				if (uqm::currentActivity() & ActivityFlag::StartEncounter)
				{ // sometimes they give up chase, don't turn them away
					// from sis during red alert phase
					suggestedFrame =
						SetAbsFrameIndex(EPtr->next.image.farray[0],
										 GetFrameIndex(EPtr->current.image.frame));
				}
				else
				{
					suggestedFrame =
						SetAbsFrameIndex(EPtr->next.image.farray[0],
										 2 + NormalizeFacing(AngleToFacing(ARCTAN(delta_x, delta_y))));
				}

				// Set ship sprite when player entering the system (image
				// index is always 1 by default)
				if (GetFrameIndex(EPtr->current.image.frame) == 1)
				{
					EPtr->next.image.frame = suggestedFrame;
				}

				if (isOrbiting)
				{ // JMS: Direction memory prevents jittering of battle
					// group icons when they are orbiting a planet (and not
					// chasing the player ship).
					if (GroupPtr->flags & CanTurn)
					{
						EPtr->next.image.frame = suggestedFrame;
						GroupPtr->flags &= ~CanTurn;
						// cannot turn until destination is reached
					}
				}
				else
				{
					EPtr->next.image.frame = suggestedFrame;
				}
			}
			// Tumble any probes
			if (GroupPtr->flags & IsProbe)
			{
				EPtr->next.image.frame =
					IncFrameIndex(EPtr->next.image.frame);
			}
			// Transform any xforms
			if (GroupPtr->flags & IsXform && GroupPtr->dest_loc == IpnlInterceptPlayer)
			{
				GroupPtr->melee_icon = Ywing;
			}
			if (GroupPtr->flags & IsXform && GroupPtr->dest_loc != IpnlInterceptPlayer)
			{
				GroupPtr->melee_icon = Xform;
			}
			// If destination reached - ship can turn (or ship
			// leaves/enters inner system, but not reached destination yet)
			if ((dest_pt.x == GroupPtr->loc.x
				 && dest_pt.y == GroupPtr->loc.y)
				|| Transition)
			{
				GroupPtr->flags |= CanTurn;
			}
		}
	}
	else if (task >= *GroupTaskFlag::ReformGroup && uqm::UQMOptions::read().shipDirectionIP)
	{ // To face sis while reforming
		if (GroupPtr->flags & Rotates)
		{
			if (GetFrameIndex(EPtr->current.image.frame) == 1)
			{ // Define direction only once and not follow player while
				// reforming
				int16_t delta_x {};
				int16_t delta_y {};
				GfxPoint sis_pt;

				sis_pt = displayToLocation(
					uqm::flagship().shipStamp.origin, radius);

				// Destination point is sis location
				delta_x = sis_pt.x - GroupPtr->loc.x;
				delta_y = sis_pt.y - GroupPtr->loc.y;

				EPtr->next.image.frame =
					SetAbsFrameIndex(EPtr->next.image.farray[0],
									 2 + NormalizeFacing(AngleToFacing(ARCTAN(delta_x, delta_y))));

				GroupPtr->flags |= CanTurn;
			}
		}
		else if (GroupPtr->flags & IsProbe)
		{ // Probe will wobble while reforming
			EPtr->next.image.frame =
				IncFrameIndex(EPtr->next.image.frame);
		}
	}

	radius = zoomRadiusForLocation(group_loc);
	adjustDeltaVforZoom(radius, &vdx, &vdy);
	GroupPtr->loc.x += vdx;
	GroupPtr->loc.y += vdy;

	dest_pt = locationToDisplay(GroupPtr->loc, radius);
	EPtr->next.location.x = DisplayToWorld(dest_pt.x)
						  + (int16_t)(LogSpaceWidth() >> 1)
						  - (LogSpaceWidth() >> (MaxReduction + 1));
	EPtr->next.location.y = DisplayToWorld(dest_pt.y)
						  + (int16_t)(LogSpaceHeight() >> 1)
						  - (LogSpaceHeight() >> (MaxReduction + 1));

	// Don't draw the group if it's not at flagship location,
	// or flash the group while it's reforming
	if (group_loc != flagship_loc
		|| ((task & GroupTaskFlag::ReformGroup)
			&& (GroupPtr->group_counter & 1)))
	{
		SetPrimType(&DisplayArray[EPtr->PrimIndex], NoPrim);
		EPtr->state_flags |= Nonsolid;
	}
	else
	{
		SetPrimType(&DisplayArray[EPtr->PrimIndex], PrimitiveType::Stamp);
		if (task & GroupTaskFlag::ReformGroup)
		{
			EPtr->state_flags |= Nonsolid;
		}
	}

	EPtr->state_flags |= Changing;
}

static void flag_ship_collision(Element* ElementPtr0, GfxPoint* pPt0,
								Element* ElementPtr1, GfxPoint* pPt1)
{
	if (uqm::currentActivity() & ActivityFlag::StartEncounter)
	{
		return; // ignore the rest of the collisions
	}

	if (!(ElementPtr1->state_flags & Collision))
	{ // The other Element's collision has not been processed yet
		// Defer starting the Encounter until it is.
		ElementPtr0->state_flags |= Collision | Nonsolid;
	}
	else
	{ // Both Element's collisions have now been processed
		ElementPtr1->state_flags &= ~Collision;
		uqm::currentActivity() |= ActivityFlag::StartEncounter;
	}
	(void)pPt0; /* Satisfying compiler (unused parameter) */
	(void)pPt1; /* Satisfying compiler (unused parameter) */
}

static void ip_group_collision(Element* ElementPtr0, GfxPoint* pPt0,
							   Element* ElementPtr1, GfxPoint* pPt1)
{
	InterplanetaryGroup* GroupPtr {};
	void* OtherPtr {};

	if (uqm::currentActivity() & ActivityFlag::StartEncounter)
	{
		return; // ignore the rest of the collisions
	}

	*(&GroupPtr) = (InterplanetaryGroup*)(ElementPtr0)->pParent;
	OtherPtr = ElementPtr1->pParent;

	if (!(uqm::flagship().autopilot.target().x == ~0 && uqm::flagship().autopilot.target().y == ~0)
		&& (CheckAlliance((RaceId)GroupPtr->race_id) == RaceAllegiance::Good))
	{
		return; // Ignore collisions when allied during Auto-Pilot
	}

	if (OtherPtr)
	{ // Collision with another group
		// Prevent the groups from coalescing into a single ship icon
		if ((ElementPtr0->state_flags & Collision)
			|| (ElementPtr1->current.location.x == ElementPtr1->next.location.x
				&& ElementPtr1->current.location.y == ElementPtr1->next.location.y))
		{
			ElementPtr0->state_flags &= ~Collision;
		}
		else
		{
			ElementPtr1->state_flags |= Collision;

			GroupPtr->loc = DisplayArray[ElementPtr0->PrimIndex].Object.Point;
			ElementPtr0->next.location = ElementPtr0->current.location;
			InitIntersectEndPoint(ElementPtr0);
		}
	}
	else // if (!OtherPtr)
	{	 // Collision with a flagship
		EncounterGroup = GroupPtr->group_id;

		GroupPtr->task |= GroupTaskFlag::ReformGroup;
		GroupPtr->group_counter = 100;
		// Send "all clear" for the time being. After the Encounter, if
		// the player battles the group, the "intercept" notify will be
		// resent.
		NotifyOthers(GroupPtr->race_id, IpnlAllClear);

		if (!(ElementPtr1->state_flags & Collision))
		{ // The other Element's collision has not been processed yet
			// Defer starting the Encounter until it is.
			ElementPtr0->state_flags |= Collision | Nonsolid;
		}
		else
		{ // Both Element's collisions have now been processed
			ElementPtr1->state_flags &= ~Collision;
			uqm::currentActivity() |= ActivityFlag::StartEncounter;
		}
	}
	(void)pPt0; /* Satisfying compiler (unused parameter) */
	(void)pPt1; /* Satisfying compiler (unused parameter) */
}

static void spawn_ip_group(InterplanetaryGroup* GroupPtr)
{
	ElementHandle hIPSHIPElement {AllocElement()};
	if (hIPSHIPElement)
	{
		Element* IPSHIPElementPtr {};

		LockElement(hIPSHIPElement, &IPSHIPElementPtr);
		// Must have mass_points for collisions to work
		IPSHIPElementPtr->mass_points = 1;
		IPSHIPElementPtr->hit_points = 1;
		IPSHIPElementPtr->state_flags =
			Changing | FiniteLife | IgnoreVelocity;

		SetPrimType(&DisplayArray[IPSHIPElementPtr->PrimIndex], PrimitiveType::Stamp);
		// XXX: Hack: farray points to FrameRef[3] and given FrameRef
		IPSHIPElementPtr->current.image.farray = &GroupPtr->melee_icon;
		IPSHIPElementPtr->current.image.frame = SetAbsFrameIndex(
			GroupPtr->melee_icon, 1);
		/* preprocessing has a side effect
			 * we wish to avoid.  So death_func
			 * is used instead, but will achieve
			 * same result without the side
			 * effect (InitIntersectFrame)
			 */
		GroupPtr->flags = 0;
		if (GroupPtr->race_id != RaceId::Slylandro)
		{
			GroupPtr->flags |= Rotates;
		}
		if (GroupPtr->race_id == RaceId::Mmrnmhrm)
		{
			GroupPtr->flags |= IsXform;
		}
		if (GroupPtr->race_id == RaceId::Slylandro)
		{
			GroupPtr->flags |= IsProbe;
		}
		if (uqm::currentSeed().ships)
		{
			const FleetInfo* TemplatePtr = nullptr;
			FleetInfoHandle hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, GroupPtr->race_id)};
			if (hFleet)
			{
				TemplatePtr = LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);
			}
			if (TemplatePtr)
			{
				const SpeciesId ship = SeedShip(TemplatePtr->SpeciesID, false);
				UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);
				GroupPtr->flags = 0;
				switch (ship)
				{
					case SpeciesId::Slylandro:
						GroupPtr->flags |= IsProbe;
						break;
					case SpeciesId::Mmrnmhrm:
						GroupPtr->flags |= IsXform + Rotates;
						break;
					case SpeciesId::Orz:
					case SpeciesId::Utwig:
					case SpeciesId::Yehat:
					case SpeciesId::Mycon:
					case SpeciesId::UrQuan:
					case SpeciesId::KohrAh:
					case SpeciesId::Arilou:
					case SpeciesId::Pkunk:
					case SpeciesId::Spathi:
					case SpeciesId::Supox:
					case SpeciesId::Melnorme:
					case SpeciesId::Druuge:
					case SpeciesId::Shofixti:
					case SpeciesId::Thraddash:
					case SpeciesId::Vux:
					case SpeciesId::Ilwrath:
					case SpeciesId::Umgah:
					case SpeciesId::Zoqfotpik:
					case SpeciesId::Chmmr:
					case SpeciesId::Chenjesu:
					case SpeciesId::Androsynth:
					case SpeciesId::Earthling:
					case SpeciesId::Syreen:
						GroupPtr->flags |= Rotates;
						break;
					default:
						break;
				}
			}
		}
		if (uqm::UQMOptions::read().shipDirectionIP && GroupPtr->flags & IsProbe)
		{
			GroupPtr->melee_icon =
				CaptureDrawable(
					LoadGraphic(SlylandroSmlMaskPmapAnim));
			IPSHIPElementPtr->current.image.frame = SetAbsFrameIndex(
				GroupPtr->melee_icon, (uqm::random().next() % (15 - 1) + 1));
			// randomize initial sprite
		}
		IPSHIPElementPtr->death_func = ip_group_preprocess;
		IPSHIPElementPtr->collision_func = ip_group_collision;

		{
			GfxPoint pt;

			const int16_t radius {zoomRadiusForLocation(GroupPtr->sys_loc)};
			pt = locationToDisplay(GroupPtr->loc, radius);

			IPSHIPElementPtr->current.location.x =
				DisplayToWorld(pt.x)
				+ (int16_t)(LogSpaceWidth() >> 1)
				- (LogSpaceWidth() >> (MaxReduction + 1));
			IPSHIPElementPtr->current.location.y =
				DisplayToWorld(pt.y)
				+ (int16_t)(LogSpaceHeight() >> 1)
				- (LogSpaceHeight() >> (MaxReduction + 1));
		}

		IPSHIPElementPtr->pParent = GroupPtr;

		SetUpElement(IPSHIPElementPtr);
		IPSHIPElementPtr->IntersectControl.IntersectStamp.frame =
			DecFrameIndex(stars_in_space);

		UnlockElement(hIPSHIPElement);

		PutElement(hIPSHIPElement);
	}
}

static constexpr auto FlipWait {42};

static void flag_ship_preprocess(Element* ElementPtr)
{
	if (--ElementPtr->thrust_wait == 0)
	/* juggle list after flagship */
	{
		ElementHandle hSuccElement {};

		if ((hSuccElement = GetSuccElement(ElementPtr))
			&& hSuccElement != GetTailElement())
		{
			Element* TailPtr {};

			LockElement(GetTailElement(), &TailPtr);
			ElementHandle hPredElement {_GetPredLink(TailPtr)};
			UnlockElement(GetTailElement());

			RemoveElement(hSuccElement);
			PutElement(hSuccElement);

			(void)hPredElement; // satisfy compiler (unused variable)
		}

		ElementPtr->thrust_wait = FlipWait;
	}

	{
		uint8_t ec {};
		int16_t vdx {};
		int16_t vdy {};
		GfxPoint pt;

		GetCurrentVelocityComponents(&uqm::flagship().velocity, &vdx, &vdy);

		if (!legacySave)
		{
			vdx >>= 1;
			vdy >>= 1;
		}
		else if ((CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][*BattleKey::Up]
				  || CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][*BattleKey::Thrust])
				 || (vdx == 0 && vdy == 0))
		{
			legacySave = false;
		}

		const uint8_t flagship_loc {getFlagshipLocation()};
		const int16_t radius {zoomRadiusForLocation(flagship_loc)};
		adjustDeltaVforZoom(radius, &vdx, &vdy);

		pt = locationToDisplay(uqm::flagship().ipLocation, radius);
		ElementPtr->current.location.x = DisplayToWorld(pt.x)
									   + (int16_t)(LogSpaceWidth() >> 1)
									   - (LogSpaceWidth() >> (MaxReduction + 1));
		ElementPtr->current.location.y = DisplayToWorld(pt.y)
									   + (int16_t)(LogSpaceHeight() >> 1)
									   - (LogSpaceHeight() >> (MaxReduction + 1));
		InitIntersectStartPoint(ElementPtr);

		uqm::flagship().ipLocation.x += vdx;
		uqm::flagship().ipLocation.y += vdy;

		pt = locationToDisplay(uqm::flagship().ipLocation, radius);
		ElementPtr->next.location.x = DisplayToWorld(pt.x)
									+ (int16_t)(LogSpaceWidth() >> 1)
									- (LogSpaceWidth() >> (MaxReduction + 1));
		ElementPtr->next.location.y = DisplayToWorld(pt.y)
									+ (int16_t)(LogSpaceHeight() >> 1)
									- (LogSpaceHeight() >> (MaxReduction + 1));

		uqm::flagship().shipStamp.origin = pt;
		ElementPtr->next.image.frame = uqm::flagship().shipStamp.frame;

		if (ElementPtr->sys_loc == flagship_loc)
		{
			if (ElementPtr->state_flags & Nonsolid)
			{
				ElementPtr->state_flags &= ~Nonsolid;
			}
		}
		else /* no collisions during transition */
		{
			ElementPtr->state_flags |= Nonsolid;
			ElementPtr->sys_loc = flagship_loc;
		}

		if ((ec = uqm::gameState().flag(uqm::GameFlag::ESCAPE_COUNTER))
			&& !(uqm::currentActivity() & ActivityFlag::StartEncounter))
		{
			ElementPtr->state_flags |= Nonsolid;

			--ec;
			uqm::gameState().setFlag(uqm::GameFlag::ESCAPE_COUNTER, static_cast<uint32_t>(ec));
		}

		ElementPtr->state_flags |= Changing;
	}
}

static void AdjustInitialPosition(void)
{ // Corrects SIS position - rounding error, described in
	// EnterPlanetOrbit() in solarsys.c
	GfxPoint pt;

	const uint8_t flagship_loc {getFlagshipLocation()};
	const int16_t radius {zoomRadiusForLocation(flagship_loc)};

	pt = locationToDisplay(uqm::flagship().ipLocation, radius);

	if (LastActivity & ActivityFlag::CheckLoad)
	{
		InitialIntersect();
	}

	uqm::flagship().shipStamp.origin = pt;
}

static void spawn_flag_ship(void)
{
	ElementHandle hFlagShipElement {AllocElement()};
	if (hFlagShipElement)
	{
		Element* FlagShipElementPtr {};

		LockElement(hFlagShipElement, &FlagShipElementPtr);
		FlagShipElementPtr->hit_points = 1;
		// Must have mass_points for collisions to work
		FlagShipElementPtr->mass_points = 1;
		FlagShipElementPtr->sys_loc = getFlagshipLocation();
		FlagShipElementPtr->state_flags = Appearing | IgnoreVelocity;
		if (uqm::gameState().flag(uqm::GameFlag::ESCAPE_COUNTER))
		{
			FlagShipElementPtr->state_flags |= Nonsolid;
		}
		FlagShipElementPtr->life_span = NormalLife;
		FlagShipElementPtr->thrust_wait = FlipWait;
		SetPrimType(&DisplayArray[FlagShipElementPtr->PrimIndex], PrimitiveType::Stamp);
		FlagShipElementPtr->current.image.farray =
			&uqm::flagship().shipStamp.frame;
		FlagShipElementPtr->current.image.frame =
			uqm::flagship().shipStamp.frame;
		FlagShipElementPtr->preprocess_func = flag_ship_preprocess;
		FlagShipElementPtr->collision_func = flag_ship_collision;

		AdjustInitialPosition();

		FlagShipElementPtr->current.location.x =
			DisplayToWorld(uqm::flagship().shipStamp.origin.x)
			+ (int16_t)(LogSpaceWidth() >> 1)
			- (LogSpaceWidth() >> (MaxReduction + 1));
		FlagShipElementPtr->current.location.y =
			DisplayToWorld(uqm::flagship().shipStamp.origin.y)
			+ (int16_t)(LogSpaceHeight() >> 1)
			- (LogSpaceHeight() >> (MaxReduction + 1));

		UnlockElement(hFlagShipElement);

		PutElement(hFlagShipElement);
	}
}

void DoMissions(void)
{
	ShipFragmentHandle hGroup {};
	ShipFragmentHandle hNextGroup {};

	spawn_flag_ship();

	if (EncounterRace >= 0)
	{ // There was a battle. Call in reinforcements.
		NotifyOthers(EncounterRace, IpnlInterceptPlayer);
		EncounterRace = -1;
	}

	for (hGroup = GetHeadLink(&uqm::gameState().game().ip_group_q);
		 hGroup; hGroup = hNextGroup)
	{
		InterplanetaryGroup* GroupPtr {LockIpGroup(&uqm::gameState().game().ip_group_q, hGroup)};
		hNextGroup = _GetSuccLink(GroupPtr);

		if (GroupPtr->in_system
			&& CheckAlliance((RaceId)GroupPtr->race_id) != RaceAllegiance::Dead)
		{
			spawn_ip_group(GroupPtr);
		}

		UnlockIpGroup(&uqm::gameState().game().ip_group_q, hGroup);
	}
}

} // namespace uqm
