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
#include "gameopt.h"
#include "menustat.h"
// for DrawMenuStateStrings()
#include "encount.h"
#include "starmap.h"
#include "ship.h"
#include "shipcont.h"
#include "process.h"
#include "globdata.h"
#include "sis.h"
#include "units.h"
#include "init.h"
#include "nameref.h"
#include "resinst.h"
#include "setup.h"
#include "sounds.h"
#include "options.h"
#include "libs/gfxlib.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/drawable.h"
#include "libs/mathlib.h"
#include "core/log/log.h"
#include "util.h"
#include "uqm/random/UQMRandom.h"

#define XOFFS ((RADAR_SCAN_WIDTH + (UNIT_SCREEN_WIDTH << 2)) >> 1)
#define YOFFS ((RADAR_SCAN_HEIGHT + (UNIT_SCREEN_HEIGHT << 2)) >> 1)
#define ANIMATED_HYPERSPACE (IS_HD && uqm::UQMOptions::read().hyperStars)

static FRAME hyperstars[3];
static COLORMAP hypercmaps[2];
static uint8_t fuel_ticks;
static uint16_t hyper_dx, hyper_dy, hyper_extra;

// HyperspaceMenu() items
enum HyperMenuItems
{
	// XXX: Must match the enum in menustat.h
	STARMAP = 1,
	EQUIP_DEVICE,
	CARGO,
	ROSTER,
	GAME_MENU,
	NAVIGATION,
};

static inline int16_t
RaceHyperSpeed(RACE_ID Index)
{
	static int seedStamp = -1;
	const int defaultMap[] {RACE_HYPER_SPEED};
	const uint16_t numRaces = uqstl::size(defaultMap);
	static int16_t speedMap[numRaces] = {0}; // set urquan drone to 0.
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
/*
 * draws the melee icon for the battle group inside the black holes,
 * so you can see who's chasing you.
 */
/*static void
decorate_vortex (ELEMENT *ElementPtr)
{
	HENCOUNTER hEncounter, hNextEncounter;
	FRAME f = nullptr;
	static FRAME vortex_ships[NUM_AVAILABLE_RACES];

	// The element is still spawning, nothing to do yet
	if (ElementPtr->death_func)
		return;

	// The element doesn't know what kind of ship it is, that
	// info is stored in the encounter queue.  I'm guessing this
	// needs refactoring
	for (hEncounter = GetHeadEncounter ();
		hEncounter != 0; hEncounter = hNextEncounter)
	{
		ENCOUNTER *EncounterPtr;

		LockEncounter (hEncounter, &EncounterPtr);
		hNextEncounter = GetSuccEncounter (EncounterPtr);
		if (EncounterPtr->hElement)
		{
			ELEMENT *EncounterElementPtr;

			LockElement (EncounterPtr->hElement, &EncounterElementPtr);
			if (EncounterElementPtr == ElementPtr)
			{
				HFLEETINFO hFleet;
				FLEET_INFO *FleetPtr;

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
						GFXCONTEXT tmp, old;
						Color trans;
						STAMP s;

						vortex_ships[EncounterPtr->race_id] =
								CaptureDrawable (
									CreateDrawable (
										WantPixmap | WantAlpha,
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

						// the original element
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
	GLOBAL_SIS(log_x) -= new_dx;
	if (GLOBAL_SIS(log_x) < 0)
	{
		new_dx += (int16_t)GLOBAL_SIS(log_x);
		GLOBAL_SIS(log_x) = 0;
	}
	else if (GLOBAL_SIS(log_x) > MAX_X_LOGICAL)
	{
		new_dx += (int16_t)(GLOBAL_SIS(log_x) - MAX_X_LOGICAL);
		GLOBAL_SIS(log_x) = MAX_X_LOGICAL;
	}

	int32_t new_dy {*pdy};
	GLOBAL_SIS(log_y) -= new_dy;
	if (GLOBAL_SIS(log_y) < 0)
	{
		new_dy += (int16_t)GLOBAL_SIS(log_y);
		GLOBAL_SIS(log_y) = 0;
	}
	else if (GLOBAL_SIS(log_y) > MAX_Y_LOGICAL)
	{
		new_dy += (int16_t)(GLOBAL_SIS(log_y) - MAX_Y_LOGICAL);
		GLOBAL_SIS(log_y) = MAX_Y_LOGICAL;
	}

	if (new_dx != *pdx || new_dy != *pdy)
	{
		HELEMENT hElement {};
		HELEMENT hNextElement {};

		*pdx = new_dx;
		*pdy = new_dy;

		for (hElement = GetTailElement();
			 hElement != 0; hElement = hNextElement)
		{
			ELEMENT* ElementPtr {};

			LockElement(hElement, &ElementPtr);

			if (!(ElementPtr->state_flags & PLAYER_SHIP))
			{
				hNextElement = GetPredElement(ElementPtr);
			}
			else
			{
				ElementPtr->next.location.x = (LOG_SPACE_WIDTH >> 1)
											- new_dx;
				ElementPtr->next.location.y = (LOG_SPACE_HEIGHT >> 1)
											- new_dy;
				hNextElement = 0;
			}

			UnlockElement(hElement);
		}
	}

	if (GLOBAL_SIS(FuelOnBoard)
		&& GET_GAME_STATE(ARILOU_SPACE_SIDE) <= 1)
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
		adj_dx = LOGX_TO_UNIVERSE(16 * hyper_dx);
		adj_dy = MAX_Y_UNIVERSE - LOGY_TO_UNIVERSE(16 * hyper_dy);

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
	GFXPOINT universe;
	HFLEETINFO hStarShip {};
	HFLEETINFO hNextShip {};
	const uint16_t EncounterPercent[] =
		{
			RACE_HYPERSPACE_PERCENT};

	universe.x = LOGX_TO_UNIVERSE(GLOBAL_SIS(log_x));
	universe.y = LOGY_TO_UNIVERSE(GLOBAL_SIS(log_y));
	for (hStarShip = GetHeadLink(&GLOBAL(avail_race_q)), Type = 0;
		 hStarShip && (uqm::currentActivity() & IN_BATTLE);
		 hStarShip = hNextShip, ++Type)
	{
		const FLEET_INFO* FleetPtr {LockFleetInfo(&GLOBAL(avail_race_q), hStarShip)};
		hNextShip = _GetSuccLink(FleetPtr);

		uint16_t encounter_radius {FleetPtr->actual_strength};
		if (encounter_radius)
		{
			int16_t dx {};
			int16_t dy {};
			HENCOUNTER hEncounter {};
			ENCOUNTER* EncounterPtr {};

			uint8_t encounter_flags {0};
			uint16_t percent {EncounterPercent[Type]};

			if (uqm::UQMOptions::read().noHQEncounters)
			{
				percent = 0;
			}

			if (encounter_radius != INFINITE_RADIUS)
			{
				encounter_radius =
					(encounter_radius * SPHERE_RADIUS_INCREMENT) >> 1;

				if (EXTENDED && GET_GAME_STATE(KOHR_AH_FRENZY)
					&& Type == BLACK_URQUAN_SHIP
					&& GET_GAME_STATE(USED_BROADCASTER))
				{
					encounter_radius = (MAX_X_UNIVERSE + 1) << 1;
					percent = 100;
					encounter_flags = ONE_SHOT_ENCOUNTER;

					SET_GAME_STATE(BROADCASTER_RESPONSE, 1);
				}
			}
			else /* encounter_radius == infinity */
			{
				HENCOUNTER hNextEncounter {};

				encounter_radius = (MAX_X_UNIVERSE + 1) << 1;
				if (Type == SLYLANDRO_SHIP)
				{
					encounter_flags = ONE_SHOT_ENCOUNTER;
					if (!GET_GAME_STATE(STARBASE_AVAILABLE))
					{
						percent = 100;
					}
					else
					{
						percent *= GET_GAME_STATE(SLYLANDRO_MULTIPLIER);
					}

					if (uqm::UQMOptions::read().noHQEncounters)
					{
						percent = 0;
					}
				}

				if ((EXTENDED && !GET_GAME_STATE(KOHR_AH_FRENZY))
					|| !EXTENDED)
				{
					if (Type == MELNORME_SHIP
						&& (GLOBAL_SIS(FuelOnBoard) == 0
							|| GET_GAME_STATE(USED_BROADCASTER))
						&& GET_GAME_STATE(MELNORME_ANGER) < 3)
					{
						if (!GET_GAME_STATE(USED_BROADCASTER))
						{
							percent = 30;
						}
						else
						{
							percent = 100;
						}
						encounter_flags = ONE_SHOT_ENCOUNTER;
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


				if (percent == 100 && Type == MELNORME_SHIP)
				{
					SET_GAME_STATE(BROADCASTER_RESPONSE, 1);
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
				// Ship spawned for encounter.
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

		UnlockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
	}

	SET_GAME_STATE(USED_BROADCASTER, 0);
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

#define ARISPACE_MASK (ANIMATED_HYPERSPACE ?        \
						   ARIANIM_MASK_PMAP_ANIM : \
						   ARISPACE_MASK_PMAP_ANIM)
#define HYPERSTARS_MASK (ANIMATED_HYPERSPACE ?          \
							 HYPERANIM_MASK_PMAP_ANIM : \
							 HYPERSTARS_MASK_PMAP_ANIM)

static void
LoadHyperData(void)
{
	if (hyperstars[0] == 0)
	{
		hyperstars[0] = CaptureDrawable(
			LoadGraphic(AMBIENT_MASK_PMAP_ANIM));
		hyperstars[1] = CaptureDrawable(
			LoadGraphic(HYPERSTARS_MASK));
		hypercmaps[0] = uqm::loadStringTable(HYPER_COLOR_TAB).release();

		hyperstars[2] = CaptureDrawable(
			LoadGraphic(ARISPACE_MASK));
		hypercmaps[1] = uqm::loadStringTable(ARISPACE_COLOR_TAB).release();
	}
}

bool LoadHyperspace(void)
{
	hyper_dx = 0;
	hyper_dy = 0;
	hyper_extra = 0;
	fuel_ticks = 1;

	uqm::flagship().shipStamp.origin.x = -MAX_X_UNIVERSE;
	uqm::flagship().shipStamp.origin.y = -MAX_Y_UNIVERSE;

	LoadHyperData();
	{
		FRAME F {hyperstars[0]};
		hyperstars[0] = stars_in_space;
		stars_in_space = F;
	}

	if (!(LastActivity & CHECK_LOAD))
	{
		RepairSISBorder();
	}
	else
	{
		if (uqm::lowByte(LastActivity) == 0)
		{
			DrawSISFrame();
		}
		else
		{
			ClearSISRect(DRAW_SIS_DISPLAY);
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
	if (GET_GAME_STATE(ARILOU_SPACE_SIDE) <= 1)
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
		SET_GAME_STATE(USED_BROADCASTER, 0);
		SET_GAME_STATE(BROADCASTER_RESPONSE, 0);
	}
	//    ClearDrawable ();

	return true;
}

void EraseRadar(void)
{
	BatchGraphics();
	ClearSISRect(CLEAR_SIS_RADAR);
	SeedUniverse();
	UnbatchGraphics();
}

bool FreeHyperspace(void)
{
	{
		FRAME F {hyperstars[0]};
		hyperstars[0] = stars_in_space;
		stars_in_space = F;
	}
	// Kruzen: To load correct set of frames until we'll use 1 with space filters
	//if (IS_HD)
	//	ReloadHyperSpace ();

	//FreeHyperData ();

	return true;
}

static void
ElementToUniverse(ELEMENT* ElementPtr, GFXPOINT* pPt)
{
	int32_t log_x {};
	int32_t log_y {};

	log_x = GLOBAL_SIS(log_x)
		  + (ElementPtr->next.location.x - (LOG_SPACE_WIDTH >> 1));
	log_y = GLOBAL_SIS(log_y)
		  + (ElementPtr->next.location.y - (LOG_SPACE_HEIGHT >> 1));
	pPt->x = LOGX_TO_UNIVERSE(log_x);
	pPt->y = LOGY_TO_UNIVERSE(log_y);
}

static void
cleanup_hyperspace(void)
{
	HENCOUNTER hEncounter {};
	HENCOUNTER hNextEncounter {};

	for (hEncounter = GetHeadEncounter();
		 hEncounter != 0; hEncounter = hNextEncounter)
	{
		ENCOUNTER* EncounterPtr {};

		LockEncounter(hEncounter, &EncounterPtr);
		hNextEncounter = GetSuccEncounter(EncounterPtr);
		if (EncounterPtr->hElement)
		{
			ELEMENT* ElementPtr {};

			LockElement(EncounterPtr->hElement, &ElementPtr);

			if (ElementPtr->hTarget)
			{ // This is the encounter that collided with flagship
				// Move the encounter to the head of the queue so that
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

typedef enum
{
	RANDOM_ENCOUNTER_TRANSITION,
	INTERPLANETARY_TRANSITION,
	ARILOU_SPACE_TRANSITION
} TRANSITION_TYPE;

static void
InterplanetaryTransition(ELEMENT* ElementPtr)
{
	uqm::flagship().ipPlanet = 0;
	uqm::flagship().inOrbit = 0;
	uqm::flagship().shipFacing = 0; /* Not reentering the system */
	SET_GAME_STATE(USED_BROADCASTER, 0);
	if (GET_GAME_STATE(ARILOU_SPACE_SIDE) <= 1)
	{
		// Enter a solar system from HyperSpace.
		uqm::currentActivity() |= START_INTERPLANETARY;
		SET_GAME_STATE(ESCAPE_COUNTER, 0);
	}
	else
	{
		GFXPOINT pt;
		uint16_t KnownQSPortals = GET_GAME_STATE(KNOW_QS_PORTAL);

		uqm::flagship().autopilot.clear();

		ElementToUniverse(ElementPtr, &pt);
		CurStarDescPtr = FindStar(nullptr, &pt, 5, 5);

		// JMS: Debugging helpers
		/*{
			STAR_DESC *SDPtr, *SDPtr2;
			SDPtr = CurStarDescPtr;
			SDPtr2 = FindStar (nullptr, &pt, 500, 500);
			uqm::log::debug( "SDPtr.x {}, SDPtr.y {} SDPtr2.x {}, "
					"SDPtr2.y {}, pt.x {} pt.y {}", 
					SDPtr->star_pt.x, SDPtr->star_pt.y, SDPtr2->star_pt.x,
					SDPtr2->star_pt.y, pt.x, pt.y);
		}*/

		if (CurStarDescPtr->star_pt.x == ARILOU_HOME_X
			&& CurStarDescPtr->star_pt.y == ARILOU_HOME_Y)
		{
			// Meet the Arilou.
			uqm::currentActivity() |= START_ENCOUNTER;

			// JMS: The arilou homeworld name can now be shown on QS map.
			KnownQSPortals |= (1 << 15);
			SET_GAME_STATE(KNOW_QS_PORTAL, KnownQSPortals);
		}
		else
		{
			// Transition from QuasiSpace to HyperSpace through
			// one of the permanent portals.
			uint16_t index {};
			// JSD - portal_map can be used to find the exit point
			//const GFXPOINT portal_pt[] =
			//		QUASISPACE_PORTALS_HYPERSPACE_ENDPOINTS;

			index = CurStarDescPtr - &star_array[NUM_SOLAR_SYSTEMS + 1];
			//GLOBAL_SIS(log_x) = UNIVERSE_TO_LOGX(portal_pt[index].x);
			//GLOBAL_SIS(log_y) = UNIVERSE_TO_LOGY(portal_pt[index].y);
			GLOBAL_SIS(log_x) = UNIVERSE_TO_LOGX(portal_map[index].star_pt.x);
			GLOBAL_SIS(log_y) = UNIVERSE_TO_LOGY(portal_map[index].star_pt.y);

			// JMS: This QS portal's HS coordinates are revealed on QS map
			// the next time the player visits QS.
			KnownQSPortals |= (1 << index);
			SET_GAME_STATE(KNOW_QS_PORTAL, KnownQSPortals);

			SET_GAME_STATE(ARILOU_SPACE_SIDE, 0);
		}
	}
}

/* Enter QuasiSpace from HyperSpace by any portal, or HyperSpace from
 * QuasiSpace through the periodically opening portal.
 */
static void
ArilouSpaceTransition(void)
{
	uqm::flagship().shipFacing = 0; /* Not reentering the system */
	SET_GAME_STATE(USED_BROADCASTER, 0);
	uqm::flagship().autopilot.clear();
	if (GET_GAME_STATE(ARILOU_SPACE_SIDE) <= 1)
	{
		// From HyperSpace to QuasiSpace.
		GLOBAL_SIS(log_x) = UNIVERSE_TO_LOGX(QUASI_SPACE_X);
		GLOBAL_SIS(log_y) = UNIVERSE_TO_LOGY(QUASI_SPACE_Y);
		if (GET_GAME_STATE(PORTAL_COUNTER) == 0)
		{
			// Periodically appearing portal.
			SET_GAME_STATE(ARILOU_SPACE_SIDE, 3);

			if (EXTENDED)
			{
				ZeroLastLoc();
			}
		}
		else
		{
			// Player-induced portal.
			SET_GAME_STATE(PORTAL_COUNTER, 0);
			SET_GAME_STATE(ARILOU_SPACE_SIDE, 3);
		}
	}
	else
	{
		// From QuasiSpace to HyperSpace through the periodically appearing
		// portal.
		if (!EXTENDED || (EXTENDED && !ValidPoint(LoadLastLoc())))
		{
			// JSD Converting hard coded ARILOU portal to plot based
			//GLOBAL_SIS (log_x) = UNIVERSE_TO_LOGX (ARILOU_SPACE_X);
			//GLOBAL_SIS (log_y) = UNIVERSE_TO_LOGY (ARILOU_SPACE_Y);
			GLOBAL_SIS(log_x) = UNIVERSE_TO_LOGX(plot_map[ARILOU_DEFINED].star_pt.x);
			GLOBAL_SIS(log_y) = UNIVERSE_TO_LOGY(plot_map[ARILOU_DEFINED].star_pt.y);
		}
		else if (EXTENDED && ValidPoint(LoadLastLoc()))
		{
			GLOBAL_SIS(log_x) = UNIVERSE_TO_LOGX(LoadLastLoc().x);
			GLOBAL_SIS(log_y) = UNIVERSE_TO_LOGY(LoadLastLoc().y);

			ZeroLastLoc();
		}
		SET_GAME_STATE(ARILOU_SPACE_SIDE, 0);
	}
}

static void
unhyper_transition(ELEMENT* ElementPtr)
{
	// JMS: If leaving interplanetary on autopilot, always arrive HS with
	// the ship's nose pointed into correct direction.
	if (uqm::UQMOptions::read().smartAutoPilot
		&& (uqm::flagship().autopilot.hasTarget()))
	{
		STARSHIP* StarShipPtr {};
		GFXPOINT universe;
		int16_t facing {};
		int32_t udx = 0;
		int32_t udy = 0;

		GetElementStarShip(ElementPtr, &StarShipPtr);
		universe.x = LOGX_TO_UNIVERSE(GLOBAL_SIS(log_x));
		universe.y = LOGY_TO_UNIVERSE(GLOBAL_SIS(log_y));
		udx = uqm::flagship().autopilot.target().x - universe.x;
		udy = -(uqm::flagship().autopilot.target().y - universe.y);

		facing = NORMALIZE_FACING(ANGLE_TO_FACING(ARCTAN(udx, udy)));
		StarShipPtr->ShipFacing = facing;
		SetElementStarShip(ElementPtr, StarShipPtr);
	}

	ElementPtr->state_flags |= CHANGING;

	uint16_t frame_index {GetFrameIndex(ElementPtr->current.image.frame)};
	if (frame_index == 0)
	{
		frame_index += ANGLE_TO_FACING(FULL_CIRCLE);
	}
	else if (frame_index < ANGLE_TO_FACING(FULL_CIRCLE))
	{
		frame_index = NORMALIZE_FACING(frame_index + 1);
	}
	else if (++frame_index == GetFrameCount(ElementPtr->current.image.frame))
	{
		cleanup_hyperspace();

		uqm::currentActivity() &= ~IN_BATTLE;
		switch ((TRANSITION_TYPE)ElementPtr->turn_wait)
		{
			case RANDOM_ENCOUNTER_TRANSITION:
				SaveSisHyperState();
				SetBattlePlanet();
				uqm::currentActivity() |= START_ENCOUNTER;
				break;
			case INTERPLANETARY_TRANSITION:
				InterplanetaryTransition(ElementPtr);
				break;
			case ARILOU_SPACE_TRANSITION:
				ArilouSpaceTransition();
				break;
		}

		ZeroVelocityComponents(&ElementPtr->velocity);
		SetPrimType(&DisplayArray[ElementPtr->PrimIndex], NO_PRIM);
		return;
	}
	ElementPtr->next.image.frame =
		SetAbsFrameIndex(ElementPtr->current.image.frame,
						 frame_index);
}

static void
init_transition(ELEMENT* ElementPtr0, ELEMENT* ElementPtr1,
				TRANSITION_TYPE which_transition)
{
	int16_t dx {};
	int16_t dy {};
	int16_t num_turns {};
	STARSHIP* StarShipPtr {};

	dx = WORLD_TO_VELOCITY(ElementPtr0->next.location.x
						   - ElementPtr1->next.location.x);
	dy = WORLD_TO_VELOCITY(ElementPtr0->next.location.y
						   - ElementPtr1->next.location.y);

	ElementPtr1->state_flags |= NONSOLID;
	ElementPtr1->preprocess_func = unhyper_transition;
	ElementPtr1->postprocess_func = nullptr;
	ElementPtr1->turn_wait = (uint8_t)which_transition;

	GetElementStarShip(ElementPtr1, &StarShipPtr);
	num_turns = GetFrameCount(ElementPtr1->next.image.frame)
			  - ANGLE_TO_FACING(FULL_CIRCLE)
			  + NORMALIZE_FACING(ANGLE_TO_FACING(FULL_CIRCLE)
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
	const GFXPOINT QuasiPilot = LoadAdvancedQuasiPilot();
	const GFXPOINT SavedPilot = LoadAdvancedAutoPilot();

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

bool hyper_transition(ELEMENT* ElementPtr)
{
	if (ElementPtr->state_flags & APPEARING)
	{
		if (LastActivity & CHECK_LOAD)
		{
			LastActivity &= ~CHECK_LOAD;

			ElementPtr->current = ElementPtr->next;
			SetUpElement(ElementPtr);

			ElementPtr->state_flags |= DEFY_PHYSICS;

			return false;
		}
		else
		{
			ElementPtr->preprocess_func =
				(void (*)(struct element* ElementPtr))hyper_transition;
			ElementPtr->postprocess_func = nullptr;
			ElementPtr->state_flags |= NONSOLID;
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
			STARSHIP* StarShipPtr {};
			GFXPOINT universe;
			int16_t facing {};
			int32_t udx = 0;
			int32_t udy = 0;

			GetElementStarShip(ElementPtr, &StarShipPtr);
			universe.x = LOGX_TO_UNIVERSE(GLOBAL_SIS(log_x));
			universe.y = LOGY_TO_UNIVERSE(GLOBAL_SIS(log_y));
			udx = uqm::flagship().autopilot.target().x - universe.x;
			udy = -(uqm::flagship().autopilot.target().y - universe.y);

			facing =
				NORMALIZE_FACING(ANGLE_TO_FACING(ARCTAN(udx, udy)));
			StarShipPtr->ShipFacing = facing;
			SetElementStarShip(ElementPtr, StarShipPtr);
		}

		uint16_t frame_index {GetFrameIndex(ElementPtr->current.image.frame)};
		if (frame_index-- <= ANGLE_TO_FACING(FULL_CIRCLE))
		{
			STARSHIP* StarShipPtr {};

			if (frame_index == ANGLE_TO_FACING(FULL_CIRCLE) - 1)
			{
				frame_index = 0;
			}
			else
			{
				frame_index = NORMALIZE_FACING(frame_index);
			}

			GetElementStarShip(ElementPtr, &StarShipPtr);
			if (frame_index == StarShipPtr->ShipFacing)
			{
				ElementPtr->preprocess_func = ship_preprocess;
				ElementPtr->postprocess_func = ship_postprocess;
				ElementPtr->state_flags &= ~NONSOLID;
			}
		}

		ElementPtr->state_flags |= CHANGING;
		ElementPtr->next.image.frame =
			SetAbsFrameIndex(ElementPtr->current.image.frame,
							 frame_index);

		if (!(ElementPtr->state_flags & NONSOLID))
		{
			ElementPtr->current = ElementPtr->next;
			SetUpElement(ElementPtr);

			ElementPtr->state_flags |= DEFY_PHYSICS;

			if (uqm::UQMOptions::read().advancedAutoPilot)
			{
				DoAdvancedAutoPilot();
			}
		}
	}

	return true;
}

static void
hyper_collision(ELEMENT* ElementPtr0, GFXPOINT* pPt0,
				ELEMENT* ElementPtr1, GFXPOINT* pPt1)
{
	if ((ElementPtr1->state_flags & PLAYER_SHIP)
		&& GET_GAME_STATE(PORTAL_COUNTER) == 0)
	{
		int16_t dx {};
		int16_t dy {};
		GFXPOINT pt;
		STARSHIP* StarShipPtr {};

		ElementToUniverse(ElementPtr0, &pt);

		STAR_DESC* SDPtr {FindStar(nullptr, &pt, 5, 5)};

		GetElementStarShip(ElementPtr1, &StarShipPtr);
		GetCurrentVelocityComponents(&ElementPtr1->velocity, &dx, &dy);
		if (SDPtr == CurStarDescPtr
			|| (ElementPtr1->state_flags & APPEARING)
			|| !(dx || dy || (StarShipPtr->cur_status_flags & (LEFT | RIGHT | THRUST | WEAPON | SPECIAL))))
		{
			CurStarDescPtr = SDPtr;
			ElementPtr0->state_flags |= DEFY_PHYSICS | COLLISION;
		}
		else if ((uqm::currentActivity() & IN_BATTLE)
				 && (!uqm::flagship().autopilot.hasTarget()
					 || (uqm::flagship().autopilot.target().x == SDPtr->star_pt.x
						 && uqm::flagship().autopilot.target().y == SDPtr->star_pt.y)))
		{
			CurStarDescPtr = SDPtr;
			ElementPtr0->state_flags |= COLLISION;

			init_transition(ElementPtr0, ElementPtr1,
							INTERPLANETARY_TRANSITION);
		}
	}
	(void)pPt0; /* Satisfying compiler (unused parameter) */
	(void)pPt1; /* Satisfying compiler (unused parameter) */
}

static void
hyper_death(ELEMENT* ElementPtr)
{
	if (!(ElementPtr->state_flags & DEFY_PHYSICS)
		&& (uqm::currentActivity() & IN_BATTLE))
	{
		CurStarDescPtr = 0;
	}
}

static void
arilou_space_death(ELEMENT* ElementPtr)
{
	if (!(ElementPtr->state_flags & DEFY_PHYSICS)
		|| GET_GAME_STATE(ARILOU_SPACE_COUNTER) == 0)
	{
		if (GET_GAME_STATE(ARILOU_SPACE_SIDE) <= 1)
		{
			SET_GAME_STATE(ARILOU_SPACE_SIDE, 0);
		}
		else
		{
			SET_GAME_STATE(ARILOU_SPACE_SIDE, 3);
		}
	}
}

static void
arilou_space_collision(ELEMENT* ElementPtr0,
					   GFXPOINT* pPt0, ELEMENT* ElementPtr1, GFXPOINT* pPt1)
{
	uint16_t which_side {};

	if (!(ElementPtr1->state_flags & PLAYER_SHIP))
	{
		return;
	}

	which_side = GET_GAME_STATE(ARILOU_SPACE_SIDE);
	if (which_side == 0 || which_side == 3)
	{
		if (ElementPtr1->state_flags & DEFY_PHYSICS)
		{
			SET_GAME_STATE(ARILOU_SPACE_SIDE, which_side ^ 1);
		}
		else
		{
			init_transition(ElementPtr0, ElementPtr1,
							ARILOU_SPACE_TRANSITION);
		}
	}

	ElementPtr0->state_flags |= DEFY_PHYSICS | COLLISION;
	(void)pPt0; /* Satisfying compiler (unused parameter) */
	(void)pPt1; /* Satisfying compiler (unused parameter) */
}

static HELEMENT
AllocHyperElement(const GFXPOINT* elem_pt)
{
	HELEMENT hHyperSpaceElement {AllocElement()};
	if (hHyperSpaceElement)
	{
		ELEMENT* HyperSpaceElementPtr {};

		LockElement(hHyperSpaceElement, &HyperSpaceElementPtr);
		HyperSpaceElementPtr->playerNr = NEUTRAL_PLAYER_NUM;
		HyperSpaceElementPtr->state_flags = CHANGING | FINITE_LIFE;
		HyperSpaceElementPtr->life_span = 1;
		HyperSpaceElementPtr->mass_points = 1;

		{
			long lx {};
			long ly {};

			lx = UNIVERSE_TO_LOGX(elem_pt->x)
			   + (LOG_SPACE_WIDTH >> 1) - GLOBAL_SIS(log_x);
			HyperSpaceElementPtr->current.location.x = WRAP_X(lx);

			ly = UNIVERSE_TO_LOGY(elem_pt->y)
			   + (LOG_SPACE_HEIGHT >> 1) - GLOBAL_SIS(log_y);
			HyperSpaceElementPtr->current.location.y = WRAP_Y(ly);
		}

		SetPrimType(&DisplayArray[HyperSpaceElementPtr->PrimIndex],
					STAMP_PRIM);
		HyperSpaceElementPtr->current.image.farray =
			&hyperstars[1 + (GET_GAME_STATE(ARILOU_SPACE_SIDE) >> 1)];

		UnlockElement(hHyperSpaceElement);
	}

	return hHyperSpaceElement;
}

static void
AddAmbientElement(void)
{
	HELEMENT hHyperSpaceElement {AllocElement()};
	if (hHyperSpaceElement)
	{
		int16_t dx {};
		int16_t dy {};
		ELEMENT* HyperSpaceElementPtr {};

		LockElement(hHyperSpaceElement, &HyperSpaceElementPtr);
		HyperSpaceElementPtr->playerNr = NEUTRAL_PLAYER_NUM;
		HyperSpaceElementPtr->state_flags =
			APPEARING | FINITE_LIFE | NONSOLID;
		SetPrimType(&DisplayArray[HyperSpaceElementPtr->PrimIndex],
					STAMP_PRIM);
		HyperSpaceElementPtr->preprocess_func = animation_preprocess;

		if (IS_HD && inQuasiSpace())
		{
			SetPrimFlags(&DisplayArray[HyperSpaceElementPtr->PrimIndex],
						 HYPER_TO_QUASI_COLOR);
		}

		const uint32_t rand_val {uqm::random().next()};
		dy = uqm::lowWord(rand_val);

		if (!IS_HD)
		{
			dx = (int16_t)(uqm::lowByte(dy) % SPACE_WIDTH) - (SPACE_WIDTH >> 1);
			dy = (int16_t)(uqm::highByte(dy) % SPACE_HEIGHT) - (SPACE_HEIGHT >> 1);
		}
		else
		{
			dx = (int16_t)((uqm::highWord(rand_val)) % SPACE_WIDTH)
			   - (SPACE_WIDTH >> 1);
			dy = (int16_t)(dy % SPACE_HEIGHT) - (SPACE_HEIGHT >> 1);
		}

		HyperSpaceElementPtr->current.location.x =
			(LOG_SPACE_WIDTH >> 1) + DISPLAY_TO_WORLD(dx);
		HyperSpaceElementPtr->current.location.y =
			(LOG_SPACE_HEIGHT >> 1) + DISPLAY_TO_WORLD(dy);
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

#define NUM_VORTEX_TRANSITIONS (ANIMATED_HYPERSPACE ? 18 : 9)
#define VORTEX_WAIT 1
/* Hyperspace Animation related */
#define BASE_VORTEX_FRAME_INDEX 139					  // Kruzen: first frame of actual battle group vortex, not the birth/death
#define TR_COMPENSATION (ANIMATED_HYPERSPACE ? 8 : 0) // Kruzen: transition_state counts down from 100 when reforming a vortex, but reaching 9 with
// standard option it stars count down every other frame, effectively adding 8 frames of waiting

// Kruzen: preprocess func for animated hole
static void
encounter_animation(ELEMENT* ElementPtr)
{
	const uint16_t f_index = GetFrameIndex(ElementPtr->current.image.frame);

	if ((f_index >= BASE_VORTEX_FRAME_INDEX + 31))
	{
		ElementPtr->next.image.frame =
			SetAbsFrameIndex(ElementPtr->current.image.frame, BASE_VORTEX_FRAME_INDEX); // reached max - reset to base frame
	}
	else
	{
		ElementPtr->next.image.frame =
			IncFrameIndex(ElementPtr->current.image.frame);
	}
}

static void
encounter_transition(ELEMENT* ElementPtr)
{
	ElementPtr->state_flags &= ~DISAPPEARING;
	ElementPtr->life_span = 1;
	if (ElementPtr->turn_wait)
	{
		--ElementPtr->turn_wait;
	}
	else
	{
		FRAME f {};

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
				if (ANIMATED_HYPERSPACE)
				{
					ElementPtr->preprocess_func = encounter_animation;
				}
				ElementPtr->death_func = nullptr;
			}
		}

		ElementPtr->turn_wait = (ANIMATED_HYPERSPACE ? 0 : VORTEX_WAIT);
	}
}

static HELEMENT
getSisElement(void)
{
	HELEMENT hShip {};

	HSTARSHIP hSis {GetHeadLink(&race_q[RPG_PLAYER_NUM])};
	if (!hSis)
	{
		return nullptr;
	}

	const STARSHIP* StarShipPtr {LockStarShip(&race_q[RPG_PLAYER_NUM], hSis)};
	hShip = StarShipPtr->hShip;
	UnlockStarShip(&race_q[RPG_PLAYER_NUM], hSis);

#ifdef DEBUG
	{
		ELEMENT* ElementPtr;
		LockElement(hShip, &ElementPtr);
		assert(ElementPtr->state_flags & PLAYER_SHIP);
		UnlockElement(hShip);
	}
#endif

	return hShip;
}

static void
encounter_collision(ELEMENT* ElementPtr0, GFXPOINT* pPt0,
					ELEMENT* ElementPtr1, GFXPOINT* pPt1)
{
	HENCOUNTER hEncounter {};
	HENCOUNTER hNextEncounter {};

	if (!(ElementPtr1->state_flags & PLAYER_SHIP)
		|| !(uqm::currentActivity() & IN_BATTLE))
	{
		return;
	}

	init_transition(ElementPtr0, ElementPtr1,
					RANDOM_ENCOUNTER_TRANSITION);

	for (hEncounter = GetHeadEncounter();
		 hEncounter != 0; hEncounter = hNextEncounter)
	{
		ENCOUNTER* EncounterPtr {};

		LockEncounter(hEncounter, &EncounterPtr);
		hNextEncounter = GetSuccEncounter(EncounterPtr);
		if (EncounterPtr->hElement)
		{
			ELEMENT* ElementPtr {};

			LockElement(EncounterPtr->hElement, &ElementPtr);
			ElementPtr->state_flags |= NONSOLID | IGNORE_SIMILAR;
			UnlockElement(EncounterPtr->hElement);
		}
		UnlockEncounter(hEncounter);
	}

	// Mark this element as collided with flagship
	// XXX: We could simply set hTarget to 1 or to ElementPtr1,
	//   but that would be too hacky ;)
	ElementPtr0->hTarget = getSisElement();
	ZeroVelocityComponents(&ElementPtr0->velocity);

	(void)pPt0; /* Satisfying compiler (unused parameter) */
	(void)pPt1; /* Satisfying compiler (unused parameter) */
}

static HELEMENT
AddEncounterElement(ENCOUNTER* EncounterPtr, GFXPOINT* puniverse)
{
	bool NewEncounter {};
	GFXPOINT enc_pt;

	if (GET_GAME_STATE(ARILOU_SPACE_SIDE) >= 2)
	{
		return 0;
	}

	if (EncounterPtr->flags & ENCOUNTER_REFORMING)
	{
		EncounterPtr->flags &= ~ENCOUNTER_REFORMING;

		EncounterPtr->transition_state = 100 + TR_COMPENSATION;
		if ((EncounterPtr->flags & ONE_SHOT_ENCOUNTER)
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
		const uint8_t EncounterMakeup[] =
			{
				RACE_ENCOUNTER_MAKEUP};

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

		if (NumShips > MAX_HYPER_SHIPS)
		{
			NumShips = MAX_HYPER_SHIPS;
		}

		EncounterPtr->num_ships = NumShips;
		for (i = 0; i < NumShips; ++i)
		{
			BRIEF_SHIP_INFO* BSIPtr = &EncounterPtr->ShipList[i];
			HFLEETINFO hStarShip =
				GetStarShipFromIndex(&GLOBAL(avail_race_q), Type);
			const FLEET_INFO* FleetPtr =
				LockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
			BSIPtr->race_id = Type;
			BSIPtr->crew_level = FleetPtr->crew_level;
			BSIPtr->max_crew = FleetPtr->max_crew;
			BSIPtr->max_energy = FleetPtr->max_energy;
			UnlockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
		}

		do
		{
			const uint32_t rand_val {uqm::random().next()};

			enc_pt.x = puniverse->x
					 + (uqm::lowWord(rand_val) % (XOFFS << 1)) - XOFFS;
			if (enc_pt.x < 0)
			{
				enc_pt.x = 0;
			}
			else if (enc_pt.x > MAX_X_UNIVERSE)
			{
				enc_pt.x = MAX_X_UNIVERSE;
			}
			enc_pt.y = puniverse->y
					 + (uqm::highWord(rand_val) % (YOFFS << 1)) - YOFFS;
			if (enc_pt.y < 0)
			{
				enc_pt.y = 0;
			}
			else if (enc_pt.y > MAX_Y_UNIVERSE)
			{
				enc_pt.y = MAX_Y_UNIVERSE;
			}

			dx = enc_pt.x - EncounterPtr->origin.x;
			dy = enc_pt.y - EncounterPtr->origin.y;
		} while ((uint32_t)((long)dx * dx + (long)dy * dy) > radius_squared);

		EncounterPtr->loc_pt = enc_pt;
		EncounterPtr->log_x = UNIVERSE_TO_LOGX(enc_pt.x);
		EncounterPtr->log_y = UNIVERSE_TO_LOGY(enc_pt.y);
	}

	HELEMENT hElement {AllocHyperElement(&enc_pt)};
	if (hElement)
	{
		ELEMENT* ElementPtr {};

		LockElement(hElement, &ElementPtr);

		int16_t i {EncounterPtr->transition_state};
		if (i || NewEncounter)
		{
			if (i < 0)
			{
				i = -i;
				ElementPtr->hit_points = 1;
			}
			if (i == 0 || i > NUM_VORTEX_TRANSITIONS)
			{
				i = NUM_VORTEX_TRANSITIONS;
			}

			ElementPtr->current.image.frame = SetRelFrameIndex(
				ElementPtr->current.image.farray[0], -i);
			ElementPtr->death_func = encounter_transition;
			ElementPtr->preprocess_func = nullptr;
		}
		else
		{
			if (ANIMATED_HYPERSPACE)
			{
				ElementPtr->preprocess_func = encounter_animation;
				ElementPtr->current.image.frame = SetAbsFrameIndex(
					ElementPtr->current.image.farray[0], BASE_VORTEX_FRAME_INDEX + (uqm::random().next() % 31));
			}
			else
			{
				ElementPtr->current.image.frame = DecFrameIndex(
					ElementPtr->current.image.farray[0]);
				ElementPtr->preprocess_func = nullptr;
			}
		}

		ElementPtr->turn_wait = VORTEX_WAIT;
		ElementPtr->postprocess_func = nullptr; // decorate_vortex;
		ElementPtr->collision_func = encounter_collision;

		SetUpElement(ElementPtr);

		ElementPtr->IntersectControl.IntersectStamp.frame =
			DecFrameIndex(stars_in_space);
		SetPrimType(&DisplayArray[ElementPtr->PrimIndex], NO_PRIM);
		ElementPtr->state_flags |= NONSOLID | IGNORE_VELOCITY;

		UnlockElement(hElement);

		InsertElement(hElement, GetTailElement());
	}

	EncounterPtr->hElement = hElement;
	return hElement;
}

#define GRID_OFFSET 200

static void
DrawHyperGrid(int16_t ux, int16_t uy, int16_t ox, int16_t oy)
{
	int16_t sx {};
	int16_t sy {};
	int16_t ex {};
	int16_t ey {};
	GFXRECT r;

	ClearDrawable();
	SetContextForeGroundColor(
		BuildColor(MakeRgb15(0x00, 0x10, 0x00), 0x6B));

	sx = ux - (RADAR_SCAN_WIDTH >> 1);
	if (sx < 0)
	{
		sx = 0;
	}
	else
	{
		sx -= sx % GRID_OFFSET;
	}
	ex = ux + (RADAR_SCAN_WIDTH >> 1);
	if (ex > MAX_X_UNIVERSE + 1)
	{
		ex = MAX_X_UNIVERSE + 1;
	}

	sy = uy - (RADAR_SCAN_HEIGHT >> 1);
	if (sy < 0)
	{
		sy = 0;
	}
	else
	{
		sy -= sy % GRID_OFFSET;
	}
	ey = uy + (RADAR_SCAN_HEIGHT >> 1);
	if (ey > MAX_Y_UNIVERSE + 1)
	{
		ey = MAX_Y_UNIVERSE + 1;
	}

	r.corner.y = (int16_t)((long)(MAX_Y_UNIVERSE - ey)
						   * RADAR_HEIGHT / RADAR_SCAN_HEIGHT)
			   - oy;
	r.extent.width = RES_SCALE(1);
	r.extent.height = ((int16_t)((long)(MAX_Y_UNIVERSE - sy)
								 * RADAR_HEIGHT / RADAR_SCAN_HEIGHT)
					   - oy)
					- r.corner.y
					+ RES_SCALE(1);
	for (ux = sx; ux <= ex; ux += GRID_OFFSET)
	{
		r.corner.x = (int16_t)((long)ux * RADAR_WIDTH / RADAR_SCAN_WIDTH)
				   - ox;
		DrawFilledRectangle(&r);
	}

	r.corner.x = (int16_t)((long)sx * RADAR_WIDTH / RADAR_SCAN_WIDTH) - ox;
	r.extent.width = ((int16_t)((long)ex * RADAR_WIDTH / RADAR_SCAN_WIDTH)
					  - ox)
				   - r.corner.x + RES_SCALE(1);
	r.extent.height = RES_SCALE(1);
	for (uy = sy; uy <= ey; uy += GRID_OFFSET)
	{
		r.corner.y = (int16_t)((long)(MAX_Y_UNIVERSE - uy)
							   * RADAR_HEIGHT / RADAR_SCAN_HEIGHT)
				   - oy;
		DrawFilledRectangle(&r);
	}
}

// Returns false if the encounter is to be removed.
static bool
ProcessEncounter(ENCOUNTER* EncounterPtr, GFXPOINT* puniverse,
				 int16_t ox, int16_t oy, STAMP* stamp)
{
	ELEMENT* ElementPtr {};
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
			if (EncounterPtr->transition_state >= NUM_VORTEX_TRANSITIONS)
			{
				++ElementPtr->turn_wait;
			}
			else if (EncounterPtr->transition_state == -NUM_VORTEX_TRANSITIONS)
			{
				ElementPtr->death_func = nullptr;
				UnlockElement(EncounterPtr->hElement);
				return false;
			}
			else
			{
				SetPrimType(&DisplayArray[ElementPtr->PrimIndex],
							STAMP_PRIM);
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
		{ // This is an encounter that did not collide with flagship
			// The colliding encounter does not move
			uint16_t cur_facing {};
			uint16_t delta_facing {};

			cur_facing = ANGLE_TO_FACING(
				GetVelocityTravelAngle(&ElementPtr->velocity));
			delta_facing = NORMALIZE_FACING(cur_facing - ANGLE_TO_FACING(ARCTAN(puniverse->x - EncounterPtr->loc_pt.x, puniverse->y - EncounterPtr->loc_pt.y)));
			if (delta_facing || (delta_x == 0 && delta_y == 0))
			{
#define ENCOUNTER_TRACK_WAIT 3
				int16_t speed {RaceHyperSpeed((RACE_ID)EncounterPtr->race_id)};
				if (delta_facing < ANGLE_TO_FACING(HALF_CIRCLE))
				{
					--cur_facing;
				}
				else
				{
					++cur_facing;
				}
				if (NORMALIZE_FACING(delta_facing
									 + ANGLE_TO_FACING(OCTANT))
					> ANGLE_TO_FACING(QUADRANT))
				{
					if (delta_facing < ANGLE_TO_FACING(HALF_CIRCLE))
					{
						--cur_facing;
					}
					else
					{
						++cur_facing;
					}
					speed >>= 1;
				}
				cur_facing = FACING_TO_ANGLE(cur_facing);
				SetVelocityComponents(&ElementPtr->velocity,
									  COSINE(cur_facing, speed),
									  SINE(cur_facing, speed));
				GetNextVelocityComponents(&ElementPtr->velocity,
										  &delta_x, &delta_y, 1);

				ElementPtr->thrust_wait = ENCOUNTER_TRACK_WAIT;
			}
		}
		EncounterPtr->log_x += delta_x;
		EncounterPtr->log_y -= delta_y;
		EncounterPtr->loc_pt.x = LOGX_TO_UNIVERSE(EncounterPtr->log_x);
		EncounterPtr->loc_pt.y = LOGY_TO_UNIVERSE(EncounterPtr->log_y);

		encounter_radius = EncounterPtr->radius + (GRID_OFFSET >> 1);
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
			ElementPtr->state_flags |= NONSOLID;
			ElementPtr->life_span = 0;

			if (EncounterPtr->transition_state == 0)
			{
				ElementPtr->death_func = encounter_transition;
				EncounterPtr->transition_state = -1;
				ElementPtr->hit_points = 1;

				if (ANIMATED_HYPERSPACE)
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
	if (ex - puniverse->x >= -UNIT_SCREEN_WIDTH
		&& ex - puniverse->x <= UNIT_SCREEN_WIDTH
		&& ey - puniverse->y >= -UNIT_SCREEN_HEIGHT
		&& ey - puniverse->y <= UNIT_SCREEN_HEIGHT)
	{
		ElementPtr->next.location.x =
			(int16_t)(EncounterPtr->log_x - GLOBAL_SIS(log_x))
			+ (LOG_SPACE_WIDTH >> 1);
		ElementPtr->next.location.y =
			(int16_t)(EncounterPtr->log_y - GLOBAL_SIS(log_y))
			+ (LOG_SPACE_HEIGHT >> 1);
		if ((ElementPtr->state_flags & NONSOLID)
			&& EncounterPtr->transition_state == 0)
		{
			ElementPtr->current.location = ElementPtr->next.location;
			SetPrimType(&DisplayArray[ElementPtr->PrimIndex],
						STAMP_PRIM);
			if (ElementPtr->death_func == 0)
			{
				InitIntersectStartPoint(ElementPtr);
				ElementPtr->state_flags &= ~NONSOLID;
			}
		}
	}
	else
	{
		ElementPtr->state_flags |= NONSOLID;
		if (ex - puniverse->x < -XOFFS || ex - puniverse->x > XOFFS
			|| ey - puniverse->y < -YOFFS || ey - puniverse->y > YOFFS)
		{
			ElementPtr->life_span = 0;
			ElementPtr->death_func = nullptr;
			UnlockElement(EncounterPtr->hElement);
			return false;
		}

		SetPrimType(&DisplayArray[ElementPtr->PrimIndex], NO_PRIM);
	}

	UnlockElement(EncounterPtr->hElement);

	stamp->origin.x = (int16_t)((long)ex * RADAR_WIDTH / RADAR_SCAN_WIDTH)
					- ox;
	stamp->origin.y = (int16_t)((long)(MAX_Y_UNIVERSE - ey) * RADAR_HEIGHT
								/ RADAR_SCAN_HEIGHT)
					- oy;
	DrawStamp(stamp);

	return true;
}

static void
ProcessEncounters(GFXPOINT* puniverse, int16_t ox, int16_t oy)
{
	STAMP stamp;
	HENCOUNTER hEncounter {};
	HENCOUNTER hNextEncounter {};

	stamp.frame = SetAbsFrameIndex(stars_in_space, 91);
	for (hEncounter = GetHeadEncounter();
		 hEncounter; hEncounter = hNextEncounter)
	{
		ENCOUNTER* EncounterPtr {};

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

#define NUM_FRAMES 32

static Color
GetMaskColor(uint8_t startype)
{
	switch (startype)
	{
		case RED_BODY:
			return RED_STAR_COLOR;
		case ORANGE_BODY:
			return ORANGE_STAR_COLOR;
		case YELLOW_BODY:
			return YELLOW_STAR_COLOR;
		case GREEN_BODY:
			return GREEN_STAR_COLOR;
		case BLUE_BODY:
			return BLUE_STAR_COLOR;
		default:
			return WHITE_COLOR;
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
	GFXPOINT universe;
	FRAME star_frame {};
	STAMP s;
	STAR_DESC* SDPtr {};
	HELEMENT hHyperSpaceElement {};
	ELEMENT* HyperSpaceElementPtr {};
	int32_t lx {};
	int32_t ly {};
	GFXRECT frameRect;
	DEXTENT img_log;

	static uint16_t frameCounter = 0; // BW

	universe.x = LOGX_TO_UNIVERSE(GLOBAL_SIS(log_x));
	universe.y = LOGY_TO_UNIVERSE(GLOBAL_SIS(log_y));

	FRAME blip_frame {SetAbsFrameIndex(stars_in_space, 90)};

	SetContext(RadarContext);
	BatchGraphics();

	ox = (int16_t)((long)universe.x * RADAR_WIDTH / RADAR_SCAN_WIDTH)
	   - (RADAR_WIDTH >> 1);
	oy = (int16_t)((long)(MAX_Y_UNIVERSE - universe.y)
				   * RADAR_HEIGHT / RADAR_SCAN_HEIGHT)
	   - (RADAR_HEIGHT >> 1);

	ex = (int16_t)((long)uqm::flagship().shipStamp.origin.x
				   * RADAR_WIDTH / RADAR_SCAN_WIDTH)
	   - (RADAR_WIDTH >> 1);
	ey = (int16_t)((long)(MAX_Y_UNIVERSE - uqm::flagship().shipStamp.origin.y)
				   * RADAR_HEIGHT / RADAR_SCAN_HEIGHT)
	   - (RADAR_HEIGHT >> 1);

	arilouSpaceCounter = GET_GAME_STATE(ARILOU_SPACE_COUNTER);
	arilouSpaceSide = GET_GAME_STATE(ARILOU_SPACE_SIDE);

	//	if (ox != ex || oy != ey)
	{
		DrawHyperGrid(universe.x, universe.y, ox, oy);

		{
			SDPtr = 0;
			while ((SDPtr = FindStar(SDPtr, &universe, XOFFS, YOFFS)))
			{
				ex = SDPtr->star_pt.x;
				ey = SDPtr->star_pt.y;
				uint8_t star_type {STAR_TYPE(SDPtr->Type)};
				if (arilouSpaceSide >= 2 && ex == ARILOU_HOME_X && ey == ARILOU_HOME_Y)
				{
					star_type = SUPER_GIANT_STAR;
				}

				s.origin.x = (int16_t)((long)ex * RADAR_WIDTH
									   / RADAR_SCAN_WIDTH)
						   - ox;
				s.origin.y = (int16_t)((long)(MAX_Y_UNIVERSE - ey)
									   * RADAR_HEIGHT / RADAR_SCAN_HEIGHT)
						   - oy;
				s.frame = SetRelFrameIndex(blip_frame,
										   star_type + 2);
				DrawStamp(&s);
			}
		}
	}

	portalCounter = GET_GAME_STATE(PORTAL_COUNTER);
	if (portalCounter || arilouSpaceCounter)
	{
		STAR_DESC SD[2];
		// This array is filled with the STAR_DESC's of
		// QuasiSpace portals that need to be taken into account.
		// i is set to the number of active portals (max 2).

		uint16_t i {0};
		if (portalCounter)
		{
			// A player-created QuasiSpace portal is opening.
			static GFXPOINT portal_pt;

			if (ANIMATED_HYPERSPACE)
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


			SET_GAME_STATE(PORTAL_COUNTER, portalCounter);
		}

		if (arilouSpaceCounter)
		{
			// The periodically appearing QuasiSpace portal is open.
			if (ANIMATED_HYPERSPACE)
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
				if (ANIMATED_HYPERSPACE)
				{
					SD[i].Index += 81;
				}
				SD[i].Index += 18;
				// JSD Converting hard coded ARILOU portal to plot based
				//SD[i].star_pt.x = ARILOU_SPACE_X;
				//SD[i].star_pt.y = ARILOU_SPACE_Y;
				SD[i].star_pt = plot_map[ARILOU_DEFINED].star_pt;
			}
			else
			{
				// The player is in QuasiSpace
				SD[i].star_pt.x = QUASI_SPACE_X;
				SD[i].star_pt.y = QUASI_SPACE_Y;
			}
			++i;
		}

		// Process the i portals from SD.
		do
		{
			--i;
			sx = SD[i].star_pt.x - universe.x + XOFFS;
			sy = SD[i].star_pt.y - universe.y + YOFFS;
			if (sx < 0 || sy < 0 || sx >= (XOFFS << 1)
				|| sy >= (YOFFS << 1))
			{
				continue;
			}

			ex = SD[i].star_pt.x;
			ey = SD[i].star_pt.y;
			s.origin.x = (int16_t)((long)ex * RADAR_WIDTH / RADAR_SCAN_WIDTH)
					   - ox;
			s.origin.y = (int16_t)((long)(MAX_Y_UNIVERSE - ey)
								   * RADAR_HEIGHT / RADAR_SCAN_HEIGHT)
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

			if (ex > (XOFFS / NUM_RADAR_SCREENS)
				|| ey > (YOFFS / NUM_RADAR_SCREENS))
			{
				continue;
			}

			hHyperSpaceElement = AllocHyperElement(&SD[i].star_pt);
			if (hHyperSpaceElement == 0)
			{
				continue;
			}

			LockElement(hHyperSpaceElement, &HyperSpaceElementPtr);

			if (ANIMATED_HYPERSPACE && ((SD[i].Index == 107 && arilouSpaceSide <= 1) || (SD[i].Index == 8 && arilouSpaceSide > 1)))
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
		while ((SDPtr = FindStar(SDPtr, &universe, XOFFS, YOFFS)))
		{
			const uint8_t star_type = SDPtr->Type;
			const uint8_t star_color = STAR_COLOR(star_type);

			lx = UNIVERSE_TO_LOGX(SDPtr->star_pt.x) - GLOBAL_SIS(log_x);
			ly = UNIVERSE_TO_LOGY(SDPtr->star_pt.y) - GLOBAL_SIS(log_y);

			if (ANIMATED_HYPERSPACE)
			{
				if ((arilouSpaceSide > 1))
				{
					star_frame = SetAbsFrameIndex(hyperstars[2],
												  frameCounter + ((star_color == YELLOW_BODY) ? 40 : 8));
				}
				else
				{
					star_frame = SetAbsFrameIndex(hyperstars[1],
												  STAR_TYPE(star_type) * NUM_FRAMES + frameCounter);
				}
			}
			else
			{
				star_frame = SetAbsFrameIndex(
					hyperstars[1 + (arilouSpaceSide >> 1)],
					STAR_TYPE(star_type) * NUM_STAR_COLORS
						+ star_color);
			}

			if (star_frame == nullptr)
			{
				continue;
			}

			GetFrameRect(star_frame, &frameRect);

			img_log.width = frameRect.extent.width << 1;
			img_log.height = frameRect.extent.height << 1;

			if ((lx + img_log.width) < -HS_WEST_EDGE || (lx - img_log.width) > HS_EAST_EDGE || (ly + img_log.height) < -HS_NORTH_EDGE || (ly - img_log.height) > HS_SOUTH_EDGE)
			{
				continue;
			}

			hHyperSpaceElement = AllocHyperElement(&SDPtr->star_pt);
			if (hHyperSpaceElement == 0)
			{
				continue;
			}

			LockElement(hHyperSpaceElement, &HyperSpaceElementPtr);

			if (ANIMATED_HYPERSPACE && !(arilouSpaceSide > 1)
				&& star_color != WHITE_BODY)
			{
				const uint16_t* PrimIndex {};

				HyperSpaceElementPtr->current.image.frame =
					SetAbsFrameIndex(hyperstars[1],
									 96 + STAR_TYPE(star_type));

				PrimIndex = &HyperSpaceElementPtr->PrimIndex;

				SetPrimType(&DisplayArray[*PrimIndex], STAMPFILL_PRIM);
				SetPrimColor(&DisplayArray[*PrimIndex],
							 GetMaskColor(STAR_COLOR(star_type)));
				SetPrimFlags(&DisplayArray[*PrimIndex], HS_STARMASK);

				HyperSpaceElementPtr->state_flags =
					APPEARING | FINITE_LIFE | NONSOLID;

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

			if (SDPtr == CurStarDescPtr
				&& GET_GAME_STATE(PORTAL_COUNTER) == 0)
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

	s.origin.x = RADAR_WIDTH >> 1;
	s.origin.y = RADAR_HEIGHT >> 1;
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

static bool
DoHyperspaceMenu(MENU_STATE* pMS)
{
	bool select = PulsedInputState.menu[KEY_MENU_SELECT];

	if ((uqm::currentActivity() & (CHECK_ABORT | CHECK_LOAD))
		|| GLOBAL_SIS(CrewEnlisted) == (uint16_t)~0)
	{
		return false;
	}

	const bool handled {DoMenuChooser(pMS, PM_STARMAP)};
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
		case EQUIP_DEVICE:
			select = DevicesMenu();
			if (GET_GAME_STATE(PORTAL_COUNTER))
			{ // A player-induced portal to QuasiSpace is opening
				return false;
			}
			if (uqm::currentActivity() & START_ENCOUNTER)
			{ // Selected Talking Pet, going into conversation
				return false;
			}
			break;
		case CARGO:
			CargoMenu();
			break;
		case ROSTER:
			select = RosterMenu();
			break;
		case GAME_MENU:
			if (!GameOptions())
			{
				return false; // abort or load
			}
			break;
		case STARMAP:
			StarMap();
			return false;
		case NAVIGATION:
			return false;
	}

	if (!(uqm::currentActivity() & CHECK_ABORT))
	{
		if (select)
		{ // 3DO menu jumps to NAVIGATE after a successful submenu run
			if (uqm::UQMOptions::read().whichMenu != uqm::EmulationMode::PC)
			{
				pMS->CurState = NAVIGATION;
			}
			DrawMenuStateStrings(PM_STARMAP, pMS->CurState);
		}
		SetFlashRect(SFR_MENU_3DO, false);
	}

	return true;
}

void HyperspaceMenu(void)
{
	Color OldColor;
	MENU_STATE MenuState;

	UnbatchGraphics();

	GFXCONTEXT OldContext {SetContext(SpaceContext)};
	OldColor = SetContextBackGroundColor(BLACK_COLOR);


	memset(&MenuState, 0, sizeof(MenuState));
	MenuState.InputFunc = DoHyperspaceMenu;
	MenuState.Initialized = true;
	MenuState.CurState = STARMAP;

	DrawMenuStateStrings(PM_STARMAP, STARMAP);
	SetFlashRect(SFR_MENU_3DO, false);

	SetMenuSounds(MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	DoInput(&MenuState, true);

	SetFlashRect(nullptr, false);

	SetContext(SpaceContext);

	if (!(uqm::currentActivity() & (CHECK_ABORT | CHECK_LOAD)))
	{
		ClearSISRect(CLEAR_SIS_RADAR);
		WaitForNoInput(c_gameSecond / 2, false);
	}

	SetContextBackGroundColor(OldColor);
	SetContext(OldContext);
	if (!(uqm::currentActivity() & IN_BATTLE))
	{
		cleanup_hyperspace();
	}

	BatchGraphics();
}

void SaveSisHyperState(void)
{
	ELEMENT* ElementPtr {};
	STARSHIP* StarShipPtr {};

	// Update 'GLOBAL (ShipFacing)' to the direction the flagship is facing
	HELEMENT hSisElement {getSisElement()};
	if (!hSisElement)
	{ // Happens when saving a game from Hyperspace encounter screen
		return;
	}
	//if (ElementPtr->state_flags & PLAYER_SHIP)
	LockElement(hSisElement, &ElementPtr);
	GetElementStarShip(ElementPtr, &StarShipPtr);
	// XXX: Solar system reentry test depends on ShipFacing != 0
	uqm::flagship().shipFacing = StarShipPtr->ShipFacing + 1;
	UnlockElement(hSisElement);
}
