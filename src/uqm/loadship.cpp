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

#include "build.h"
#include "coderes.h"
#include "corecode.h"
#include "globdata.h"
#include "nameref.h"
#include "races.h"
#include "init.h"
#include "libs/strings/strintrn.h"
#include "uqm/random/UQMRandom.h"

static RESOURCE code_resources[] = {
	NullResource,
	ArilouCode,
	ChmmrCode,
	HumanCode,
	OrzCode,
	PkunkCode,
	ShofixtiCode,
	SpathiCode,
	SupoxCode,
	ThraddashCode,
	UtwigCode,
	VuxCode,
	YehatCode,
	MelnormeCode,
	DruugeCode,
	IlwrathCode,
	MyconCode,
	SlylandroCode,
	UmgahCode,
	UrquanCode,
	ZoqfotpikCode,
	SyreenCode,
	KohrAhCode,
	AndrosynthCode,
	ChenjesuCode,
	MmrnmhrmCode,
	SisCode,
	SamatraCode,
	UrquanDroneCode};

SPECIES_ID ship[] = {CHMMR_ID, ORZ_ID, UTWIG_ID, YEHAT_ID,					// Capital
					 MYCON_ID, UR_QUAN_ID, KOHR_AH_ID, CHENJESU_ID,			// Capital
					 ARILOU_ID, PKUNK_ID, SPATHI_ID, SUPOX_ID, MELNORME_ID, // Escort
					 DRUUGE_ID, SLYLANDRO_ID, ANDROSYNTH_ID, MMRNMHRM_ID,	// Escort
					 EARTHLING_ID, SHOFIXTI_ID, THRADDASH_ID, VUX_ID,		// Scout
					 ILWRATH_ID, UMGAH_ID, ZOQFOTPIK_ID, SYREEN_ID};		// Scout

SPECIES_ID* capital = ship;
static constexpr auto NumCapitals {8};
SPECIES_ID* escort = &(ship[NumCapitals]);
static constexpr auto NumEscorts {9};
SPECIES_ID* scout = &(ship[NumCapitals + NumEscorts]);
static constexpr auto NumScouts {8};
static constexpr auto NumShips {(NumCapitals + NumEscorts + NumScouts)};

// Returns the array index in ship array for a specific SpeciesID.
static inline uint16_t ShipIndex(SPECIES_ID SpeciesID)
{
	for (uint16_t x = 0; x < NumShips; x++)
	{
		if (SpeciesID == ship[x])
		{
			return x;
		}
	}
	return (NumShips);
}

// Creates a ship map based on seed. We need up to two maps loaded
// at once to be able to handle the load/save game preview window.
static inline void SeedShipMap(SPECIES_ID* map, int seed)
{
	uqm::PrimeRandom ShipGenRNG {};
	uint16_t rand_val {};
	SPECIES_ID* cMap = map;
	SPECIES_ID* eMap = &(map[NumCapitals]);
	SPECIES_ID* sMap = &(map[NumCapitals + NumEscorts]);
	uint16_t x = 0;
	ShipGenRNG.reseed(seed);
	for (x = 0; x < NumShips; x++)
	{
		map[x] = NUM_SPECIES_ID;
	}
	for (x = 0; x < NumCapitals; x++)
	{
		rand_val = ShipGenRNG.next() % NumCapitals;
		while (cMap[rand_val] != NUM_SPECIES_ID)
		{
			rand_val = (rand_val + 1) % NumCapitals;
		}
		cMap[rand_val] = capital[x];
	}
	for (x = 0; x < NumEscorts; x++)
	{
		rand_val = ShipGenRNG.next() % NumEscorts;
		while (eMap[rand_val] != NUM_SPECIES_ID)
		{
			rand_val = (rand_val + 1) % NumEscorts;
		}
		eMap[rand_val] = escort[x];
	}
	for (x = 0; x < NumScouts; x++)
	{
		rand_val = ShipGenRNG.next() % NumScouts;
		while (sMap[rand_val] != NUM_SPECIES_ID)
		{
			rand_val = (rand_val + 1) % NumScouts;
		}
		sMap[rand_val] = scout[x];
	}
}

// Uses a ship map to map species ID to another species ID.
// When in load window, it uses two ship maps to fake a valid response.
SPECIES_ID SeedShip(SPECIES_ID SpeciesID, bool loadWindow)
{
	static int seedStamp = -1;
	static int sisStamp = -1;
	static SPECIES_ID shipMap[NumShips];
	static SPECIES_ID shipWindowMap[NumShips];
	SPECIES_ID target;
	uint16_t index = NumShips;

#ifdef DEBUG_SHIPSEED
	if (loadWindow)
	{
		fmt::print(stderr, "Calling SeedShip (load window) species {}, "
						   "Seed {}, ShipSeed {}, SIS (Seed) {}, SIS (ShipSeed) {}\n",
				   SpeciesID, uqm::currentSeed().value, uqm::currentSeed().ships ? "on" : "off",
				   uqm::gameState().sis().Seed, uqm::gameState().sis().ShipSeed);
	}
#endif
	if (seedStamp != uqm::currentSeed().value)
	{
		SeedShipMap(shipMap, seedStamp = uqm::currentSeed().value);
	}
	if (sisStamp != uqm::gameState().sis().Seed && loadWindow)
	{
		SeedShipMap(shipWindowMap, sisStamp = uqm::gameState().sis().Seed);
	}

	target = SpeciesID;
	if ((index = ShipIndex(SpeciesID)) < NumShips)
	{
		if (!loadWindow && uqm::currentSeed().ships)
		{
			return shipMap[index];
		}
		if (uqm::gameState().sis().ShipSeed != 0)
		{
			target = shipWindowMap[index];
		}
		if (!uqm::currentSeed().ships)
		{
			return target;
		}
		for (index = 0; index < NumShips; index++)
		{
			if (shipMap[index] == target)
			{
				break;
			}
		}
		if (index < NumShips)
		{
			return (ship[index]);
		}
	}
	return target;
}

static void SwapStrings(uqm::StringTable* a, uqm::StringTable* b, size_t ia, size_t ib)
{
	a->swap(ia, *b, ib);
}

RaceDesc* load_ship(SPECIES_ID SpeciesID, bool LoadBattleData)
{
	RaceDesc* RDPtr = 0;
	void* CodeRef {};
	RaceDesc* RDPtrSwap = 0;
	void* CodeRefSwap {};

	if (SpeciesID >= NUM_SPECIES_ID)
	{
		return nullptr;
	}

#ifdef DEBUG_SHIPSEED
	fmt::print(stderr, "Calling load_ship species {}, Seed {}, "
					   "ShipSeed {}\n",
			   SpeciesID, uqm::currentSeed().value,
			   uqm::currentSeed().ships ? "on" : "off");
#endif
	CodeRef = CaptureCodeRes(LoadCodeResInstance(
								 code_resources[uqm::currentSeed().ships ?
													SeedShip(SpeciesID, false) :
													SpeciesID]),
							 nullptr, (void**)(&RDPtr));
	CodeRefSwap = CaptureCodeRes(LoadCodeResInstance(
									 code_resources[SpeciesID]),
								 nullptr, (void**)(&RDPtrSwap));

	if (!CodeRef || !CodeRefSwap)
	{
		goto BadLoad;
	}
	RDPtr->CodeRef = CodeRef;
	RDPtrSwap->CodeRef = CodeRefSwap;

	RDPtr->fleet = RDPtrSwap->fleet;
	if (RDPtr->ship_info.icons_rsc != NullResource)
	{
		RDPtr->ship_info.icons = CaptureDrawable(LoadGraphic(
			RDPtr->ship_info.icons_rsc));
		if (!RDPtr->ship_info.icons)
		{
			/* goto BadLoad */
		}
	}

	if (RDPtr->ship_info.melee_icon_rsc != NullResource)
	{
		RDPtr->ship_info.melee_icon = CaptureDrawable(LoadGraphic(
			RDPtr->ship_info.melee_icon_rsc));
		if (!RDPtr->ship_info.melee_icon)
		{
			/* goto BadLoad */
		}
	}

	if (RDPtrSwap->ship_info.race_strings_rsc != NullResource)
	{
		RDPtr->ship_info.race_strings = uqm::loadStringTable(
											RDPtrSwap->ship_info.race_strings_rsc)
											.release();
		if (!RDPtr->ship_info.race_strings)
		{
			/* goto BadLoad */
		}
		const uqm::StringTablePtr swapStrings {uqm::loadStringTable(RDPtr->ship_info.race_strings_rsc)};
		SwapStrings(swapStrings.get(), RDPtr->ship_info.race_strings, 4, 4);
		SwapStrings(swapStrings.get(), RDPtr->ship_info.race_strings,
					swapStrings->size() - 2,
					RDPtr->ship_info.race_strings->size() - 2);
	}

	if (LoadBattleData)
	{
		ShipResources* RawPtr = &RDPtr->ship_data;
		if (!load_animation(RawPtr->ship,
							RawPtr->ship_rsc[0],
							RawPtr->ship_rsc[1],
							RawPtr->ship_rsc[2]))
		{
			goto BadLoad;
		}

		if (RawPtr->weapon_rsc[0] != NullResource)
		{
			if (!load_animation(RawPtr->weapon,
								RawPtr->weapon_rsc[0],
								RawPtr->weapon_rsc[1],
								RawPtr->weapon_rsc[2]))
			{
				goto BadLoad;
			}
		}

		if (RawPtr->special_rsc[0] != NullResource)
		{
			if (!load_animation(RawPtr->special,
								RawPtr->special_rsc[0],
								RawPtr->special_rsc[1],
								RawPtr->special_rsc[2]))
			{
				goto BadLoad;
			}
		}

		if (RDPtrSwap->ship_data.captain_control.captain_rsc != NullResource)
		{
			RawPtr->captain_control.background = CaptureDrawable(LoadGraphic(
				RDPtrSwap->ship_data.captain_control.captain_rsc));
			if (!RawPtr->captain_control.background)
			{
				goto BadLoad;
			}
		}

		if (RDPtrSwap->ship_data.victory_ditty_rsc != NullResource)
		{
			RawPtr->victory_ditty =
				LoadMusicInstance(RDPtrSwap->ship_data.victory_ditty_rsc);
			if (!RawPtr->victory_ditty)
			{
				goto BadLoad;
			}
		}

		if (RawPtr->ship_sounds_rsc != NullResource)
		{
			RawPtr->ship_sounds = uqm::loadSoundBank(RawPtr->ship_sounds_rsc).release();
			if (!RawPtr->ship_sounds)
			{
				goto BadLoad;
			}
		}
	}

ExitFunc:
	DestroyCodeRes(ReleaseCodeRes(RDPtrSwap->CodeRef));
	return RDPtr;

	// TODO: We should really free the resources that did load here
BadLoad:
	if (CodeRef)
	{
		DestroyCodeRes(ReleaseCodeRes(CodeRef));
	}

	RDPtr = 0; /* failed */

	goto ExitFunc;
}

void free_ship(RaceDesc* raceDescPtr, bool FreeIconData,
			   bool FreeBattleData)
{
	if (raceDescPtr->uninit_func != nullptr)
	{
		(*raceDescPtr->uninit_func)(raceDescPtr);
	}

	if (FreeBattleData)
	{
		ShipResources* shipData = &raceDescPtr->ship_data;

		free_image(shipData->special);
		free_image(shipData->weapon);
		free_image(shipData->ship);

		DestroyDrawable(
			ReleaseDrawable(shipData->captain_control.background));
		DestroyMusic(shipData->victory_ditty);
		delete shipData->ship_sounds;
	}

	if (FreeIconData)
	{
		const ShipInfoDesc* shipInfo = &raceDescPtr->ship_info;

		DestroyDrawable(ReleaseDrawable(shipInfo->melee_icon));
		DestroyDrawable(ReleaseDrawable(shipInfo->icons));
		delete shipInfo->race_strings;
	}

	DestroyCodeRes(ReleaseCodeRes(raceDescPtr->CodeRef));
}
