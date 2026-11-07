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

#include "hyper.h"

#include "build.h"
#include "collide.h"
#include "colors.h"
#include "controls.h"
#include "core/types/enum.h"
#include "gameopt.h"
#include "menustat.h"
// for DrawMenuStateStrings()
#include "core/log/log.h"
#include "encount.h"
#include "globdata.h"
#include "init.h"
#include "libs/gfxlib.h"
#include "libs/graphics/drawable.h"
#include "libs/graphics/gfx_common.h"
#include "libs/mathlib.h"
#include "nameref.h"
#include "options.h"
#include "process.h"
#include "resinst.h"
#include "setup.h"
#include "ship.h"
#include "shipcont.h"
#include "sis.h"
#include "sounds.h"
#include "starmap.h"
#include "units.h"
#include "uqm/random/UQMRandom.h"
#include "util.h"

namespace uqm
{
static auto Xoffs() { return ((RadarScanWidth() + (UnitScreenWidth() << 2)) >> 1); }
static auto Yoffs() { return ((RadarScanHeight() + (UnitScreenHeight() << 2)) >> 1); }
static auto AnimatedHyperspace() { return (IsHd() && uqm::UQMOptions::read().hyperStars); }

static uqstl::array<FrameRef, 3> hyperstars;
static uqstl::array<ColorMapRef, 2> hypercmaps;
static uint8_t fuel_ticks;
static uint16_t hyper_dx, hyper_dy, hyper_extra;

// HyperspaceMenu() items
enum class HyperMenuItem
{
	// XXX: Must match the enum in menustat.h
	Starmap = 1,
	EquipDevice,
	Cargo,
	Roster,
	GameMenu,
	Navigation,
};
consteval bool isArithmeticEnum(HyperMenuItem) { return true; }

static inline int16_t RaceHyperSpeed(RaceId Index)
{
	static int seedStamp = -1;
	const uqstl::array<int, 26> defaultMap {
		LogToHyper(ResScale(40)), /* ARILOU_SHIP */
		LogToHyper(ResScale(27)), /* CHMMR_SHIP */
		LogToHyper(ResScale(24)), /* HUMAN_SHIP */
		LogToHyper(ResScale(40)), /* ORZ_SHIP */
		LogToHyper(ResScale(40)), /* PKUNK_SHIP */
		LogToHyper(ResScale(35)), /* SHOFIXTI_SHIP */
		LogToHyper(ResScale(48)), /* SPATHI_SHIP */
		LogToHyper(ResScale(40)), /* SUPOX_SHIP */
		LogToHyper(ResScale(50)), /* THRADDASH_SHIP */
		LogToHyper(ResScale(30)), /* UTWIG_SHIP */
		LogToHyper(ResScale(21)), /* VUX_SHIP */
		LogToHyper(ResScale(30)), /* YEHAT_SHIP */
		LogToHyper(ResScale(40)), /* MELNORME_SHIP */
		LogToHyper(ResScale(20)), /* DRUUGE_SHIP */
		LogToHyper(ResScale(25)), /* ILWRATH_SHIP */
		LogToHyper(ResScale(27)), /* MYCON_SHIP */
		LogToHyper(ResScale(60)), /* SLYLANDRO_SHIP */
		LogToHyper(ResScale(18)), /* UMGAH_SHIP */
		LogToHyper(ResScale(30)), /* URQUAN_SHIP */
		LogToHyper(ResScale(40)), /* ZOQFOTPIK_SHIP */
		LogToHyper(ResScale(36)), /* SYREEN_SHIP */
		LogToHyper(ResScale(30)), /* BLACK_URQUAN_SHIP */
		LogToHyper(ResScale(45)), /* ANDROSYNTH_SHIP */
		LogToHyper(ResScale(27)), /* CHENJESU_SHIP */
		LogToHyper(ResScale(50)), /* MMRNMHRM_SHIP */
		LogToHyper(ResScale(30)), /* YEHAT_REBEL_SHIP */

	};
	const uint16_t numRaces = uqstl::size(defaultMap);
	static uqstl::array<int16_t, numRaces> speedMap {}; // set urquan drone to 0.
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
/*
 * draws the melee icon for the battle group inside the black holes,
 * so you can see who's chasing you.
 */
/*static void
decorate_vortex (Element *ElementPtr)
{
	EncounterHandle hEncounter, hNextEncounter;
	FrameRef f = nullptr;
	static uqstl::array<FrameRef, NUM_AVAILABLE_RACES> vortex_ships;

	// The Element is still spawning, nothing to do yet
	if (ElementPtr->death_func)
		return;

	// The Element doesn't know what kind of ship it is, that
	// info is stored in the Encounter queue.  I'm guessing this
	// needs refactoring
	for (hEncounter = GetHeadEncounter ();
		hEncounter != 0; hEncounter = hNextEncounter)
	{
		Encounter *EncounterPtr;

		LockEncounter (hEncounter, &EncounterPtr);
		hNextEncounter = GetSuccEncounter (EncounterPtr);
		if (EncounterPtr->hElement)
		{
			Element *EncounterElementPtr;

			LockElement (EncounterPtr->hElement, &EncounterElementPtr);
			if (EncounterElementPtr == ElementPtr)
			{
				FleetInfoHandle hFleet;
				FleetInfo *FleetPtr;

				if (vortex_ships[EncounterPtr->race_id])
				{
					if (ElementPtr->next.image.frame !=
							vortex_ships[EncounterPtr->race_id])
						ElementPtr->next.image.frame =
							vortex_ships[EncounterPtr->race_id];
				}
				else
				{
					hFleet = GetStarShipFromIndex (&GLOBAL (avail_race_q),
						EncounterPtr->race_id);
					if (hFleet)
					{
						FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q),
							hFleet);
						f = SetAbsFrameIndex (FleetPtr->melee_icon, 1);
						UnlockFleetInfo (&GLOBAL (avail_race_q), hFleet);
					}

					// now make a frame, and use a context to scribble
					// into it with DrawStamp().  uses a static array to
					// reuse generated frames for the life of the game
					// (or multiple games) as a dodge around figuring out
					// a sensible memory management strategy  ;)
					if (f)
					{
						GfxContextRef tmp, old;
						Color trans;
						GfxStamp s;

						vortex_ships[EncounterPtr->race_id] =
								CaptureDrawable (
									CreateDrawable (
										CreateFlags::WantPixmap | CreateFlags::WantAlpha,
										GetFrameWidth (
											ElementPtr->next.image.frame),
										GetFrameHeight (
											ElementPtr->next.image.frame),
										1)
									);
						tmp = CreateContext ("HyperSpaceContext");
						old = SetContext (tmp);
						SetContextFGFrame (
								vortex_ships[EncounterPtr->race_id]);
						trans = BuildColor (
								MakeRgb15 (0x10, 0x00, 0x10), 0x00);
						SetContextBackGroundColor (trans);
						ClearDrawable ();
						SetFrameTransparentColor (
								vortex_ships[EncounterPtr->race_id], trans
							);

						// the original Element
						s.frame = ElementPtr->current.image.frame;
						s.origin = GetFrameHot (s.frame);
						DrawStamp (&s);

						// the overlaid gfx
						s.frame = f;
						DrawStamp (&s);

						// important to make sure the
						// collision animation looks correct
						SetFrameHot (vortex_ships[EncounterPtr->race_id],
								s.origin);

						// cleanup
						SetContext (old);
						DestroyContext (tmp);
						// vortex_ships[EncounterPtr->race_id]->parent =
						//		ElementPtr->current.image.frame->parent;
						// ElementPtr->next.image.frame =
						//		vortex_ships[EncounterPtr->race_id];
					}
				}
			}
			UnlockElement (EncounterPtr->hElement);
		}
		UnlockEncounter (hEncounter);
	}
}*/

void MoveSIS(int32_t* pdx, int32_t* pdy)
{
	int32_t new_dx {*pdx};
	uqm::gameState().sis().log_x -= new_dx;
	if (uqm::gameState().sis().log_x < 0)
	{
		new_dx += (int16_t)uqm::gameState().sis().log_x;
		uqm::gameState().sis().log_x = 0;
	}
	else if (uqm::gameState().sis().log_x > MaxXLogical())
	{
		new_dx += (int16_t)(uqm::gameState().sis().log_x - MaxXLogical());
		uqm::gameState().sis().log_x = MaxXLogical();
	}

	int32_t new_dy {*pdy};
	uqm::gameState().sis().log_y -= new_dy;
	if (uqm::gameState().sis().log_y < 0)
	{
		new_dy += (int16_t)uqm::gameState().sis().log_y;
		uqm::gameState().sis().log_y = 0;
	}
	else if (uqm::gameState().sis().log_y > MaxYLogical())
	{
		new_dy += (int16_t)(uqm::gameState().sis().log_y - MaxYLogical());
		uqm::gameState().sis().log_y = MaxYLogical();
	}

	if (new_dx != *pdx || new_dy != *pdy)
	{
		ElementHandle hElement {};
		ElementHandle hNextElement {};

		*pdx = new_dx;
		*pdy = new_dy;

		for (hElement = GetTailElement();
			 hElement != 0; hElement = hNextElement)
		{
			Element* ElementPtr {};

			LockElement(hElement, &ElementPtr);

			if (!(testFlag(ElementPtr->state_flags, ElementFlags::PlayerShip)))
			{
				hNextElement = GetPredElement(ElementPtr);
			}
			else
			{
				ElementPtr->next.location.x = (LogSpaceWidth() >> 1)
											- new_dx;
				ElementPtr->next.location.y = (LogSpaceHeight() >> 1)
											- new_dy;
				hNextElement = 0;
			}

			UnlockElement(hElement);
		}
	}

	if (uqm::gameState().sis().FuelOnBoard
		&& uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) <= 1)
	{
		uint16_t cur_fuel_ticks {};
		uint16_t hyper_dist {};
		uint32_t adj_dx {};
		uint32_t adj_dy {};

		if (new_dx < 0)
		{
			new_dx = -new_dx;
		}
		hyper_dx += new_dx;
		if (new_dy < 0)
		{
			new_dy = -new_dy;
		}
		hyper_dy += new_dy;

		// These macros are also used in the fuel estimate on the starmap.
		adj_dx = LogxToUniverse(16 * hyper_dx);
		adj_dy = MaxYUniverse - LogyToUniverse(16 * hyper_dy);

		hyper_dist = square_root(adj_dx * adj_dx + adj_dy * adj_dy)
				   + hyper_extra;
		cur_fuel_ticks = hyper_dist >> 4;

		if (cur_fuel_ticks > (uint16_t)fuel_ticks)
		{
			if (!uqm::UQMOptions::read().infiniteFuel)
			{
				DeltaSISGauges(0, fuel_ticks - cur_fuel_ticks, 0);
			}

			if (cur_fuel_ticks > 0x00FF)
			{
				hyper_dx = 0;
				hyper_extra = hyper_dist & ((1 << 4) - 1);
				hyper_dy = 0;
				cur_fuel_ticks = 0;
			}

			fuel_ticks = (uint8_t)cur_fuel_ticks;
		}
	}
}

void check_hyperspace_encounter(void)
{
	uint8_t Type {};
	GfxPoint universe;
	FleetInfoHandle hStarShip {};
	FleetInfoHandle hNextShip {};
	const auto& EncounterPercent {RaceHyperspacePercent};

	universe.x = LogxToUniverse(uqm::gameState().sis().log_x);
	universe.y = LogyToUniverse(uqm::gameState().sis().log_y);
	for (hStarShip = GetHeadLink(&uqm::gameState().game().avail_race_q), Type = 0;
		 hStarShip && (uqm::currentActivity() & ActivityFlag::InBattle);
		 hStarShip = hNextShip, ++Type)
	{
		const FleetInfo* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip)};
		hNextShip = _GetSuccLink(FleetPtr);

		uint16_t encounter_radius {FleetPtr->actual_strength};
		if (encounter_radius)
		{
			int16_t dx {};
			int16_t dy {};
			EncounterHandle hEncounter {};
			Encounter* EncounterPtr {};

			uint8_t encounter_flags {0};
			uint16_t percent {EncounterPercent[Type]};

			if (uqm::UQMOptions::read().noHQEncounters)
			{
				percent = 0;
			}

			if (encounter_radius != InfiniteRadius)
			{
				encounter_radius =
					(encounter_radius * SphereRadiusIncrement) >> 1;

				if (IsExtended() && uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY)
					&& Type == RaceId::BlackUrquan
					&& uqm::gameState().flag(uqm::GameFlag::USED_BROADCASTER))
				{
					encounter_radius = (MaxXUniverse + 1) << 1;
					percent = 100;
					encounter_flags = OneShotEncounter;

					uqm::gameState().setFlag(uqm::GameFlag::BROADCASTER_RESPONSE, static_cast<uint32_t>(1));
				}
			}
			else /* encounter_radius == infinity */
			{
				EncounterHandle hNextEncounter {};

				encounter_radius = (MaxXUniverse + 1) << 1;
				if (Type == RaceId::Slylandro)
				{
					encounter_flags = OneShotEncounter;
					if (!uqm::gameState().flag(uqm::GameFlag::STARBASE_AVAILABLE))
					{
						percent = 100;
					}
					else
					{
						percent *= uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_MULTIPLIER);
					}

					if (uqm::UQMOptions::read().noHQEncounters)
					{
						percent = 0;
					}
				}

				if ((IsExtended() && !uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY))
					|| !IsExtended())
				{
					if (Type == RaceId::Melnorme
						&& (uqm::gameState().sis().FuelOnBoard == 0
							|| uqm::gameState().flag(uqm::GameFlag::USED_BROADCASTER))
						&& uqm::gameState().flag(uqm::GameFlag::MELNORME_ANGER) < 3)
					{
						if (!uqm::gameState().flag(uqm::GameFlag::USED_BROADCASTER))
						{
							percent = 30;
						}
						else
						{
							percent = 100;
						}
						encounter_flags = OneShotEncounter;
					}
				}

				// There can be only one! (of either Slylandro or Melnorme)
				for (hEncounter = GetHeadEncounter();
					 hEncounter; hEncounter = hNextEncounter)
				{
					LockEncounter(hEncounter, &EncounterPtr);
					hNextEncounter = GetSuccEncounter(EncounterPtr);
					if (EncounterPtr->race_id == Type)
					{
						percent = 0;
						hNextEncounter = 0;
					}
					UnlockEncounter(hEncounter);
				}


				if (percent == 100 && Type == RaceId::Melnorme)
				{
					uqm::gameState().setFlag(uqm::GameFlag::BROADCASTER_RESPONSE, static_cast<uint32_t>(1));
				}
			}

			dx = universe.x - FleetPtr->loc.x;
			if (dx < 0)
			{
				dx = -dx;
			}
			dy = universe.y - FleetPtr->loc.y;
			if (dy < 0)
			{
				dy = -dy;
			}
			if ((uint16_t)dx < encounter_radius
				&& (uint16_t)dy < encounter_radius
				&& (uint32_t)dx * dx + (uint32_t)dy * dy < (uint32_t)encounter_radius * encounter_radius
				&& ((uint16_t)uqm::random().next() % 100) < percent)
			{
				// Ship spawned for Encounter.
				hEncounter = AllocEncounter();
				if (hEncounter)
				{
					LockEncounter(hEncounter, &EncounterPtr);
					memset(EncounterPtr, 0, sizeof(*EncounterPtr));
					EncounterPtr->origin = FleetPtr->loc;
					EncounterPtr->radius = encounter_radius;
					EncounterPtr->flags = encounter_flags;
					EncounterPtr->race_id = Type;

					UnlockEncounter(hEncounter);

					PutEncounter(hEncounter);
				}
			}
		}

		UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
	}

	uqm::gameState().setFlag(uqm::GameFlag::USED_BROADCASTER, static_cast<uint32_t>(0));
}

void FreeHyperData(void)
{
	DestroyDrawable(ReleaseDrawable(hyperstars[0]));
	hyperstars[0] = 0;
	DestroyDrawable(ReleaseDrawable(hyperstars[1]));
	hyperstars[1] = 0;
	DestroyDrawable(ReleaseDrawable(hyperstars[2]));
	hyperstars[2] = 0;

	delete hypercmaps[0];
	hypercmaps[0] = 0;
	delete hypercmaps[1];
	hypercmaps[1] = 0;
}

static auto ArispaceMask() { return (AnimatedHyperspace() ? ArianimMaskPmapAnimRes : ArispaceMaskPmapAnimRes); }
static auto HyperstarsMask() { return (AnimatedHyperspace() ? HyperanimMaskPmapAnimRes : HyperstarsMaskPmapAnimRes); }

static void LoadHyperData(void)
{
	if (hyperstars[0] == 0)
	{
		hyperstars[0] = CaptureDrawable(
			LoadGraphic(AmbientMaskPmapAnimRes));
		hyperstars[1] = CaptureDrawable(
			LoadGraphic(HyperstarsMask()));
		hypercmaps[0] = uqm::loadStringTable(HyperColorTabRes).release();

		hyperstars[2] = CaptureDrawable(
			LoadGraphic(ArispaceMask()));
		hypercmaps[1] = uqm::loadStringTable(ArispaceColorTabRes).release();
	}
}

bool LoadHyperspace(void)
{
	hyper_dx = 0;
	hyper_dy = 0;
	hyper_extra = 0;
	fuel_ticks = 1;

	uqm::flagship().shipStamp.origin.x = -MaxXUniverse;
	uqm::flagship().shipStamp.origin.y = -MaxYUniverse;

	LoadHyperData();
	{
		FrameRef F {hyperstars[0]};
		hyperstars[0] = stars_in_space;
		stars_in_space = F;
	}

	if (!(LastActivity & ActivityFlag::CheckLoad))
	{
		RepairSISBorder();
	}
	else
	{
		if (uqm::activityKind(LastActivity) == 0)
		{
			DrawSISFrame();
		}
		else
		{
			ClearSISRect(ClearSisFlags::DrawDisplay);
			RepairSISBorder();
		}
	}
	if (!(uqm::flagship().autopilot.hasTarget()))
	{
		DrawSISMessage(nullptr);
	}

	SetContext(RadarContext);
	SetContextBackGroundColor(
		BuildColor(MakeRgb15(0x00, 0x0E, 0x00), 0x6C));

	SetContext(SpaceContext);
	if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) <= 1)
	{ // Hyperspace Color
		SetContextBackGroundColor(
			BuildColor(MakeRgb15(0x07, 0x00, 0x00), 0x2F));
		SetColorMap(GetColorMapAddress(hypercmaps[0], 0));
	}
	else
	{ // Quasispace Color
		SetContextBackGroundColor(
			BuildColor(MakeRgb15(0x00, 0x1A, 0x00), 0x2F));
		SetColorMap(GetColorMapAddress(hypercmaps[1], 0));
		uqm::gameState().setFlag(uqm::GameFlag::USED_BROADCASTER, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::BROADCASTER_RESPONSE, static_cast<uint32_t>(0));
	}
	//    ClearDrawable ();

	return true;
}

void EraseRadar(void)
{
	BatchGraphics();
	ClearSISRect(ClearSisFlags::ClearRadar);
	SeedUniverse();
	UnbatchGraphics();
}

bool FreeHyperspace(void)
{
	{
		FrameRef F {hyperstars[0]};
		hyperstars[0] = stars_in_space;
		stars_in_space = F;
	}
	// Kruzen: To load correct set of frames until we'll use 1 with space filters
	//if (IsHd())
	//	ReloadHyperSpace ();

	//FreeHyperData ();

	return true;
}

static void ElementToUniverse(Element* ElementPtr, GfxPoint* pPt)
{
	int32_t log_x {};
	int32_t log_y {};

	log_x = uqm::gameState().sis().log_x
		  + (ElementPtr->next.location.x - (LogSpaceWidth() >> 1));
	log_y = uqm::gameState().sis().log_y
		  + (ElementPtr->next.location.y - (LogSpaceHeight() >> 1));
	pPt->x = LogxToUniverse(log_x);
	pPt->y = LogyToUniverse(log_y);
}

static void cleanup_hyperspace(void)
{
	EncounterHandle hEncounter {};
	EncounterHandle hNextEncounter {};

	for (hEncounter = GetHeadEncounter();
		 hEncounter != 0; hEncounter = hNextEncounter)
	{
		Encounter* EncounterPtr {};

		LockEncounter(hEncounter, &EncounterPtr);
		hNextEncounter = GetSuccEncounter(EncounterPtr);
		if (EncounterPtr->hElement)
		{
			Element* ElementPtr {};

			LockElement(EncounterPtr->hElement, &ElementPtr);

			if (ElementPtr->hTarget)
			{ // This is the Encounter that collided with flagship
				// Move the Encounter to the head of the queue so that
				// comm.c:RaceCommunication() gets the right one.
				RemoveEncounter(hEncounter);
				InsertEncounter(hEncounter, GetHeadEncounter());
			}

			UnlockElement(EncounterPtr->hElement);
		}
		EncounterPtr->hElement = 0;
		UnlockEncounter(hEncounter);
	}
}

enum class TransitionType
{
	RandomEncounter,
	Interplanetary,
	ArilouSpace
};

static void InterplanetaryTransition(Element* ElementPtr)
{
	uqm::flagship().ipPlanet = 0;
	uqm::flagship().inOrbit = 0;
	uqm::flagship().shipFacing = 0; /* Not reentering the system */
	uqm::gameState().setFlag(uqm::GameFlag::USED_BROADCASTER, static_cast<uint32_t>(0));
	if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) <= 1)
	{
		// Enter a solar system from HyperSpace.
		uqm::currentActivity() |= ActivityFlag::StartInterplanetary;
		uqm::gameState().setFlag(uqm::GameFlag::ESCAPE_COUNTER, static_cast<uint32_t>(0));
	}
	else
	{
		GfxPoint pt;
		uint16_t KnownQSPortals = uqm::gameState().flag(uqm::GameFlag::KNOW_QS_PORTAL);

		uqm::flagship().autopilot.clear();

		ElementToUniverse(ElementPtr, &pt);
		CurStarDescPtr() = FindStar(nullptr, &pt, 5, 5);

		// JMS: Debugging helpers
		/*{
			StarDesc *SDPtr, *SDPtr2;
			SDPtr = CurStarDescPtr();
			SDPtr2 = FindStar (nullptr, &pt, 500, 500);
			uqm::log::debug( "SDPtr.x {}, SDPtr.y {} SDPtr2.x {}, "
					"SDPtr2.y {}, pt.x {} pt.y {}", 
					SDPtr->star_pt.x, SDPtr->star_pt.y, SDPtr2->star_pt.x,
					SDPtr2->star_pt.y, pt.x, pt.y);
		}*/

		if (CurStarDescPtr()->star_pt.x == ArilouHomeX
			&& CurStarDescPtr()->star_pt.y == ArilouHomeY)
		{
			// Meet the Arilou.
			uqm::currentActivity() |= ActivityFlag::StartEncounter;

			// JMS: The arilou homeworld name can now be shown on QS map.
			KnownQSPortals |= (1 << 15);
			uqm::gameState().setFlag(uqm::GameFlag::KNOW_QS_PORTAL, static_cast<uint32_t>(KnownQSPortals));
		}
		else
		{
			// Transition from QuasiSpace to HyperSpace through
			// one of the permanent portals.
			uint16_t index {};
			// JSD - portal_map can be used to find the exit point
			//const GfxPoint portal_pt[] =
			//		QUASISPACE_PORTALS_HYPERSPACE_ENDPOINTS;

			index = CurStarDescPtr() - &star_array[NumSolarSystems + 1];
			//GLOBAL_SIS(log_x) = UniverseToLogx(portal_pt[index].x);
			//GLOBAL_SIS(log_y) = UniverseToLogy(portal_pt[index].y);
			uqm::gameState().sis().log_x = UniverseToLogx(portal_map[index].star_pt.x);
			uqm::gameState().sis().log_y = UniverseToLogy(portal_map[index].star_pt.y);

			// JMS: This QS portal's HS coordinates are revealed on QS map
			// the next time the player visits QS.
			KnownQSPortals |= (1 << index);
			uqm::gameState().setFlag(uqm::GameFlag::KNOW_QS_PORTAL, static_cast<uint32_t>(KnownQSPortals));

			uqm::gameState().setFlag(uqm::GameFlag::ARILOU_SPACE_SIDE, static_cast<uint32_t>(0));
		}
	}
}

/* Enter QuasiSpace from HyperSpace by any portal, or HyperSpace from
 * QuasiSpace through the periodically opening portal.
 */
static void ArilouSpaceTransition(void)
{
	uqm::flagship().shipFacing = 0; /* Not reentering the system */
	uqm::gameState().setFlag(uqm::GameFlag::USED_BROADCASTER, static_cast<uint32_t>(0));
	uqm::flagship().autopilot.clear();
	if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) <= 1)
	{
		// From HyperSpace to QuasiSpace.
		uqm::gameState().sis().log_x = UniverseToLogx(QuasiSpaceX);
		uqm::gameState().sis().log_y = UniverseToLogy(QuasiSpaceY);
		if (uqm::gameState().flag(uqm::GameFlag::PORTAL_COUNTER) == 0)
		{
			// Periodically appearing portal.
			uqm::gameState().setFlag(uqm::GameFlag::ARILOU_SPACE_SIDE, static_cast<uint32_t>(3));

			if (IsExtended())
			{
				ZeroLastLoc();
			}
		}
		else
		{
			// Player-induced portal.
			uqm::gameState().setFlag(uqm::GameFlag::PORTAL_COUNTER, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::ARILOU_SPACE_SIDE, static_cast<uint32_t>(3));
		}
	}
	else
	{
		// From QuasiSpace to HyperSpace through the periodically appearing
		// portal.
		if (!IsExtended() || (IsExtended() && !ValidPoint(LoadLastLoc())))
		{
			// JSD Converting hard coded ARILOU portal to plot based
			//GLOBAL_SIS (log_x) = UniverseToLogx (ArilouSpaceX);
			//GLOBAL_SIS (log_y) = UniverseToLogy (ArilouSpaceY);
			uqm::gameState().sis().log_x = UniverseToLogx(plot_map[*PlotId::Arilou].star_pt.x);
			uqm::gameState().sis().log_y = UniverseToLogy(plot_map[*PlotId::Arilou].star_pt.y);
		}
		else if (IsExtended() && ValidPoint(LoadLastLoc()))
		{
			uqm::gameState().sis().log_x = UniverseToLogx(LoadLastLoc().x);
			uqm::gameState().sis().log_y = UniverseToLogy(LoadLastLoc().y);

			ZeroLastLoc();
		}
		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_SPACE_SIDE, static_cast<uint32_t>(0));
	}
}

static void unhyper_transition(Element* ElementPtr)
{
	// JMS: If leaving interplanetary on autopilot, always arrive HS with
	// the ship's nose pointed into correct direction.
	if (uqm::UQMOptions::read().smartAutoPilot
		&& (uqm::flagship().autopilot.hasTarget()))
	{
		Starship* StarShipPtr {};
		GfxPoint universe;
		int16_t facing {};
		int32_t udx = 0;
		int32_t udy = 0;

		GetElementStarShip(ElementPtr, &StarShipPtr);
		universe.x = LogxToUniverse(uqm::gameState().sis().log_x);
		universe.y = LogyToUniverse(uqm::gameState().sis().log_y);
		udx = uqm::flagship().autopilot.target().x - universe.x;
		udy = -(uqm::flagship().autopilot.target().y - universe.y);

		facing = NormalizeFacing(AngleToFacing(ARCTAN(udx, udy)));
		StarShipPtr->ShipFacing = facing;
		SetElementStarShip(ElementPtr, StarShipPtr);
	}

	ElementPtr->state_flags |= ElementFlags::Changing;

	uint16_t frame_index {GetFrameIndex(ElementPtr->current.image.frame)};
	if (frame_index == 0)
	{
		frame_index += AngleToFacing(FullCircle);
	}
	else if (frame_index < AngleToFacing(FullCircle))
	{
		frame_index = NormalizeFacing(frame_index + 1);
	}
	else if (++frame_index == GetFrameCount(ElementPtr->current.image.frame))
	{
		cleanup_hyperspace();

		uqm::currentActivity() &= ~ActivityFlag::InBattle;
		switch ((TransitionType)ElementPtr->turn_wait)
		{
			case TransitionType::RandomEncounter:
				SaveSisHyperState();
				SetBattlePlanet();
				uqm::currentActivity() |= ActivityFlag::StartEncounter;
				break;
			case TransitionType::Interplanetary:
				InterplanetaryTransition(ElementPtr);
				break;
			case TransitionType::ArilouSpace:
				ArilouSpaceTransition();
				break;
		}

		ZeroVelocityComponents(&ElementPtr->velocity);
		SetPrimType(&DisplayArray[ElementPtr->PrimIndex], NoPrim);
		return;
	}
	ElementPtr->next.image.frame =
		SetAbsFrameIndex(ElementPtr->current.image.frame,
						 frame_index);
}

static void init_transition(Element* ElementPtr0, Element* ElementPtr1,
							TransitionType which_transition)
{
	int16_t dx {};
	int16_t dy {};
	int16_t num_turns {};
	Starship* StarShipPtr {};

	dx = WorldToVelocity(ElementPtr0->next.location.x
						 - ElementPtr1->next.location.x);
	dy = WorldToVelocity(ElementPtr0->next.location.y
						 - ElementPtr1->next.location.y);

	ElementPtr1->state_flags |= ElementFlags::Nonsolid;
	ElementPtr1->preprocess_func = unhyper_transition;
	ElementPtr1->postprocess_func = nullptr;
	ElementPtr1->turn_wait = (uint8_t)which_transition;

	GetElementStarShip(ElementPtr1, &StarShipPtr);
	num_turns = GetFrameCount(ElementPtr1->next.image.frame)
			  - AngleToFacing(FullCircle)
			  + NormalizeFacing(AngleToFacing(FullCircle)
								- StarShipPtr->ShipFacing);
	if (num_turns == 0)
	{
		num_turns = 1;
	}

	SetVelocityComponents(&ElementPtr1->velocity,
						  dx / num_turns, dy / num_turns);
}

void DoAdvancedAutoPilot(void)
{
	const GfxPoint QuasiPilot = LoadAdvancedQuasiPilot();
	const GfxPoint SavedPilot = LoadAdvancedAutoPilot();

	if (!ValidPoint(SavedPilot))
	{
		return;
	}

	if (inHyperSpace())
	{
		if (ValidPoint(QuasiPilot)
			&& ValidPoint(uqm::flagship().autopilot.target()))
		{
			InvokeSpawner();
		}
		else
		{
			uqm::flagship().autopilot.setTarget(SavedPilot);
			ZeroAdvancedAutoPilot();
		}

		return;
	}

	if (!ValidPoint(QuasiPilot))
	{
		return;
	}

	uqm::flagship().autopilot.setTarget(QuasiPilot);
	ZeroAdvancedQuasiPilot();
}

bool hyper_transition(Element* ElementPtr)
{
	if (testFlag(ElementPtr->state_flags, ElementFlags::Appearing))
	{
		if (LastActivity & ActivityFlag::CheckLoad)
		{
			LastActivity &= ~ActivityFlag::CheckLoad;

			ElementPtr->current = ElementPtr->next;
			SetUpElement(ElementPtr);

			ElementPtr->state_flags |= ElementFlags::DefyPhysics;

			return false;
		}
		else
		{
			ElementPtr->preprocess_func =
				(void (*)(struct Element* ElementPtr))hyper_transition;
			ElementPtr->postprocess_func = nullptr;
			ElementPtr->state_flags |= ElementFlags::Nonsolid;
			ElementPtr->next.image.frame =
				SetAbsFrameIndex(ElementPtr->current.image.frame,
								 GetFrameCount(ElementPtr->current.image.frame) - 1);
		}
	}
	else
	{
		// JMS: If leaving interplanetary on autopilot, always arrive HS
		// with the ship's nose pointed into correct direction.
		if (uqm::UQMOptions::read().smartAutoPilot && ValidPoint(uqm::flagship().autopilot.target()))
		{
			Starship* StarShipPtr {};
			GfxPoint universe;
			int16_t facing {};
			int32_t udx = 0;
			int32_t udy = 0;

			GetElementStarShip(ElementPtr, &StarShipPtr);
			universe.x = LogxToUniverse(uqm::gameState().sis().log_x);
			universe.y = LogyToUniverse(uqm::gameState().sis().log_y);
			udx = uqm::flagship().autopilot.target().x - universe.x;
			udy = -(uqm::flagship().autopilot.target().y - universe.y);

			facing =
				NormalizeFacing(AngleToFacing(ARCTAN(udx, udy)));
			StarShipPtr->ShipFacing = facing;
			SetElementStarShip(ElementPtr, StarShipPtr);
		}

		uint16_t frame_index {GetFrameIndex(ElementPtr->current.image.frame)};
		if (frame_index-- <= AngleToFacing(FullCircle))
		{
			Starship* StarShipPtr {};

			if (frame_index == AngleToFacing(FullCircle) - 1)
			{
				frame_index = 0;
			}
			else
			{
				frame_index = NormalizeFacing(frame_index);
			}

			GetElementStarShip(ElementPtr, &StarShipPtr);
			if (frame_index == StarShipPtr->ShipFacing)
			{
				ElementPtr->preprocess_func = ship_preprocess;
				ElementPtr->postprocess_func = ship_postprocess;
				ElementPtr->state_flags &= ~ElementFlags::Nonsolid;
			}
		}

		ElementPtr->state_flags |= ElementFlags::Changing;
		ElementPtr->next.image.frame =
			SetAbsFrameIndex(ElementPtr->current.image.frame,
							 frame_index);

		if (!(testFlag(ElementPtr->state_flags, ElementFlags::Nonsolid)))
		{
			ElementPtr->current = ElementPtr->next;
			SetUpElement(ElementPtr);

			ElementPtr->state_flags |= ElementFlags::DefyPhysics;

			if (uqm::UQMOptions::read().advancedAutoPilot)
			{
				DoAdvancedAutoPilot();
			}
		}
	}

	return true;
}

static void hyper_collision(Element* ElementPtr0, GfxPoint* pPt0,
							Element* ElementPtr1, GfxPoint* pPt1)
{
	if ((testFlag(ElementPtr1->state_flags, ElementFlags::PlayerShip))
		&& uqm::gameState().flag(uqm::GameFlag::PORTAL_COUNTER) == 0)
	{
		int16_t dx {};
		int16_t dy {};
		GfxPoint pt;
		Starship* StarShipPtr {};

		ElementToUniverse(ElementPtr0, &pt);

		StarDesc* SDPtr {FindStar(nullptr, &pt, 5, 5)};

		GetElementStarShip(ElementPtr1, &StarShipPtr);
		GetCurrentVelocityComponents(&ElementPtr1->velocity, &dx, &dy);
		if (SDPtr == CurStarDescPtr()
			|| (testFlag(ElementPtr1->state_flags, ElementFlags::Appearing))
			|| !(dx || dy || (testFlag(StarShipPtr->cur_status_flags, (StatusFlags::Left | StatusFlags::Right | StatusFlags::Thrust | StatusFlags::Weapon | StatusFlags::Special)))))
		{
			CurStarDescPtr() = SDPtr;
			ElementPtr0->state_flags |= ElementFlags::DefyPhysics | ElementFlags::Collision;
		}
		else if ((uqm::currentActivity() & ActivityFlag::InBattle)
				 && (!uqm::flagship().autopilot.hasTarget()
					 || (uqm::flagship().autopilot.target().x == SDPtr->star_pt.x
						 && uqm::flagship().autopilot.target().y == SDPtr->star_pt.y)))
		{
			CurStarDescPtr() = SDPtr;
			ElementPtr0->state_flags |= ElementFlags::Collision;

			init_transition(ElementPtr0, ElementPtr1,
							TransitionType::Interplanetary);
		}
	}
	(void)pPt0; /* Satisfying compiler (unused parameter) */
	(void)pPt1; /* Satisfying compiler (unused parameter) */
}

static void hyper_death(Element* ElementPtr)
{
	if (!(testFlag(ElementPtr->state_flags, ElementFlags::DefyPhysics))
		&& (uqm::currentActivity() & ActivityFlag::InBattle))
	{
		CurStarDescPtr() = 0;
	}
}

static void arilou_space_death(Element* ElementPtr)
{
	if (!(testFlag(ElementPtr->state_flags, ElementFlags::DefyPhysics))
		|| uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_COUNTER) == 0)
	{
		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) <= 1)
		{
			uqm::gameState().setFlag(uqm::GameFlag::ARILOU_SPACE_SIDE, static_cast<uint32_t>(0));
		}
		else
		{
			uqm::gameState().setFlag(uqm::GameFlag::ARILOU_SPACE_SIDE, static_cast<uint32_t>(3));
		}
	}
}

static void arilou_space_collision(Element* ElementPtr0,
								   GfxPoint* pPt0, Element* ElementPtr1, GfxPoint* pPt1)
{
	uint16_t which_side {};

	if (!(testFlag(ElementPtr1->state_flags, ElementFlags::PlayerShip)))
	{
		return;
	}

	which_side = uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE);
	if (which_side == 0 || which_side == 3)
	{
		if (testFlag(ElementPtr1->state_flags, ElementFlags::DefyPhysics))
		{
			uqm::gameState().setFlag(uqm::GameFlag::ARILOU_SPACE_SIDE, static_cast<uint32_t>(which_side ^ 1));
		}
		else
		{
			init_transition(ElementPtr0, ElementPtr1,
							TransitionType::ArilouSpace);
		}
	}

	ElementPtr0->state_flags |= ElementFlags::DefyPhysics | ElementFlags::Collision;
	(void)pPt0; /* Satisfying compiler (unused parameter) */
	(void)pPt1; /* Satisfying compiler (unused parameter) */
}

static ElementHandle AllocHyperElement(const GfxPoint* elem_pt)
{
	ElementHandle hHyperSpaceElement {AllocElement()};
	if (hHyperSpaceElement)
	{
		Element* HyperSpaceElementPtr {};

		LockElement(hHyperSpaceElement, &HyperSpaceElementPtr);
		HyperSpaceElementPtr->playerNr = NeutralPlayerNum;
		HyperSpaceElementPtr->state_flags = ElementFlags::Changing | ElementFlags::FiniteLife;
		HyperSpaceElementPtr->life_span = 1;
		HyperSpaceElementPtr->mass_points = 1;

		{
			long lx {};
			long ly {};

			lx = UniverseToLogx(elem_pt->x)
			   + (LogSpaceWidth() >> 1) - uqm::gameState().sis().log_x;
			HyperSpaceElementPtr->current.location.x = WrapX(lx);

			ly = UniverseToLogy(elem_pt->y)
			   + (LogSpaceHeight() >> 1) - uqm::gameState().sis().log_y;
			HyperSpaceElementPtr->current.location.y = WrapY(ly);
		}

		SetPrimType(&DisplayArray[HyperSpaceElementPtr->PrimIndex],
					PrimitiveType::Stamp);
		HyperSpaceElementPtr->current.image.farray =
			&hyperstars[1 + (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) >> 1)];

		UnlockElement(hHyperSpaceElement);
	}

	return hHyperSpaceElement;
}

static void AddAmbientElement(void)
{
	ElementHandle hHyperSpaceElement {AllocElement()};
	if (hHyperSpaceElement)
	{
		int16_t dx {};
		int16_t dy {};
		Element* HyperSpaceElementPtr {};

		LockElement(hHyperSpaceElement, &HyperSpaceElementPtr);
		HyperSpaceElementPtr->playerNr = NeutralPlayerNum;
		HyperSpaceElementPtr->state_flags =
			ElementFlags::Appearing | ElementFlags::FiniteLife | ElementFlags::Nonsolid;
		SetPrimType(&DisplayArray[HyperSpaceElementPtr->PrimIndex],
					PrimitiveType::Stamp);
		HyperSpaceElementPtr->preprocess_func = animation_preprocess;

		if (IsHd() && inQuasiSpace())
		{
			SetPrimFlags(&DisplayArray[HyperSpaceElementPtr->PrimIndex],
						 HyperToQuasiColor);
		}

		const uint32_t rand_val {uqm::random().next()};
		dy = uqm::lowWord(rand_val);

		if (!IsHd())
		{
			dx = (int16_t)(uqm::lowByte(dy) % SpaceWidth()) - (SpaceWidth() >> 1);
			dy = (int16_t)(uqm::highByte(dy) % SpaceHeight()) - (SpaceHeight() >> 1);
		}
		else
		{
			dx = (int16_t)((uqm::highWord(rand_val)) % SpaceWidth())
			   - (SpaceWidth() >> 1);
			dy = (int16_t)(dy % SpaceHeight()) - (SpaceHeight() >> 1);
		}

		HyperSpaceElementPtr->current.location.x =
			(LogSpaceWidth() >> 1) + DisplayToWorld(dx);
		HyperSpaceElementPtr->current.location.y =
			(LogSpaceHeight() >> 1) + DisplayToWorld(dy);
		HyperSpaceElementPtr->current.image.farray = &stars_in_space;

		if (uqm::highWord(rand_val) & 7)
		{
			HyperSpaceElementPtr->life_span = 14;
			HyperSpaceElementPtr->current.image.frame = stars_in_space;
		}
		else
		{
			HyperSpaceElementPtr->life_span = 12;
			HyperSpaceElementPtr->current.image.frame =
				SetAbsFrameIndex(stars_in_space, 14);
		}

		UnlockElement(hHyperSpaceElement);

		InsertElement(hHyperSpaceElement, GetHeadElement());
	}
}

static auto NumVortexTransitions() { return (AnimatedHyperspace() ? 18 : 9); }
static constexpr auto VortexWait {1};
/* Hyperspace Animation related */
static constexpr auto BaseVortexFrameIndex {139};						// Kruzen: first frame of actual battle group vortex, not the birth/death
static auto TrCompensation() { return (AnimatedHyperspace() ? 8 : 0); } // Kruzen: transition_state counts down from 100 when reforming a vortex, but reaching 9 with
// standard option it stars count down every other frame, effectively adding 8 frames of waiting

// Kruzen: preprocess func for animated hole
static void encounter_animation(Element* ElementPtr)
{
	const uint16_t f_index = GetFrameIndex(ElementPtr->current.image.frame);

	if ((f_index >= BaseVortexFrameIndex + 31))
	{
		ElementPtr->next.image.frame =
			SetAbsFrameIndex(ElementPtr->current.image.frame, BaseVortexFrameIndex); // reached max - reset to base frame
	}
	else
	{
		ElementPtr->next.image.frame =
			IncFrameIndex(ElementPtr->current.image.frame);
	}
}

static void encounter_transition(Element* ElementPtr)
{
	ElementPtr->state_flags &= ~ElementFlags::Disappearing;
	ElementPtr->life_span = 1;
	if (ElementPtr->turn_wait)
	{
		--ElementPtr->turn_wait;
	}
	else
	{
		FrameRef f {};

		if (ElementPtr->hit_points)
		{
			f = DecFrameIndex(ElementPtr->current.image.frame);
			ElementPtr->next.image.frame = f;
		}
		else
		{
			f = IncFrameIndex(ElementPtr->current.image.frame);
			if (f != ElementPtr->current.image.farray[0])
			{
				ElementPtr->next.image.frame = f;
			}
			else
			{
				if (AnimatedHyperspace())
				{
					ElementPtr->preprocess_func = encounter_animation;
				}
				ElementPtr->death_func = nullptr;
			}
		}

		ElementPtr->turn_wait = (AnimatedHyperspace() ? 0 : VortexWait);
	}
}

static ElementHandle getSisElement(void)
{
	ElementHandle hShip {};

	StarshipHandle hSis {GetHeadLink(&race_q[RpgPlayerNum])};
	if (!hSis)
	{
		return nullptr;
	}

	const Starship* StarShipPtr {LockStarShip(&race_q[RpgPlayerNum], hSis)};
	hShip = StarShipPtr->hShip;
	UnlockStarShip(&race_q[RpgPlayerNum], hSis);

#ifdef DEBUG
	{
		Element* ElementPtr;
		LockElement(hShip, &ElementPtr);
		assert(testFlag(ElementPtr->state_flags, ElementFlags::PlayerShip));
		UnlockElement(hShip);
	}
#endif

	return hShip;
}

static void encounter_collision(Element* ElementPtr0, GfxPoint* pPt0,
								Element* ElementPtr1, GfxPoint* pPt1)
{
	EncounterHandle hEncounter {};
	EncounterHandle hNextEncounter {};

	if (!(testFlag(ElementPtr1->state_flags, ElementFlags::PlayerShip))
		|| !(uqm::currentActivity() & ActivityFlag::InBattle))
	{
		return;
	}

	init_transition(ElementPtr0, ElementPtr1,
					TransitionType::RandomEncounter);

	for (hEncounter = GetHeadEncounter();
		 hEncounter != 0; hEncounter = hNextEncounter)
	{
		Encounter* EncounterPtr {};

		LockEncounter(hEncounter, &EncounterPtr);
		hNextEncounter = GetSuccEncounter(EncounterPtr);
		if (EncounterPtr->hElement)
		{
			Element* ElementPtr {};

			LockElement(EncounterPtr->hElement, &ElementPtr);
			ElementPtr->state_flags |= ElementFlags::Nonsolid | ElementFlags::IgnoreSimilar;
			UnlockElement(EncounterPtr->hElement);
		}
		UnlockEncounter(hEncounter);
	}

	// Mark this Element as collided with flagship
	// XXX: We could simply set hTarget to 1 or to ElementPtr1,
	//   but that would be too hacky ;)
	ElementPtr0->hTarget = getSisElement();
	ZeroVelocityComponents(&ElementPtr0->velocity);

	(void)pPt0; /* Satisfying compiler (unused parameter) */
	(void)pPt1; /* Satisfying compiler (unused parameter) */
}

static ElementHandle AddEncounterElement(Encounter* EncounterPtr, GfxPoint* puniverse)
{
	bool NewEncounter {};
	GfxPoint enc_pt;

	if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) >= 2)
	{
		return 0;
	}

	if (EncounterPtr->flags & EncounterReforming)
	{
		EncounterPtr->flags &= ~EncounterReforming;

		EncounterPtr->transition_state = 100 + TrCompensation();
		if ((EncounterPtr->flags & OneShotEncounter)
			|| EncounterPtr->num_ships == 0)
		{
			return 0;
		}
	}

	if (EncounterPtr->num_ships)
	{
		NewEncounter = false;
		enc_pt = EncounterPtr->loc_pt;
	}
	else
	{
		int16_t dx {};
		int16_t dy {};
		uint16_t i {};
		uint32_t radius_squared {};
		const auto& EncounterMakeup {RaceEncounterMakeup};

		NewEncounter = true;

		radius_squared =
			(uint32_t)EncounterPtr->radius * EncounterPtr->radius;

		const uint8_t Type {EncounterPtr->race_id};
		uint16_t NumShips {uqm::lowNibble(EncounterMakeup[Type])};
		for (i = uqm::highNibble(EncounterMakeup[Type]) - NumShips; i; --i)
		{
			if ((uint16_t)uqm::random().next() % 100 < 50)
			{
				++NumShips;
			}
		}

		if (NumShips > MaxHyperShips)
		{
			NumShips = MaxHyperShips;
		}

		EncounterPtr->num_ships = NumShips;
		for (i = 0; i < NumShips; ++i)
		{
			BriefShipInfo* BSIPtr = &EncounterPtr->ShipList[i];
			FleetInfoHandle hStarShip =
				GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, Type);
			const FleetInfo* FleetPtr =
				LockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
			BSIPtr->race_id = Type;
			BSIPtr->crew_level = FleetPtr->crew_level;
			BSIPtr->max_crew = FleetPtr->max_crew;
			BSIPtr->max_energy = FleetPtr->max_energy;
			UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
		}

		do
		{
			const uint32_t rand_val {uqm::random().next()};

			enc_pt.x = puniverse->x
					 + (uqm::lowWord(rand_val) % (Xoffs() << 1)) - Xoffs();
			if (enc_pt.x < 0)
			{
				enc_pt.x = 0;
			}
			else if (enc_pt.x > MaxXUniverse)
			{
				enc_pt.x = MaxXUniverse;
			}
			enc_pt.y = puniverse->y
					 + (uqm::highWord(rand_val) % (Yoffs() << 1)) - Yoffs();
			if (enc_pt.y < 0)
			{
				enc_pt.y = 0;
			}
			else if (enc_pt.y > MaxYUniverse)
			{
				enc_pt.y = MaxYUniverse;
			}

			dx = enc_pt.x - EncounterPtr->origin.x;
			dy = enc_pt.y - EncounterPtr->origin.y;
		} while ((uint32_t)((long)dx * dx + (long)dy * dy) > radius_squared);

		EncounterPtr->loc_pt = enc_pt;
		EncounterPtr->log_x = UniverseToLogx(enc_pt.x);
		EncounterPtr->log_y = UniverseToLogy(enc_pt.y);
	}

	ElementHandle hElement {AllocHyperElement(&enc_pt)};
	if (hElement)
	{
		Element* ElementPtr {};

		LockElement(hElement, &ElementPtr);

		int16_t i {EncounterPtr->transition_state};
		if (i || NewEncounter)
		{
			if (i < 0)
			{
				i = -i;
				ElementPtr->hit_points = 1;
			}
			if (i == 0 || i > NumVortexTransitions())
			{
				i = NumVortexTransitions();
			}

			ElementPtr->current.image.frame = SetRelFrameIndex(
				ElementPtr->current.image.farray[0], -i);
			ElementPtr->death_func = encounter_transition;
			ElementPtr->preprocess_func = nullptr;
		}
		else
		{
			if (AnimatedHyperspace())
			{
				ElementPtr->preprocess_func = encounter_animation;
				ElementPtr->current.image.frame = SetAbsFrameIndex(
					ElementPtr->current.image.farray[0], BaseVortexFrameIndex + (uqm::random().next() % 31));
			}
			else
			{
				ElementPtr->current.image.frame = DecFrameIndex(
					ElementPtr->current.image.farray[0]);
				ElementPtr->preprocess_func = nullptr;
			}
		}

		ElementPtr->turn_wait = VortexWait;
		ElementPtr->postprocess_func = nullptr; // decorate_vortex;
		ElementPtr->collision_func = encounter_collision;

		SetUpElement(ElementPtr);

		ElementPtr->IntersectControl.IntersectStamp.frame =
			DecFrameIndex(stars_in_space);
		SetPrimType(&DisplayArray[ElementPtr->PrimIndex], NoPrim);
		ElementPtr->state_flags |= ElementFlags::Nonsolid | ElementFlags::IgnoreVelocity;

		UnlockElement(hElement);

		InsertElement(hElement, GetTailElement());
	}

	EncounterPtr->hElement = hElement;
	return hElement;
}

static constexpr auto GridOffset {200};

static void DrawHyperGrid(int16_t ux, int16_t uy, int16_t ox, int16_t oy)
{
	int16_t sx {};
	int16_t sy {};
	int16_t ex {};
	int16_t ey {};
	GfxRect r;

	ClearDrawable();
	SetContextForeGroundColor(
		BuildColor(MakeRgb15(0x00, 0x10, 0x00), 0x6B));

	sx = ux - (RadarScanWidth() >> 1);
	if (sx < 0)
	{
		sx = 0;
	}
	else
	{
		sx -= sx % GridOffset;
	}
	ex = ux + (RadarScanWidth() >> 1);
	if (ex > MaxXUniverse + 1)
	{
		ex = MaxXUniverse + 1;
	}

	sy = uy - (RadarScanHeight() >> 1);
	if (sy < 0)
	{
		sy = 0;
	}
	else
	{
		sy -= sy % GridOffset;
	}
	ey = uy + (RadarScanHeight() >> 1);
	if (ey > MaxYUniverse + 1)
	{
		ey = MaxYUniverse + 1;
	}

	r.corner.y = (int16_t)((long)(MaxYUniverse - ey)
						   * RadarHeight() / RadarScanHeight())
			   - oy;
	r.extent.width = ResScale(1);
	r.extent.height = ((int16_t)((long)(MaxYUniverse - sy)
								 * RadarHeight() / RadarScanHeight())
					   - oy)
					- r.corner.y
					+ ResScale(1);
	for (ux = sx; ux <= ex; ux += GridOffset)
	{
		r.corner.x = (int16_t)((long)ux * RadarWidth() / RadarScanWidth())
				   - ox;
		DrawFilledRectangle(&r);
	}

	r.corner.x = (int16_t)((long)sx * RadarWidth() / RadarScanWidth()) - ox;
	r.extent.width = ((int16_t)((long)ex * RadarWidth() / RadarScanWidth())
					  - ox)
				   - r.corner.x + ResScale(1);
	r.extent.height = ResScale(1);
	for (uy = sy; uy <= ey; uy += GridOffset)
	{
		r.corner.y = (int16_t)((long)(MaxYUniverse - uy)
							   * RadarHeight() / RadarScanHeight())
				   - oy;
		DrawFilledRectangle(&r);
	}
}

static constexpr auto EncounterTrackWait {3};

// Returns false if the Encounter is to be removed.
static bool ProcessEncounter(Encounter* EncounterPtr, GfxPoint* puniverse,
							 int16_t ox, int16_t oy, GfxStamp* stamp)
{
	Element* ElementPtr {};
	int16_t ex {};
	int16_t ey {};

	if (EncounterPtr->hElement == 0
		&& AddEncounterElement(EncounterPtr, puniverse) == 0)
	{
		return false;
	}

	LockElement(EncounterPtr->hElement, &ElementPtr);

	if (ElementPtr->death_func)
	{
		if (EncounterPtr->transition_state && ElementPtr->turn_wait == 0)
		{
			--EncounterPtr->transition_state;
			if (EncounterPtr->transition_state >= NumVortexTransitions())
			{
				++ElementPtr->turn_wait;
			}
			else if (EncounterPtr->transition_state == -NumVortexTransitions())
			{
				ElementPtr->death_func = nullptr;
				UnlockElement(EncounterPtr->hElement);
				return false;
			}
			else
			{
				SetPrimType(&DisplayArray[ElementPtr->PrimIndex],
							PrimitiveType::Stamp);
			}
		}
	}
	else
	{
		int16_t delta_x {};
		int16_t delta_y {};
		uint16_t encounter_radius {};

		ElementPtr->life_span = 1;
		GetNextVelocityComponents(&ElementPtr->velocity,
								  &delta_x, &delta_y, 1);
		if (ElementPtr->thrust_wait)
		{
			--ElementPtr->thrust_wait;
		}
		else if (!ElementPtr->hTarget)
		{ // This is an Encounter that did not collide with flagship
			// The colliding Encounter does not move
			uint16_t cur_facing {};
			uint16_t delta_facing {};

			cur_facing = AngleToFacing(
				GetVelocityTravelAngle(&ElementPtr->velocity));
			delta_facing = NormalizeFacing(cur_facing - AngleToFacing(ARCTAN(puniverse->x - EncounterPtr->loc_pt.x, puniverse->y - EncounterPtr->loc_pt.y)));
			if (delta_facing || (delta_x == 0 && delta_y == 0))
			{
				int16_t speed {RaceHyperSpeed((RaceId)EncounterPtr->race_id)};
				if (delta_facing < AngleToFacing(HalfCircle))
				{
					--cur_facing;
				}
				else
				{
					++cur_facing;
				}
				if (NormalizeFacing(delta_facing
									+ AngleToFacing(Octant))
					> AngleToFacing(Quadrant))
				{
					if (delta_facing < AngleToFacing(HalfCircle))
					{
						--cur_facing;
					}
					else
					{
						++cur_facing;
					}
					speed >>= 1;
				}
				cur_facing = FacingToAngle(cur_facing);
				SetVelocityComponents(&ElementPtr->velocity,
									  Cosine(cur_facing, speed),
									  Sine(cur_facing, speed));
				GetNextVelocityComponents(&ElementPtr->velocity,
										  &delta_x, &delta_y, 1);

				ElementPtr->thrust_wait = EncounterTrackWait;
			}
		}
		EncounterPtr->log_x += delta_x;
		EncounterPtr->log_y -= delta_y;
		EncounterPtr->loc_pt.x = LogxToUniverse(EncounterPtr->log_x);
		EncounterPtr->loc_pt.y = LogyToUniverse(EncounterPtr->log_y);

		encounter_radius = EncounterPtr->radius + (GridOffset >> 1);
		delta_x = EncounterPtr->loc_pt.x - EncounterPtr->origin.x;
		if (delta_x < 0)
		{
			delta_x = -delta_x;
		}
		delta_y = EncounterPtr->loc_pt.y - EncounterPtr->origin.y;
		if (delta_y < 0)
		{
			delta_y = -delta_y;
		}
		if ((uint16_t)delta_x >= encounter_radius
			|| (uint16_t)delta_y >= encounter_radius
			|| (uint32_t)delta_x * delta_x + (uint32_t)delta_y * delta_y >= (uint32_t)encounter_radius * encounter_radius)
		{
			// Encounter globe traveled outside the SoI and now disappears
			ElementPtr->state_flags |= ElementFlags::Nonsolid;
			ElementPtr->life_span = 0;

			if (EncounterPtr->transition_state == 0)
			{
				ElementPtr->death_func = encounter_transition;
				EncounterPtr->transition_state = -1;
				ElementPtr->hit_points = 1;

				if (AnimatedHyperspace())
				{
					ElementPtr->preprocess_func = nullptr;
					ElementPtr->current.image.frame =
						ElementPtr->current.image.farray[0];
				}
			}
			else
			{
				ElementPtr->death_func = nullptr;
				UnlockElement(EncounterPtr->hElement);
				return false;
			}
		}
	}

	ex = EncounterPtr->loc_pt.x;
	ey = EncounterPtr->loc_pt.y;
	if (ex - puniverse->x >= -UnitScreenWidth()
		&& ex - puniverse->x <= UnitScreenWidth()
		&& ey - puniverse->y >= -UnitScreenHeight()
		&& ey - puniverse->y <= UnitScreenHeight())
	{
		ElementPtr->next.location.x =
			(int16_t)(EncounterPtr->log_x - uqm::gameState().sis().log_x)
			+ (LogSpaceWidth() >> 1);
		ElementPtr->next.location.y =
			(int16_t)(EncounterPtr->log_y - uqm::gameState().sis().log_y)
			+ (LogSpaceHeight() >> 1);
		if ((testFlag(ElementPtr->state_flags, ElementFlags::Nonsolid))
			&& EncounterPtr->transition_state == 0)
		{
			ElementPtr->current.location = ElementPtr->next.location;
			SetPrimType(&DisplayArray[ElementPtr->PrimIndex],
						PrimitiveType::Stamp);
			if (ElementPtr->death_func == 0)
			{
				InitIntersectStartPoint(ElementPtr);
				ElementPtr->state_flags &= ~ElementFlags::Nonsolid;
			}
		}
	}
	else
	{
		ElementPtr->state_flags |= ElementFlags::Nonsolid;
		if (ex - puniverse->x < -Xoffs() || ex - puniverse->x > Xoffs()
			|| ey - puniverse->y < -Yoffs() || ey - puniverse->y > Yoffs())
		{
			ElementPtr->life_span = 0;
			ElementPtr->death_func = nullptr;
			UnlockElement(EncounterPtr->hElement);
			return false;
		}

		SetPrimType(&DisplayArray[ElementPtr->PrimIndex], NoPrim);
	}

	UnlockElement(EncounterPtr->hElement);

	stamp->origin.x = (int16_t)((long)ex * RadarWidth() / RadarScanWidth())
					- ox;
	stamp->origin.y = (int16_t)((long)(MaxYUniverse - ey) * RadarHeight()
								/ RadarScanHeight())
					- oy;
	DrawStamp(stamp);

	return true;
}

static void ProcessEncounters(GfxPoint* puniverse, int16_t ox, int16_t oy)
{
	GfxStamp stamp;
	EncounterHandle hEncounter {};
	EncounterHandle hNextEncounter {};

	stamp.frame = SetAbsFrameIndex(stars_in_space, 91);
	for (hEncounter = GetHeadEncounter();
		 hEncounter; hEncounter = hNextEncounter)
	{
		Encounter* EncounterPtr {};

		LockEncounter(hEncounter, &EncounterPtr);
		hNextEncounter = GetSuccEncounter(EncounterPtr);

		if (!ProcessEncounter(EncounterPtr, puniverse, ox, oy, &stamp))
		{
			UnlockEncounter(hEncounter);
			RemoveEncounter(hEncounter);
			FreeEncounter(hEncounter);
			continue;
		}

		UnlockEncounter(hEncounter);
	}
}

static constexpr auto NumFrames {32};

static Color GetMaskColor(uint8_t startype)
{
	switch (static_cast<StarBodyColor>(startype))
	{
		case StarBodyColor::Red:
			return RedStarColor;
		case StarBodyColor::Orange:
			return OrangeStarColor;
		case StarBodyColor::Yellow:
			return YellowStarColor;
		case StarBodyColor::Green:
			return GreenStarColor;
		case StarBodyColor::Blue:
			return BlueStarColor;
		default:
			return WhiteColor;
	}
}

void SeedUniverse(void)
{
	int16_t ox {};
	int16_t oy {};
	int16_t sx {};
	int16_t sy {};
	int16_t ex {};
	int16_t ey {};
	int16_t portalCounter {};
	int16_t arilouSpaceCounter {};
	int16_t arilouSpaceSide {};
	GfxPoint universe;
	FrameRef star_frame {};
	GfxStamp s;
	StarDesc* SDPtr {};
	ElementHandle hHyperSpaceElement {};
	Element* HyperSpaceElementPtr {};
	int32_t lx {};
	int32_t ly {};
	GfxRect frameRect;
	LargeExtent img_log;

	static uint16_t frameCounter = 0; // BW

	universe.x = LogxToUniverse(uqm::gameState().sis().log_x);
	universe.y = LogyToUniverse(uqm::gameState().sis().log_y);

	FrameRef blip_frame {SetAbsFrameIndex(stars_in_space, 90)};

	SetContext(RadarContext);
	BatchGraphics();

	ox = (int16_t)((long)universe.x * RadarWidth() / RadarScanWidth())
	   - (RadarWidth() >> 1);
	oy = (int16_t)((long)(MaxYUniverse - universe.y)
				   * RadarHeight() / RadarScanHeight())
	   - (RadarHeight() >> 1);

	ex = (int16_t)((long)uqm::flagship().shipStamp.origin.x
				   * RadarWidth() / RadarScanWidth())
	   - (RadarWidth() >> 1);
	ey = (int16_t)((long)(MaxYUniverse - uqm::flagship().shipStamp.origin.y)
				   * RadarHeight() / RadarScanHeight())
	   - (RadarHeight() >> 1);

	arilouSpaceCounter = uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_COUNTER);
	arilouSpaceSide = uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE);

	//	if (ox != ex || oy != ey)
	{
		DrawHyperGrid(universe.x, universe.y, ox, oy);

		{
			SDPtr = 0;
			while ((SDPtr = FindStar(SDPtr, &universe, Xoffs(), Yoffs())))
			{
				ex = SDPtr->star_pt.x;
				ey = SDPtr->star_pt.y;
				uint8_t star_type {*StarType(SDPtr->Type)};
				if (arilouSpaceSide >= 2 && ex == ArilouHomeX && ey == ArilouHomeY)
				{
					star_type = *StellarSize::SuperGiant;
				}

				s.origin.x = (int16_t)((long)ex * RadarWidth()
									   / RadarScanWidth())
						   - ox;
				s.origin.y = (int16_t)((long)(MaxYUniverse - ey)
									   * RadarHeight() / RadarScanHeight())
						   - oy;
				s.frame = SetRelFrameIndex(blip_frame,
										   star_type + 2);
				DrawStamp(&s);
			}
		}
	}

	portalCounter = uqm::gameState().flag(uqm::GameFlag::PORTAL_COUNTER);
	if (portalCounter || arilouSpaceCounter)
	{
		uqstl::array<StarDesc, 2> SD;
		// This array is filled with the StarDesc's of
		// QuasiSpace portals that need to be taken into account.
		// i is set to the number of active portals (max 2).

		uint16_t i {0};
		if (portalCounter)
		{
			// A player-created QuasiSpace portal is opening.
			static GfxPoint portal_pt;

			if (AnimatedHyperspace())
			{
				SD[i].Index = portalCounter + 98;
				if (SD[i].Index > 107)
				{
					SD[i].Index--;
				}
			}
			else
			{
				SD[i].Index = ((portalCounter - 1) >> 1) + 18;
			}
			if (portalCounter == 1)
			{
				portal_pt = universe;
			}
			SD[i].star_pt = portal_pt;
			++i;

			if (++portalCounter == (10 + 1))
			{
				portalCounter = (9 + 1);
			}


			uqm::gameState().setFlag(uqm::GameFlag::PORTAL_COUNTER, static_cast<uint32_t>(portalCounter));
		}

		if (arilouSpaceCounter)
		{
			// The periodically appearing QuasiSpace portal is open.
			if (AnimatedHyperspace())
			{
				SD[i].Index = arilouSpaceCounter - 1;
			}
			else
			{
				SD[i].Index = arilouSpaceCounter >> 1;
			}
			if (arilouSpaceSide <= 1)
			{
				// The player is in HyperSpace
				if (AnimatedHyperspace())
				{
					SD[i].Index += 81;
				}
				SD[i].Index += 18;
				// JSD Converting hard coded ARILOU portal to plot based
				//SD[i].star_pt.x = ArilouSpaceX;
				//SD[i].star_pt.y = ArilouSpaceY;
				SD[i].star_pt = plot_map[*PlotId::Arilou].star_pt;
			}
			else
			{
				// The player is in QuasiSpace
				SD[i].star_pt.x = QuasiSpaceX;
				SD[i].star_pt.y = QuasiSpaceY;
			}
			++i;
		}

		// Process the i portals from SD.
		do
		{
			--i;
			sx = SD[i].star_pt.x - universe.x + Xoffs();
			sy = SD[i].star_pt.y - universe.y + Yoffs();
			if (sx < 0 || sy < 0 || sx >= (Xoffs() << 1)
				|| sy >= (Yoffs() << 1))
			{
				continue;
			}

			ex = SD[i].star_pt.x;
			ey = SD[i].star_pt.y;
			s.origin.x = (int16_t)((long)ex * RadarWidth() / RadarScanWidth())
					   - ox;
			s.origin.y = (int16_t)((long)(MaxYUniverse - ey)
								   * RadarHeight() / RadarScanHeight())
					   - oy;
			s.frame = SetAbsFrameIndex(stars_in_space, 95);
			DrawStamp(&s);

			ex -= universe.x;
			if (ex < 0)
			{
				ex = -ex;
			}
			ey -= universe.y;
			if (ey < 0)
			{
				ey = -ey;
			}

			if (ex > (Xoffs() / NumRadarScreens)
				|| ey > (Yoffs() / NumRadarScreens))
			{
				continue;
			}

			hHyperSpaceElement = AllocHyperElement(&SD[i].star_pt);
			if (hHyperSpaceElement == 0)
			{
				continue;
			}

			LockElement(hHyperSpaceElement, &HyperSpaceElementPtr);

			if (AnimatedHyperspace() && ((SD[i].Index == 107 && arilouSpaceSide <= 1) || (SD[i].Index == 8 && arilouSpaceSide > 1)))
			{
				HyperSpaceElementPtr->current.image.frame =
					SetAbsFrameIndex(
						hyperstars[1 + (arilouSpaceSide >> 1)],
						frameCounter + SD[i].Index);
			}
			else
			{
				HyperSpaceElementPtr->current.image.frame =
					SetAbsFrameIndex(
						hyperstars[1 + (arilouSpaceSide >> 1)],
						SD[i].Index);
			}

			HyperSpaceElementPtr->preprocess_func = nullptr;
			HyperSpaceElementPtr->postprocess_func = nullptr;
			HyperSpaceElementPtr->collision_func = arilou_space_collision;

			SetUpElement(HyperSpaceElementPtr);

			if (arilouSpaceSide == 1 || arilouSpaceSide == 2)
			{
				HyperSpaceElementPtr->death_func = arilou_space_death;
			}
			else
			{
				HyperSpaceElementPtr->death_func = nullptr;
				HyperSpaceElementPtr->IntersectControl.IntersectStamp.frame =
					DecFrameIndex(stars_in_space);
			}

			UnlockElement(hHyperSpaceElement);

			InsertElement(hHyperSpaceElement, GetHeadElement());
		} while (i);
	}

	{
		SDPtr = 0;
		while ((SDPtr = FindStar(SDPtr, &universe, Xoffs(), Yoffs())))
		{
			const uint8_t star_type = SDPtr->Type;
			const StarBodyColor star_color {StarColor(star_type)};

			lx = UniverseToLogx(SDPtr->star_pt.x) - uqm::gameState().sis().log_x;
			ly = UniverseToLogy(SDPtr->star_pt.y) - uqm::gameState().sis().log_y;

			if (AnimatedHyperspace())
			{
				if ((arilouSpaceSide > 1))
				{
					star_frame = SetAbsFrameIndex(hyperstars[2],
												  frameCounter + ((star_color == StarBodyColor::Yellow) ? 40 : 8));
				}
				else
				{
					star_frame = SetAbsFrameIndex(hyperstars[1],
												  *StarType(star_type) * NumFrames + frameCounter);
				}
			}
			else
			{
				star_frame = SetAbsFrameIndex(
					hyperstars[1 + (arilouSpaceSide >> 1)],
					*StarType(star_type) * *StarBodyColor::Count
						+ *star_color);
			}

			if (star_frame == nullptr)
			{
				continue;
			}

			GetFrameRect(star_frame, &frameRect);

			img_log.width = frameRect.extent.width << 1;
			img_log.height = frameRect.extent.height << 1;

			if ((lx + img_log.width) < -HsWestEdge() || (lx - img_log.width) > HsEastEdge() || (ly + img_log.height) < -HsNorthEdge() || (ly - img_log.height) > HsSouthEdge())
			{
				continue;
			}

			hHyperSpaceElement = AllocHyperElement(&SDPtr->star_pt);
			if (hHyperSpaceElement == 0)
			{
				continue;
			}

			LockElement(hHyperSpaceElement, &HyperSpaceElementPtr);

			if (AnimatedHyperspace() && !(arilouSpaceSide > 1)
				&& star_color != StarBodyColor::White)
			{
				const uint16_t* PrimIndex {};

				HyperSpaceElementPtr->current.image.frame =
					SetAbsFrameIndex(hyperstars[1],
									 96 + StarType(star_type));

				PrimIndex = &HyperSpaceElementPtr->PrimIndex;

				SetPrimType(&DisplayArray[*PrimIndex], PrimitiveType::StampFill);
				SetPrimColor(&DisplayArray[*PrimIndex],
							 GetMaskColor(*StarColor(star_type)));
				SetPrimFlags(&DisplayArray[*PrimIndex], HsStarmask);

				HyperSpaceElementPtr->state_flags =
					ElementFlags::Appearing | ElementFlags::FiniteLife | ElementFlags::Nonsolid;

				UnlockElement(hHyperSpaceElement);

				InsertElement(hHyperSpaceElement, GetHeadElement());

				hHyperSpaceElement = AllocHyperElement(&SDPtr->star_pt);
				if (hHyperSpaceElement == 0)
				{
					continue;
				}

				LockElement(hHyperSpaceElement, &HyperSpaceElementPtr);
			}

			HyperSpaceElementPtr->current.image.frame = star_frame;
			HyperSpaceElementPtr->preprocess_func = nullptr;
			HyperSpaceElementPtr->postprocess_func = nullptr;
			HyperSpaceElementPtr->collision_func = hyper_collision;

			SetUpElement(HyperSpaceElementPtr);

			if (SDPtr == CurStarDescPtr()
				&& uqm::gameState().flag(uqm::GameFlag::PORTAL_COUNTER) == 0)
			{
				HyperSpaceElementPtr->death_func = hyper_death;
			}
			else
			{
				HyperSpaceElementPtr->death_func = nullptr;
				HyperSpaceElementPtr->IntersectControl.IntersectStamp.frame =
					DecFrameIndex(stars_in_space);
			}
			UnlockElement(hHyperSpaceElement);

			InsertElement(hHyperSpaceElement, GetHeadElement());
		}
		ProcessEncounters(&universe, ox, oy);
	}

	s.origin.x = RadarWidth() >> 1;
	s.origin.y = RadarHeight() >> 1;
	s.frame = blip_frame;
	DrawStamp(&s);

	DrawRadarBorder();

	UnbatchGraphics();

	SetContext(StatusContext);

	if (!(uqm::lowWord(uqm::random().next()) & 7))
	{
		AddAmbientElement();
	}

	if (universe.x != uqm::flagship().shipStamp.origin.x
		|| universe.y != uqm::flagship().shipStamp.origin.y)
	{
		uqm::flagship().shipStamp.origin = universe;
		DrawHyperCoords(universe);
	}

	frameCounter = (frameCounter + 1) & 0x1F;
}

static bool DoHyperspaceMenu(MenuState* pMS)
{
	bool select = PulsedInputState.menu[*MenuKey::MenuSelect];

	if ((uqm::currentActivity() & (ActivityFlag::CheckAbort | ActivityFlag::CheckLoad))
		|| uqm::gameState().sis().CrewEnlisted == (uint16_t)~0)
	{
		return false;
	}

	const bool handled {DoMenuChooser(pMS, MenuString::Starmap)};
	if (handled)
	{
		return true;
	}

	if (!select)
	{
		return true;
	}

	SetFlashRect(nullptr, false);

	switch (pMS->CurState)
	{
		case *HyperMenuItem::EquipDevice:
			select = DevicesMenu();
			if (uqm::gameState().flag(uqm::GameFlag::PORTAL_COUNTER))
			{ // A player-induced portal to QuasiSpace is opening
				return false;
			}
			if (uqm::currentActivity() & ActivityFlag::StartEncounter)
			{ // Selected Talking Pet, going into conversation
				return false;
			}
			break;
		case *HyperMenuItem::Cargo:
			CargoMenu();
			break;
		case *HyperMenuItem::Roster:
			select = RosterMenu();
			break;
		case *HyperMenuItem::GameMenu:
			if (!GameOptions())
			{
				return false; // abort or load
			}
			break;
		case *HyperMenuItem::Starmap:
			StarMap();
			return false;
		case *HyperMenuItem::Navigation:
			return false;
	}

	if (!(uqm::currentActivity() & ActivityFlag::CheckAbort))
	{
		if (select)
		{ // 3DO menu jumps to NAVIGATE after a successful submenu run
			if (uqm::UQMOptions::read().whichMenu != uqm::EmulationMode::PC)
			{
				pMS->CurState = *HyperMenuItem::Navigation;
			}
			DrawMenuStateStrings(MenuString::Starmap, pMS->CurState);
		}
		SetFlashRect(SfrMenu3do(), false);
	}

	return true;
}

void HyperspaceMenu(void)
{
	Color OldColor;
	MenuState menuState;

	UnbatchGraphics();

	GfxContextRef OldContext {SetContext(SpaceContext)};
	OldColor = SetContextBackGroundColor(BlackColor);


	memset(&menuState, 0, sizeof(menuState));
	menuState.InputFunc = DoHyperspaceMenu;
	menuState.Initialized = true;
	menuState.CurState = *HyperMenuItem::Starmap;

	DrawMenuStateStrings(MenuString::Starmap, *HyperMenuItem::Starmap);
	SetFlashRect(SfrMenu3do(), false);

	SetMenuSounds(MenuSoundFlags::Arrows, MenuSoundFlags::Select);
	DoInput(&menuState, true);

	SetFlashRect(nullptr, false);

	SetContext(SpaceContext);

	if (!(uqm::currentActivity() & (ActivityFlag::CheckAbort | ActivityFlag::CheckLoad)))
	{
		ClearSISRect(ClearSisFlags::ClearRadar);
		WaitForNoInput(c_gameSecond / 2, false);
	}

	SetContextBackGroundColor(OldColor);
	SetContext(OldContext);
	if (!(uqm::currentActivity() & ActivityFlag::InBattle))
	{
		cleanup_hyperspace();
	}

	BatchGraphics();
}

void SaveSisHyperState(void)
{
	Element* ElementPtr {};
	Starship* StarShipPtr {};

	// Update 'GLOBAL (ShipFacing)' to the direction the flagship is facing
	ElementHandle hSisElement {getSisElement()};
	if (!hSisElement)
	{ // Happens when saving a game from Hyperspace Encounter screen
		return;
	}
	//if (ElementPtr->state_flags & PlayerShip)
	LockElement(hSisElement, &ElementPtr);
	GetElementStarShip(ElementPtr, &StarShipPtr);
	// XXX: Solar system reentry test depends on ShipFacing != 0
	uqm::flagship().shipFacing = StarShipPtr->ShipFacing + 1;
	UnlockElement(hSisElement);
}

} // namespace uqm
