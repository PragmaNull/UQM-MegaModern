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

#include "init.h"

#include "build.h"
#include "colors.h"
#include "cons_res.h"
#include "element.h"
#include "encount.h"
#include "globdata.h"
#include "hyper.h"
#include "libs/reslib.h"
#include "master.h"
#include "nameref.h"
#include "process.h"
#include "races.h"
#include "setup.h"
#include "tactrans.h"
#include "units.h"


namespace uqm
{
FrameRef stars_in_space;
uqstl::array<FrameRef, NumViews> stars_misc;
FrameRef scenery;
uqstl::array<FrameRef, NumViews> asteroid;
uqstl::array<FrameRef, NumViews> blast;
uqstl::array<FrameRef, NumViews> explosion;


bool load_animation(FrameRef* pixarray, ResourceName big_res, ResourceName med_res, ResourceName sml_res)
{
	DrawableRef d {LoadGraphic(big_res)};
	if (!d)
	{
		return false;
	}
	pixarray[0] = CaptureDrawable(d);

	if (med_res != NullResource)
	{
		d = LoadGraphic(med_res);
		if (!d)
		{
			return false;
		}
	}
	pixarray[1] = CaptureDrawable(d);

	if (sml_res != NullResource)
	{
		d = LoadGraphic(sml_res);
		if (!d)
		{
			return false;
		}
	}
	pixarray[2] = CaptureDrawable(d);

	return true;
}

/* Warning: Some ships (such as the Umgah) will alias their pixarrays,
   so we need to track to make sure that we do not double-free. */
bool free_image(FrameRef* pixarray)
{
	uint16_t j {};
	uqstl::array<void*, NumViews> already_freed;

	bool retval {true};
	for (uint16_t i {0}; i < NumViews; ++i)
	{
		if (pixarray[i] != nullptr)
		{
			bool ok = true;
			for (j = 0; j < i; j++)
			{
				if (already_freed[j] == pixarray[i])
				{
					ok = false;
					break;
				}
			}
			if (ok)
			{
				if (!DestroyDrawable(ReleaseDrawable(pixarray[i])))
				{
					retval = false;
				}
			}
			already_freed[i] = pixarray[i];
			pixarray[i] = nullptr;
		}
	}

	return (retval);
}

static uint8_t space_ini_cnt;

bool InitSpace(void)
{
	if ((space_ini_cnt++ == 0
		 && uqm::activityKind(uqm::currentActivity()) <= ActivityKind::InEncounter)
		|| optRequiresReload)
	{
		stars_in_space = CaptureDrawable(
			LoadGraphic(StarMaskPmapAnimRes));
		if (stars_in_space == nullptr)
		{
			return false;
		}

		scenery = CaptureDrawable(
			LoadGraphic(SceneryMaskPmapAnimRes));
		if (scenery == nullptr)
		{
			return false;
		}


		if (IsHd())
		{
			if (!load_animation(stars_misc.data(),
								StarmiskBigMaskPmapAnimRes,
								StarmiskMedMaskPmapAnimRes,
								StarmiskSmlMaskPmapAnimRes))
			{
				return false;
			}
		}

		if (!load_animation(explosion.data(),
							BoomBigMaskPmapAnimRes,
							BoomMedMaskPmapAnimRes,
							BoomSmlMaskPmapAnimRes))
		{
			return false;
		}

		if (!load_animation(blast.data(),
							BlastBigMaskPmapAnimRes,
							BlastMedMaskPmapAnimRes,
							BlastSmlMaskPmapAnimRes))
		{
			return false;
		}

		if (!load_animation(asteroid.data(),
							AsteroidBigMaskPmapAnimRes,
							AsteroidMedMaskPmapAnimRes,
							AsteroidSmlMaskPmapAnimRes))
		{
			return false;
		}
	}

	return true;
}

void UninitSpace(void)
{
	if (space_ini_cnt && --space_ini_cnt == 0)
	{
		free_image(blast.data());
		free_image(explosion.data());
		free_image(asteroid.data());

		free_image(stars_misc.data());

		DestroyDrawable(ReleaseDrawable(stars_in_space));
		stars_in_space = 0;

		DestroyDrawable(ReleaseDrawable(scenery));
		scenery = 0;
	}
}

static StarshipHandle BuildSIS(void)
{
	StarshipHandle hStarShip {Build(&race_q[0], SpeciesId::SisShip)};
	if (!hStarShip)
	{
		return 0;
	}
	Starship* StarShipPtr {LockStarShip(&race_q[0], hStarShip)};
	StarShipPtr->playerNr = RpgPlayerNum;
	StarShipPtr->captains_name_index = 0;
	UnlockStarShip(&race_q[0], hStarShip);

	return hStarShip;
}

static constexpr auto NumAsteroids {5};
static constexpr auto NumPlanets {1};

int16_t InitShips(void)
{
	int16_t num_ships {};

	InitSpace();

	if (inHQSpace())
	{
		ReinitQueue(&race_q[0]);
		ReinitQueue(&race_q[1]);

		BuildSIS();
		LoadHyperspace();

		// Kruzen: Moved and duped for melee to load correct stars in InitGalaxy()
		// Depended on stars_in_space frame, which is redeclared in LoadHyperspace()
		InitDisplayList();
		InitGalaxy();

		num_ships = 1;
	}
	else
	{
		uint16_t i {};
		GfxRect r;

		SetContext(SpaceContext);

		InitDisplayList();
		InitGalaxy();

		SetContextFGFrame(Screen);
		r.corner.x = SafeX();
		r.corner.y = SafeY();
		r.extent.width = SpaceWidth();
		r.extent.height = SpaceHeight();
		SetContextClipRect(&r);

		SetContextBackGroundColor(BlackColor);
		{
			const ContextScope contextScope {ScreenContext};

			SetContextBackGroundColor(BlackColor);
			ClearDrawable();
		}

		if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InLastBattle)
		{
			free_gravity_well();
		}
		else
		{
			for (i = 0; i < NumAsteroids; ++i)
			{
				spawn_asteroid(nullptr);
			}
			for (i = 0; i < NumPlanets; ++i)
			{
				spawn_planet();
			}
		}

		num_ships = NumSides;
	}

	// FlushInput ();

	return (num_ships);
}

// Count the crew elements in the display list.
static uint16_t CountCrewElements(void)
{
	ElementHandle hElement {};
	ElementHandle hNextElement {};

	uint16_t result {0};
	for (hElement = GetHeadElement();
		 hElement != 0; hElement = hNextElement)
	{
		Element* ElementPtr {};

		LockElement(hElement, &ElementPtr);
		hNextElement = GetSuccElement(ElementPtr);
		if (testFlag(ElementPtr->state_flags, ElementFlags::CrewObject))
		{
			++result;
		}

		UnlockElement(hElement);
	}

	return result;
}

void UninitShips(void)
{
	int i {};
	ElementHandle hElement {};
	ElementHandle hNextElement {};
	uqstl::array<Starship*, NumPlayers> SPtr;

	StopSound();

	UninitSpace();

	for (i = 0; i < SPtr.size(); ++i)
	{
		SPtr[i] = 0;
	}

	// Count the crew floating in space.
	const uint16_t crew_retrieved {CountCrewElements()};

	for (hElement = GetHeadElement();
		 hElement != 0; hElement = hNextElement)
	{
		Element* ElementPtr {};

		LockElement(hElement, &ElementPtr);
		hNextElement = GetSuccElement(ElementPtr);
		if ((testFlag(ElementPtr->state_flags, ElementFlags::PlayerShip))
			|| ElementPtr->death_func == new_ship)
		{
			Starship* StarShipPtr {};

			GetElementStarShip(ElementPtr, &StarShipPtr);

			// There should only be one ship left in battle.
			// He gets the crew still floating in space.
			if (StarShipPtr->RaceDescPtr->ship_info.crew_level)
			{
				if (crew_retrieved >= StarShipPtr->RaceDescPtr->ship_info.max_crew - StarShipPtr->RaceDescPtr->ship_info.crew_level)
				{
					StarShipPtr->RaceDescPtr->ship_info.crew_level =
						StarShipPtr->RaceDescPtr->ship_info.max_crew;
				}
				else
				{
					StarShipPtr->RaceDescPtr->ship_info.crew_level +=
						crew_retrieved;
				}
			}

			/* Record crew left after battle */
			StarShipPtr->crew_level =
				StarShipPtr->RaceDescPtr->ship_info.crew_level;
			SPtr[StarShipPtr->playerNr] = StarShipPtr;
			free_ship(StarShipPtr->RaceDescPtr, true, true);
			StarShipPtr->RaceDescPtr = 0;
		}
		UnlockElement(hElement);
	}

	uqm::currentActivity() &= ~ActivityFlag::InBattle;

	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InEncounter
		&& !(uqm::currentActivity() & ActivityFlag::CheckAbort))
	{
		// Encounter battle in full game.
		//   Record the crew left in the last ship standing. The crew left
		//   is first recorded into Starship.crew_level just a few lines
		//   above here.
		for (i = NumPlayers - 1; i >= 0; --i)
		{
			if (SPtr[i] && !FleetIsInfinite(i))
			{
				UpdateShipFragCrew(SPtr[i]);
			}
		}
	}

	if (uqm::activityKind(uqm::currentActivity()) != ActivityKind::InEncounter)
	{
		// Remove any ships left from the race queue.
		for (i = 0; i < race_q.size(); i++)
		{
			ReinitQueue(&race_q[i]);
		}

		if (inHQSpace())
		{
			FreeHyperspace();
		}
	}
}

void ReloadGameContent(void)
{
	//FreeKernel (); Crashes when going from HdResolutionFactor to SD
	ClearPlayerInputAll();
	UninitGameKernel();
	FreeMasterShipList();
	//TFB_UninitInput ();

	uqm::prepareContentDir(contentDirPath, addonDirPath, 0);

	if (LoadKernel(0, 0))
	{
		//TFB_InitInput (TFB_INPUTDRIVER_SDL, 0);
		LoadMasterShipList(TaskSwitch);
		TaskSwitch();
		InitGameKernel();
	}
}

} // namespace uqm
