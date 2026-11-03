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

bool legacySave;
uint8_t GTFO = 0;
extern FRAME SpaceJunkFrame;

static inline int16_t
RaceIPSpeed(RACE_ID Index)
{
	static int seedStamp = -1;
	const int16_t defaultMap[] = {RACE_IP_SPEED};
	const uint16_t numRaces = sizeof(defaultMap) / sizeof(uint16_t);
	static int16_t speedMap[sizeof(defaultMap) / sizeof(uint16_t)] = {0};
	int x {};
	if (!uqm::UQMOptions::read().shipSeed && (seedStamp != -1 || speedMap[0] == 0))
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

	if (uqm::UQMOptions::read().shipSeed && (seedStamp != uqm::UQMOptions::read().customSeed || speedMap[0] == 0))
	{
		HFLEETINFO hFleet {};
		for (x = 0; x < numRaces; x++)
		{
			hFleet = GetSeededFleetFromIndex(x);
			if (hFleet)
			{
				speedMap[x] = defaultMap[GetIndexFromStarShip(
					&GLOBAL(avail_race_q), hFleet)];
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
	return speedMap[Index];
}

void NotifyOthers(uint16_t which_race, uint8_t target_loc)
{
	HSHIPFRAG hGroup {};
	HSHIPFRAG hNextGroup {};

	// NOTE: "Others" includes the group causing the notification too.

	for (hGroup = GetHeadLink(&GLOBAL(ip_group_q));
		 hGroup; hGroup = hNextGroup)
	{
		IP_GROUP* GroupPtr {LockIpGroup(&GLOBAL(ip_group_q), hGroup)};
		hNextGroup = _GetSuccLink(GroupPtr);

		if (GroupPtr->race_id != which_race)
		{
			UnlockIpGroup(&GLOBAL(ip_group_q), hGroup);
			continue;
		}

		if (target_loc == IPNL_INTERCEPT_PLAYER)
		{
			GroupPtr->task &= ~IGNORE_FLAGSHIP;
			// XXX: orbit_pos is abused here to store the previous
			//   group destination, before the intercept task.
			//   Returned to dest_loc below.
			GroupPtr->orbit_pos = GroupPtr->dest_loc;
			GroupPtr->dest_loc = IPNL_INTERCEPT_PLAYER;
		}
		else if (target_loc == IPNL_ALL_CLEAR)
		{
			GroupPtr->task |= IGNORE_FLAGSHIP;

			if (GroupPtr->dest_loc == IPNL_INTERCEPT_PLAYER)
			{ // The group was intercepting, so send it back where it came
				// XXX: orbit_pos was abused to store the previous
				//   group destination, before the intercept task.
				GroupPtr->dest_loc = GroupPtr->orbit_pos;
				GroupPtr->orbit_pos = NORMALIZE_FACING(TFB_Random());
#ifdef OLD
				GroupPtr->dest_loc = (uint8_t)(((uint16_t)TFB_Random()
												% pSolarSysState->SunDesc[0].NumPlanets)
											   + 1);
#endif /* OLD */
			}
			// If the group wasn't intercepting, it will just continue
			// going about its business.

			if (!(GroupPtr->task & REFORM_GROUP))
			{
				if ((GroupPtr->task & ~IGNORE_FLAGSHIP) != EXPLORE)
				{
					GroupPtr->group_counter = 0;
				}
				else
				{
					GroupPtr->group_counter = ((uint16_t)TFB_Random()
											   % MAX_REVOLUTIONS)
										   << FACING_SHIFT;
				}
			}
		}
		else
		{ // Send the group to the location.
			// XXX: There is currently no use of such notify that I know of.
			GroupPtr->task &= REFORM_GROUP;
			GroupPtr->task |= FLEE | IGNORE_FLAGSHIP;
			GroupPtr->dest_loc = 0;
		}

		UnlockIpGroup(&GLOBAL(ip_group_q), hGroup);
	}
}

static int16_t
zoomRadiusForLocation(uint8_t location)
{
	if (location == 0)
	{ // In outer system view; use current zoom radius
		return pSolarSysState->SunDesc[0].radius;
	}
	else
	{ // In inner system view; always max zoom
		return MAX_ZOOM_RADIUS;
	}
}

static inline void
adjustDeltaVforZoom(int16_t zoom, int16_t* dx, int16_t* dy)
{
	if (zoom == MIN_ZOOM_RADIUS)
	{
		*dx >>= 2;
		*dy >>= 2;
	}
	else if (zoom < MAX_ZOOM_RADIUS)
	{
		*dx >>= 1;
		*dy >>= 1;
	}
}

static uint8_t
getFlagshipLocation(void)
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
FRAME Xform = NULL_RESOURCE;
FRAME Ywing = NULL_RESOURCE;

static void
ip_group_preprocess(ELEMENT* ElementPtr)
{
#define TRACK_WAIT 5
	uint8_t task {};
	uint8_t group_loc {};
	GFXPOINT dest_pt;
	int16_t vdx {};
	int16_t vdy {};
	IP_GROUP* GroupPtr {};

	ELEMENT* EPtr {ElementPtr};
	EPtr->state_flags &= ~(DISAPPEARING | NONSOLID); // "I'm not quite dead"
	++EPtr->life_span;								 // so that it will 'die' again next time

	*(&GroupPtr) = (IP_GROUP*)(EPtr)->pParent;
	group_loc = GroupPtr->sys_loc; // save old location
	DisplayArray[EPtr->PrimIndex].Object.Point = GroupPtr->loc;

	int16_t radius {zoomRadiusForLocation(group_loc)};
	dest_pt = locationToDisplay(GroupPtr->loc, radius);
	EPtr->current.location.x = DISPLAY_TO_WORLD(dest_pt.x)
							 + (int16_t)(LOG_SPACE_WIDTH >> 1)
							 - (LOG_SPACE_WIDTH >> (MAX_REDUCTION + 1));
	EPtr->current.location.y = DISPLAY_TO_WORLD(dest_pt.y)
							 + (int16_t)(LOG_SPACE_HEIGHT >> 1)
							 - (LOG_SPACE_HEIGHT >> (MAX_REDUCTION + 1));

	InitIntersectStartPoint(EPtr);

	const uint8_t flagship_loc {getFlagshipLocation()};

	if ((GET_GAME_STATE(KOHR_AH_FRENZY)
		 && CheckAlliance((RACE_ID)GroupPtr->race_id) == DEAD_GUY)
		|| (GTFO == 1 && GroupPtr->race_id == ILWRATH_SHIP))
	{
		GTFO = 0;
		NotifyOthers(GroupPtr->race_id, IPNL_FLEE);
	}

	if (GTFO == 2 && GroupPtr->race_id == ILWRATH_SHIP)
	{
		GTFO = 0;
		NotifyOthers(ILWRATH_SHIP, IPNL_INTERCEPT_PLAYER);
	}

	task = GroupPtr->task;

	if ((task & REFORM_GROUP) && --GroupPtr->group_counter == 0)
	{ // Finished reforming the group
		task &= ~REFORM_GROUP;
		GroupPtr->task = task;
		if ((task & ~IGNORE_FLAGSHIP) != EXPLORE)
		{
			GroupPtr->group_counter = 0;
		}
		else
		{
			GroupPtr->group_counter = ((uint16_t)TFB_Random()
									   % MAX_REVOLUTIONS)
								   << FACING_SHIFT;
		}
	}

	// If fleeing *and* ignoring flagship
	if ((task & ~(IGNORE_FLAGSHIP | REFORM_GROUP)) == FLEE
		&& (task & IGNORE_FLAGSHIP))
	{ // Make fleeing groups non-collidable
		EPtr->state_flags |= NONSOLID;
	}

	uint8_t target_loc {GroupPtr->dest_loc};
	if (!(task & (IGNORE_FLAGSHIP | REFORM_GROUP)))
	{
		if (target_loc == IPNL_INTERCEPT_PLAYER && task != FLEE)
		{
			/* if intercepting flagship */
			target_loc = flagship_loc;
			if (EPtr->thrust_wait > TRACK_WAIT)
			{
				EPtr->thrust_wait = 0;
				ZeroVelocityComponents(&EPtr->velocity);
			}
		}
		else if (group_loc == flagship_loc)
		{
			int16_t detect_dist = 1200;

			if (EXTENDED && CheckAlliance((RACE_ID)GroupPtr->race_id) == GOOD_GUY)
			{
				detect_dist = 0;
			}

			if (group_loc != 0) /* if in planetary views */
			{
				detect_dist *= (MAX_ZOOM_RADIUS / MIN_ZOOM_RADIUS);
				if (GroupPtr->race_id == URQUAN_DRONE_SHIP)
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

				NotifyOthers(GroupPtr->race_id, IPNL_INTERCEPT_PLAYER);
				task = GroupPtr->task;
				target_loc = GroupPtr->dest_loc;
				if (target_loc == IPNL_INTERCEPT_PLAYER)
				{
					target_loc = flagship_loc;
				}
			}
		}
	}

	GetCurrentVelocityComponents(&EPtr->velocity, &vdx, &vdy);

	task &= ~IGNORE_FLAGSHIP;
	// Make sure we have images for the xform and ywing
	if (uqm::UQMOptions::read().shipSeed)
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
	if (task <= FLEE || (task == ON_STATION && GroupPtr->dest_loc == 0))
#else
	if (task <= ON_STATION)
#endif /* NEVER */
	{
		int16_t delta_x {};
		int16_t delta_y {};
		uint16_t angle {};
		FRAME suggestedFrame {};
		const bool FilthyCheater =
			(GroupPtr->race_id == URQUAN_DRONE_SHIP
			 && (uqm::UQMOptions::read().bubbleWarp
				 || CountSISPieces(FUSION_THRUSTER) > 6));

		bool Transition {false};
		bool isOrbiting {false};
		if (task == FLEE)
		{
			dest_pt.x = GroupPtr->loc.x << 1;
			dest_pt.y = GroupPtr->loc.y << 1;
		}
		else if (((task != ON_STATION || GroupPtr->dest_loc == IPNL_INTERCEPT_PLAYER)
				  && group_loc == target_loc)
				 || (task == ON_STATION && GroupPtr->dest_loc != IPNL_INTERCEPT_PLAYER
					 && group_loc == 0))
		{
			if (GroupPtr->dest_loc == IPNL_INTERCEPT_PLAYER)
			{
				dest_pt = uqm::flagship().ipLocation;
			}
			// ship is circling around a planet.
			else
			{
				uint16_t orbit_dist {};
				GFXPOINT org;

				isOrbiting = true;
				if (task != ON_STATION)
				{
					orbit_dist = ORBIT_RADIUS;
					org.x = org.y = 0;
				}
				else
				{
					orbit_dist = STATION_RADIUS;
					org = planetOuterLocation(target_loc - 1);
				}

				angle = FACING_TO_ANGLE(GroupPtr->orbit_pos + 1);
				dest_pt.x = org.x + COSINE(angle, orbit_dist);
				dest_pt.y = org.y + SINE(angle, orbit_dist);
				if (GroupPtr->loc.x == dest_pt.x
					&& GroupPtr->loc.y == dest_pt.y)
				{
					uint8_t next_loc {};

					GroupPtr->orbit_pos = NORMALIZE_FACING(
						ANGLE_TO_FACING(angle));
					angle += FACING_TO_ANGLE(1);
					dest_pt.x = org.x + COSINE(angle, orbit_dist);
					dest_pt.y = org.y + SINE(angle, orbit_dist);

					EPtr->thrust_wait = (uint8_t)~0;
					if (GroupPtr->group_counter)
					{
						--GroupPtr->group_counter;
					}
					else if (task == EXPLORE
							 && (next_loc = (uint8_t)(((uint16_t)TFB_Random()
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
			if (GroupPtr->dest_loc == IPNL_INTERCEPT_PLAYER)
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
			if (task == ON_STATION)
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

			if (EPtr->thrust_wait && GroupPtr->dest_loc != IPNL_INTERCEPT_PLAYER)
			{
#define ORBIT_SPEED 60
				speed = ORBIT_SPEED;
				if (task == ON_STATION)
				{
					speed >>= 1;
				}
			}
			else
			{
				speed = RaceIPSpeed((RACE_ID)GroupPtr->race_id);
				EPtr->thrust_wait = TRACK_WAIT;
			}

			vdx = COSINE(angle, speed);
			vdy = SINE(angle, speed);
			SetVelocityComponents(&EPtr->velocity, vdx, vdy);
		}

		int16_t dx {vdx};
		int16_t dy {vdy};
		if (group_loc == target_loc)
		{
			if (target_loc == 0)
			{
				if (task == FLEE)
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

				if (task == ON_STATION && GroupPtr->dest_loc)
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
					MAKE_POINT(SIS_SCREEN_WIDTH, SIS_SCREEN_HEIGHT),
					MAX_ZOOM_RADIUS);
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
				EPtr->state_flags |= NONSOLID;

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
					EPtr->state_flags |= DISAPPEARING | NONSOLID;
					GroupPtr->in_system = 0;
					return;
				}
				else
				{
					GFXPOINT entryPt;

					if (target_loc == GroupPtr->dest_loc)
					{
						GroupPtr->orbit_pos = NORMALIZE_FACING(
							ANGLE_TO_FACING(angle + HALF_CIRCLE));
						GroupPtr->group_counter =
							((uint16_t)TFB_Random() % MAX_REVOLUTIONS)
							<< FACING_SHIFT;
					}
					// The group enters inner system exactly on the edge of
					// a circle with radius = 9/16 * window-dim, which is
					// different from how the flagship enters, but similar
					// in the way that the group will never show up in any
					// of the corners.
					entryPt.x = RES_SCALE(ORIG_SIS_SCREEN_WIDTH >> 1)
							  - COSINE(angle, SIS_SCREEN_WIDTH * 9 / 16);
					entryPt.y = RES_SCALE(ORIG_SIS_SCREEN_HEIGHT >> 1)
							  - SINE(angle, SIS_SCREEN_HEIGHT * 9 / 16);
					GroupPtr->loc = displayToLocation(entryPt,
													  MAX_ZOOM_RADIUS);
					group_loc = target_loc;
					GroupPtr->sys_loc = target_loc;
				}
			}
		}

		if (uqm::UQMOptions::read().shipDirectionIP)
		{
			if (GroupPtr->flags & ROTATES)
			{ // BW : make IP ships face the direction they're going into
				if (uqm::currentActivity() & START_ENCOUNTER)
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
										 2 + NORMALIZE_FACING(ANGLE_TO_FACING(ARCTAN(delta_x, delta_y))));
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
					if (GroupPtr->flags & CAN_TURN)
					{
						EPtr->next.image.frame = suggestedFrame;
						GroupPtr->flags &= ~CAN_TURN;
						// cannot turn until destination is reached
					}
				}
				else
				{
					EPtr->next.image.frame = suggestedFrame;
				}
			}
			// Tumble any probes
			if (GroupPtr->flags & IS_PROBE)
			{
				EPtr->next.image.frame =
					IncFrameIndex(EPtr->next.image.frame);
			}
			// Transform any xforms
			if (GroupPtr->flags & IS_XFORM && GroupPtr->dest_loc == IPNL_INTERCEPT_PLAYER)
			{
				GroupPtr->melee_icon = Ywing;
			}
			if (GroupPtr->flags & IS_XFORM && GroupPtr->dest_loc != IPNL_INTERCEPT_PLAYER)
			{
				GroupPtr->melee_icon = Xform;
			}
			// If destination reached - ship can turn (or ship
			// leaves/enters inner system, but not reached destination yet)
			if ((dest_pt.x == GroupPtr->loc.x
				 && dest_pt.y == GroupPtr->loc.y)
				|| Transition)
			{
				GroupPtr->flags |= CAN_TURN;
			}
		}
	}
	else if (task >= REFORM_GROUP && uqm::UQMOptions::read().shipDirectionIP)
	{ // To face sis while reforming
		if (GroupPtr->flags & ROTATES)
		{
			if (GetFrameIndex(EPtr->current.image.frame) == 1)
			{ // Define direction only once and not follow player while
				// reforming
				int16_t delta_x {};
				int16_t delta_y {};
				GFXPOINT sis_pt;

				sis_pt = displayToLocation(
					uqm::flagship().shipStamp.origin, radius);

				// Destination point is sis location
				delta_x = sis_pt.x - GroupPtr->loc.x;
				delta_y = sis_pt.y - GroupPtr->loc.y;

				EPtr->next.image.frame =
					SetAbsFrameIndex(EPtr->next.image.farray[0],
									 2 + NORMALIZE_FACING(ANGLE_TO_FACING(ARCTAN(delta_x, delta_y))));

				GroupPtr->flags |= CAN_TURN;
			}
		}
		else if (GroupPtr->flags & IS_PROBE)
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
	EPtr->next.location.x = DISPLAY_TO_WORLD(dest_pt.x)
						  + (int16_t)(LOG_SPACE_WIDTH >> 1)
						  - (LOG_SPACE_WIDTH >> (MAX_REDUCTION + 1));
	EPtr->next.location.y = DISPLAY_TO_WORLD(dest_pt.y)
						  + (int16_t)(LOG_SPACE_HEIGHT >> 1)
						  - (LOG_SPACE_HEIGHT >> (MAX_REDUCTION + 1));

	// Don't draw the group if it's not at flagship location,
	// or flash the group while it's reforming
	if (group_loc != flagship_loc
		|| ((task & REFORM_GROUP)
			&& (GroupPtr->group_counter & 1)))
	{
		SetPrimType(&DisplayArray[EPtr->PrimIndex], NO_PRIM);
		EPtr->state_flags |= NONSOLID;
	}
	else
	{
		SetPrimType(&DisplayArray[EPtr->PrimIndex], STAMP_PRIM);
		if (task & REFORM_GROUP)
		{
			EPtr->state_flags |= NONSOLID;
		}
	}

	EPtr->state_flags |= CHANGING;
}

static void
flag_ship_collision(ELEMENT* ElementPtr0, GFXPOINT* pPt0,
					ELEMENT* ElementPtr1, GFXPOINT* pPt1)
{
	if (uqm::currentActivity() & START_ENCOUNTER)
	{
		return; // ignore the rest of the collisions
	}

	if (!(ElementPtr1->state_flags & COLLISION))
	{ // The other element's collision has not been processed yet
		// Defer starting the encounter until it is.
		ElementPtr0->state_flags |= COLLISION | NONSOLID;
	}
	else
	{ // Both element's collisions have now been processed
		ElementPtr1->state_flags &= ~COLLISION;
		uqm::currentActivity() |= START_ENCOUNTER;
	}
	(void)pPt0; /* Satisfying compiler (unused parameter) */
	(void)pPt1; /* Satisfying compiler (unused parameter) */
}

static void
ip_group_collision(ELEMENT* ElementPtr0, GFXPOINT* pPt0,
				   ELEMENT* ElementPtr1, GFXPOINT* pPt1)
{
	IP_GROUP* GroupPtr {};
	void* OtherPtr {};

	if (uqm::currentActivity() & START_ENCOUNTER)
	{
		return; // ignore the rest of the collisions
	}

	*(&GroupPtr) = (IP_GROUP*)(ElementPtr0)->pParent;
	GetElementStarShip(ElementPtr1, &OtherPtr);

	if (!(uqm::flagship().autopilot.target().x == ~0 && uqm::flagship().autopilot.target().y == ~0)
		&& (CheckAlliance((RACE_ID)GroupPtr->race_id) == GOOD_GUY))
	{
		return; // Ignore collisions when allied during Auto-Pilot
	}

	if (OtherPtr)
	{ // Collision with another group
		// Prevent the groups from coalescing into a single ship icon
		if ((ElementPtr0->state_flags & COLLISION)
			|| (ElementPtr1->current.location.x == ElementPtr1->next.location.x
				&& ElementPtr1->current.location.y == ElementPtr1->next.location.y))
		{
			ElementPtr0->state_flags &= ~COLLISION;
		}
		else
		{
			ElementPtr1->state_flags |= COLLISION;

			GroupPtr->loc = DisplayArray[ElementPtr0->PrimIndex].Object.Point;
			ElementPtr0->next.location = ElementPtr0->current.location;
			InitIntersectEndPoint(ElementPtr0);
		}
	}
	else // if (!OtherPtr)
	{	 // Collision with a flagship
		EncounterGroup = GroupPtr->group_id;

		GroupPtr->task |= REFORM_GROUP;
		GroupPtr->group_counter = 100;
		// Send "all clear" for the time being. After the encounter, if
		// the player battles the group, the "intercept" notify will be
		// resent.
		NotifyOthers(GroupPtr->race_id, IPNL_ALL_CLEAR);

		if (!(ElementPtr1->state_flags & COLLISION))
		{ // The other element's collision has not been processed yet
			// Defer starting the encounter until it is.
			ElementPtr0->state_flags |= COLLISION | NONSOLID;
		}
		else
		{ // Both element's collisions have now been processed
			ElementPtr1->state_flags &= ~COLLISION;
			uqm::currentActivity() |= START_ENCOUNTER;
		}
	}
	(void)pPt0; /* Satisfying compiler (unused parameter) */
	(void)pPt1; /* Satisfying compiler (unused parameter) */
}

static void
spawn_ip_group(IP_GROUP* GroupPtr)
{
	HELEMENT hIPSHIPElement {AllocElement()};
	if (hIPSHIPElement)
	{
		ELEMENT* IPSHIPElementPtr {};

		LockElement(hIPSHIPElement, &IPSHIPElementPtr);
		// Must have mass_points for collisions to work
		IPSHIPElementPtr->mass_points = 1;
		IPSHIPElementPtr->hit_points = 1;
		IPSHIPElementPtr->state_flags =
			CHANGING | FINITE_LIFE | IGNORE_VELOCITY;

		SetPrimType(&DisplayArray[IPSHIPElementPtr->PrimIndex], STAMP_PRIM);
		// XXX: Hack: farray points to FRAME[3] and given FRAME
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
		if (GroupPtr->race_id != SLYLANDRO_SHIP)
		{
			GroupPtr->flags |= ROTATES;
		}
		if (GroupPtr->race_id == MMRNMHRM_SHIP)
		{
			GroupPtr->flags |= IS_XFORM;
		}
		if (GroupPtr->race_id == SLYLANDRO_SHIP)
		{
			GroupPtr->flags |= IS_PROBE;
		}
		if (uqm::UQMOptions::read().shipSeed)
		{
			const FLEET_INFO* TemplatePtr = nullptr;
			HFLEETINFO hFleet {GetStarShipFromIndex(&GLOBAL(avail_race_q), GroupPtr->race_id)};
			if (hFleet)
			{
				TemplatePtr = LockFleetInfo(&GLOBAL(avail_race_q), hFleet);
			}
			if (TemplatePtr)
			{
				const SPECIES_ID ship = SeedShip(TemplatePtr->SpeciesID, false);
				UnlockFleetInfo(&GLOBAL(avail_race_q), hFleet);
				GroupPtr->flags = 0;
				switch (ship)
				{
					case SLYLANDRO_ID:
						GroupPtr->flags |= IS_PROBE;
						break;
					case MMRNMHRM_ID:
						GroupPtr->flags |= IS_XFORM + ROTATES;
						break;
					case ORZ_ID:
					case UTWIG_ID:
					case YEHAT_ID:
					case MYCON_ID:
					case UR_QUAN_ID:
					case KOHR_AH_ID:
					case ARILOU_ID:
					case PKUNK_ID:
					case SPATHI_ID:
					case SUPOX_ID:
					case MELNORME_ID:
					case DRUUGE_ID:
					case SHOFIXTI_ID:
					case THRADDASH_ID:
					case VUX_ID:
					case ILWRATH_ID:
					case UMGAH_ID:
					case ZOQFOTPIK_ID:
					case CHMMR_ID:
					case CHENJESU_ID:
					case ANDROSYNTH_ID:
					case EARTHLING_ID:
					case SYREEN_ID:
						GroupPtr->flags |= ROTATES;
						break;
					default:
						break;
				}
			}
		}
		if (uqm::UQMOptions::read().shipDirectionIP && GroupPtr->flags & IS_PROBE)
		{
			GroupPtr->melee_icon =
				CaptureDrawable(
					LoadGraphic(SLYLANDRO_SML_MASK_PMAP_ANIM));
			IPSHIPElementPtr->current.image.frame = SetAbsFrameIndex(
				GroupPtr->melee_icon, (TFB_Random() % (15 - 1) + 1));
			// randomize initial sprite
		}
		IPSHIPElementPtr->death_func = ip_group_preprocess;
		IPSHIPElementPtr->collision_func = ip_group_collision;

		{
			GFXPOINT pt;

			const int16_t radius {zoomRadiusForLocation(GroupPtr->sys_loc)};
			pt = locationToDisplay(GroupPtr->loc, radius);

			IPSHIPElementPtr->current.location.x =
				DISPLAY_TO_WORLD(pt.x)
				+ (int16_t)(LOG_SPACE_WIDTH >> 1)
				- (LOG_SPACE_WIDTH >> (MAX_REDUCTION + 1));
			IPSHIPElementPtr->current.location.y =
				DISPLAY_TO_WORLD(pt.y)
				+ (int16_t)(LOG_SPACE_HEIGHT >> 1)
				- (LOG_SPACE_HEIGHT >> (MAX_REDUCTION + 1));
		}

		SetElementStarShip(IPSHIPElementPtr, GroupPtr);

		SetUpElement(IPSHIPElementPtr);
		IPSHIPElementPtr->IntersectControl.IntersectStamp.frame =
			DecFrameIndex(stars_in_space);

		UnlockElement(hIPSHIPElement);

		PutElement(hIPSHIPElement);
	}
}

#define FLIP_WAIT 42

static void
flag_ship_preprocess(ELEMENT* ElementPtr)
{
	if (--ElementPtr->thrust_wait == 0)
	/* juggle list after flagship */
	{
		HELEMENT hSuccElement {};

		if ((hSuccElement = GetSuccElement(ElementPtr))
			&& hSuccElement != GetTailElement())
		{
			ELEMENT* TailPtr {};

			LockElement(GetTailElement(), &TailPtr);
			HELEMENT hPredElement {_GetPredLink(TailPtr)};
			UnlockElement(GetTailElement());

			RemoveElement(hSuccElement);
			PutElement(hSuccElement);

			(void)hPredElement; // satisfy compiler (unused variable)
		}

		ElementPtr->thrust_wait = FLIP_WAIT;
	}

	{
		uint8_t ec {};
		int16_t vdx {};
		int16_t vdy {};
		GFXPOINT pt;

		GetCurrentVelocityComponents(&uqm::flagship().velocity, &vdx, &vdy);

		if (!legacySave)
		{
			vdx >>= 1;
			vdy >>= 1;
		}
		else if ((CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][KEY_UP]
				  || CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][KEY_THRUST])
				 || (vdx == 0 && vdy == 0))
		{
			legacySave = false;
		}

		const uint8_t flagship_loc {getFlagshipLocation()};
		const int16_t radius {zoomRadiusForLocation(flagship_loc)};
		adjustDeltaVforZoom(radius, &vdx, &vdy);

		pt = locationToDisplay(uqm::flagship().ipLocation, radius);
		ElementPtr->current.location.x = DISPLAY_TO_WORLD(pt.x)
									   + (int16_t)(LOG_SPACE_WIDTH >> 1)
									   - (LOG_SPACE_WIDTH >> (MAX_REDUCTION + 1));
		ElementPtr->current.location.y = DISPLAY_TO_WORLD(pt.y)
									   + (int16_t)(LOG_SPACE_HEIGHT >> 1)
									   - (LOG_SPACE_HEIGHT >> (MAX_REDUCTION + 1));
		InitIntersectStartPoint(ElementPtr);

		uqm::flagship().ipLocation.x += vdx;
		uqm::flagship().ipLocation.y += vdy;

		pt = locationToDisplay(uqm::flagship().ipLocation, radius);
		ElementPtr->next.location.x = DISPLAY_TO_WORLD(pt.x)
									+ (int16_t)(LOG_SPACE_WIDTH >> 1)
									- (LOG_SPACE_WIDTH >> (MAX_REDUCTION + 1));
		ElementPtr->next.location.y = DISPLAY_TO_WORLD(pt.y)
									+ (int16_t)(LOG_SPACE_HEIGHT >> 1)
									- (LOG_SPACE_HEIGHT >> (MAX_REDUCTION + 1));

		uqm::flagship().shipStamp.origin = pt;
		ElementPtr->next.image.frame = uqm::flagship().shipStamp.frame;

		if (ElementPtr->sys_loc == flagship_loc)
		{
			if (ElementPtr->state_flags & NONSOLID)
			{
				ElementPtr->state_flags &= ~NONSOLID;
			}
		}
		else /* no collisions during transition */
		{
			ElementPtr->state_flags |= NONSOLID;
			ElementPtr->sys_loc = flagship_loc;
		}

		if ((ec = GET_GAME_STATE(ESCAPE_COUNTER))
			&& !(uqm::currentActivity() & START_ENCOUNTER))
		{
			ElementPtr->state_flags |= NONSOLID;

			--ec;
			SET_GAME_STATE(ESCAPE_COUNTER, ec);
		}

		ElementPtr->state_flags |= CHANGING;
	}
}

static void
AdjustInitialPosition(void)
{ // Corrects SIS position - rounding error, described in
	// EnterPlanetOrbit() in solarsys.c
	GFXPOINT pt;

	const uint8_t flagship_loc {getFlagshipLocation()};
	const int16_t radius {zoomRadiusForLocation(flagship_loc)};

	pt = locationToDisplay(uqm::flagship().ipLocation, radius);

	if (LastActivity & CHECK_LOAD)
	{
		InitialIntersect();
	}

	uqm::flagship().shipStamp.origin = pt;
}

static void
spawn_flag_ship(void)
{
	HELEMENT hFlagShipElement {AllocElement()};
	if (hFlagShipElement)
	{
		ELEMENT* FlagShipElementPtr {};

		LockElement(hFlagShipElement, &FlagShipElementPtr);
		FlagShipElementPtr->hit_points = 1;
		// Must have mass_points for collisions to work
		FlagShipElementPtr->mass_points = 1;
		FlagShipElementPtr->sys_loc = getFlagshipLocation();
		FlagShipElementPtr->state_flags = APPEARING | IGNORE_VELOCITY;
		if (GET_GAME_STATE(ESCAPE_COUNTER))
		{
			FlagShipElementPtr->state_flags |= NONSOLID;
		}
		FlagShipElementPtr->life_span = NORMAL_LIFE;
		FlagShipElementPtr->thrust_wait = FLIP_WAIT;
		SetPrimType(&DisplayArray[FlagShipElementPtr->PrimIndex], STAMP_PRIM);
		FlagShipElementPtr->current.image.farray =
			&uqm::flagship().shipStamp.frame;
		FlagShipElementPtr->current.image.frame =
			uqm::flagship().shipStamp.frame;
		FlagShipElementPtr->preprocess_func = flag_ship_preprocess;
		FlagShipElementPtr->collision_func = flag_ship_collision;

		AdjustInitialPosition();

		FlagShipElementPtr->current.location.x =
			DISPLAY_TO_WORLD(uqm::flagship().shipStamp.origin.x)
			+ (int16_t)(LOG_SPACE_WIDTH >> 1)
			- (LOG_SPACE_WIDTH >> (MAX_REDUCTION + 1));
		FlagShipElementPtr->current.location.y =
			DISPLAY_TO_WORLD(uqm::flagship().shipStamp.origin.y)
			+ (int16_t)(LOG_SPACE_HEIGHT >> 1)
			- (LOG_SPACE_HEIGHT >> (MAX_REDUCTION + 1));

		UnlockElement(hFlagShipElement);

		PutElement(hFlagShipElement);
	}
}

void DoMissions(void)
{
	HSHIPFRAG hGroup {};
	HSHIPFRAG hNextGroup {};

	spawn_flag_ship();

	if (EncounterRace >= 0)
	{ // There was a battle. Call in reinforcements.
		NotifyOthers(EncounterRace, IPNL_INTERCEPT_PLAYER);
		EncounterRace = -1;
	}

	for (hGroup = GetHeadLink(&GLOBAL(ip_group_q));
		 hGroup; hGroup = hNextGroup)
	{
		IP_GROUP* GroupPtr {LockIpGroup(&GLOBAL(ip_group_q), hGroup)};
		hNextGroup = _GetSuccLink(GroupPtr);

		if (GroupPtr->in_system
			&& CheckAlliance((RACE_ID)GroupPtr->race_id) != DEAD_GUY)
		{
			spawn_ip_group(GroupPtr);
		}

		UnlockIpGroup(&GLOBAL(ip_group_q), hGroup);
	}
}
