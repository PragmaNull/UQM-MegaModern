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

#include "lifeform.h"
#include "planets.h"
#include "libs/mathlib.h"
#include "core/log/log.h"


//#define DEBUG_SURFACE

const uint8_t* Elements;
const PlanetFrame* PlanData;

#define MEDIUM_DEPOSIT_THRESHOLD 150
#define LARGE_DEPOSIT_THRESHOLD 225

static uint16_t
CalcMineralDeposits(const SYSTEM_INFO* SysInfoPtr, uint16_t which_deposit,
					NODE_INFO* info)
{
	uint8_t j;
	uint16_t num_deposits;
	const ELEMENT_ENTRY* eptr;

	eptr = &SysInfoPtr->PlanetInfo.PlanDataPtr->UsefulElements[0];
	num_deposits = 0;
	j = NUM_USEFUL_ELEMENTS;
	do
	{
		uint8_t num_possible;

		num_possible = uqm::lowByte(RandomContext_Random(SysGenRNG))
					 % (DEPOSIT_QUANTITY(eptr->Density) + 1);
		while (num_possible--)
		{
			uint16_t deposit_quality_fine;
			uint16_t deposit_quality_gross;

			// JMS: For making the mineral blip smaller in case it is
			// partially scavenged.
			int32_t temp_deposit_quality;

			deposit_quality_fine = (uqm::lowWord(RandomContext_Random(SysGenRNG)) % 100)
								 + (DEPOSIT_QUALITY(eptr->Density)
									+ SysInfoPtr->StarSize)
									   * 50;

			// JMS: This makes the mineral blip smaller in case it is
			// partially scavenged.
			if (which_deposit < 32)
			{
				temp_deposit_quality = deposit_quality_fine - ((SysInfoPtr->PlanetInfo.PartiallyScavengedList[MINERAL_SCAN][which_deposit]) * 10);
			}
			// JMS: In case which_deposit >= 32 (most likely 65535), it
			// means that this function is being called only to count the
			// number of deposit nodes on the surface. In that case we
			// don't need to use the PartiallyScavengedList
			// since the amount of minerals in that node is not stored yet.
			// (AND we cannot use the list since accessing element 65535
			// would crash the game ;)
			else
			{
				temp_deposit_quality = deposit_quality_fine;
			}

			if (temp_deposit_quality < 0)
			{
				temp_deposit_quality = 0;
			}

			if (temp_deposit_quality < MEDIUM_DEPOSIT_THRESHOLD)
			{
				deposit_quality_gross = 0;
			}
			else if (temp_deposit_quality < LARGE_DEPOSIT_THRESHOLD)
			{
				deposit_quality_gross = 1;
			}
			else
			{
				deposit_quality_gross = 2;
			}

			GenerateRandomLocation(&info->loc_pt);

			info->density = uqm::makeWord(
				deposit_quality_gross, deposit_quality_fine / 10 + 1);
			info->type = eptr->ElementType;
#ifdef DEBUG_SURFACE
			uqm::log::debug("\t\t{} units of %Fs",
							info->density,
							Elements[eptr->ElementType].name);
#endif /* DEBUG_SURFACE */
			if (num_deposits >= which_deposit
				|| ++num_deposits == sizeof(uint32_t) * 8)
			{ // reached the maximum or the requested node
				return num_deposits;
			}
		}
		++eptr;
	} while (--j);

	return num_deposits;
}

// Returns:
//   for whichLife==~0 : the number of nodes generated
//   for whichLife<32  : the index of the last node (no known usage exists)
// Sets the SysGenRNG to the required state first.
uint16_t
GenerateMineralDeposits(const SYSTEM_INFO* SysInfoPtr, uint16_t whichDeposit,
						NODE_INFO* info)
{
	NODE_INFO temp_info;
	if (!info) // user not interested in info but we need space for it
	{
		info = &temp_info;
	}
	RandomContext_SeedRandom(SysGenRNG,
							 SysInfoPtr->PlanetInfo.ScanSeed[MINERAL_SCAN]);
	return CalcMineralDeposits(SysInfoPtr, whichDeposit, info);
}

static uint16_t
CalcLifeForms(const SYSTEM_INFO* SysInfoPtr, uint16_t which_life,
			  NODE_INFO* info)
{
	uint16_t num_life_forms;

	num_life_forms = 0;
	if (PLANSIZE(SysInfoPtr->PlanetInfo.PlanDataPtr->Type) != GAS_GIANT)
	{
#define MIN_LIFE_CHANCE 10
		int16_t life_var;

		life_var = RandomContext_Random(SysGenRNG) & 1023;
		if (life_var < SysInfoPtr->PlanetInfo.LifeChance
			|| (SysInfoPtr->PlanetInfo.LifeChance < MIN_LIFE_CHANCE
				&& life_var < MIN_LIFE_CHANCE))
		{
			uint8_t num_types;

			num_types = 1 + uqm::lowByte(RandomContext_Random(SysGenRNG)) % MAX_LIFE_VARIATION;
			do
			{
				uint8_t index, num_creatures;
				uint16_t rand_val;

				rand_val = RandomContext_Random(SysGenRNG);
				index = uqm::lowByte(rand_val) % NUM_CREATURE_TYPES;
				num_creatures = 1 + uqm::highByte(rand_val) % 10;
				do
				{
					GenerateRandomLocation(&info->loc_pt);
					info->type = index;
					info->density = 0;

					if (num_life_forms >= which_life
						|| ++num_life_forms == sizeof(uint32_t) * 8)
					{ // reached the maximum or the requested node
						return num_life_forms;
					}
				} while (--num_creatures);
			} while (--num_types);
		}
#ifdef DEBUG_SURFACE
		else
		{
			uqm::log::debug("It's dead, Jim! ({} >= {})", life_var,
							SysInfoPtr->PlanetInfo.LifeChance);
		}
#endif /* DEBUG_SURFACE */
	}

	return num_life_forms;
}

// Returns:
//   for whichLife==~0 : the number of lifeforms generated
//   for whichLife<32  : the index of the last lifeform (no known usage exists)
// Sets the SysGenRNG to the required state first.
uint16_t
GenerateLifeForms(const SYSTEM_INFO* SysInfoPtr, uint16_t whichLife,
				  NODE_INFO* info)
{
	NODE_INFO temp_info;
	if (!info) // user not interested in info but we need space for it
	{
		info = &temp_info;
	}
	RandomContext_SeedRandom(SysGenRNG,
							 SysInfoPtr->PlanetInfo.ScanSeed[BIOLOGICAL_SCAN]);
	return CalcLifeForms(SysInfoPtr, whichLife, info);
}

// Returns:
//   for whichLife==~0 : the number of lifeforms generated
//   for whichLife<32  : the index of the last lifeform (no known usage exists)
// Sets the SysGenRNG to the required state first.
// lifeTypes[] is terminated with -1
uint16_t
GeneratePresetLife(const SYSTEM_INFO* SysInfoPtr, const int8_t* lifeTypes,
				   uint16_t whichLife, NODE_INFO* info)
{
	uint16_t i;
	NODE_INFO temp_info;

	if (!info) // user not interested in info but we need space for it
	{
		info = &temp_info;
	}

	// This function may look unnecessarily complicated, but it must be
	// kept this way to preserve the universe. That is done by preserving
	// the order and number of Random() calls.

	RandomContext_SeedRandom(SysGenRNG,
							 SysInfoPtr->PlanetInfo.ScanSeed[BIOLOGICAL_SCAN]);

	for (i = 0; lifeTypes[i] >= 0; ++i)
	{
		GenerateRandomLocation(&info->loc_pt);
		info->type = lifeTypes[i];
		// density is irrelevant for bio nodes
		info->density = 0;

		if (i >= whichLife)
		{
			break;
		}
	}

	return i;
}


static int16_t
widthPick(void)
{
	EXTENT map_dimensions[] = {MAP_DIMENSIONS};

	return map_dimensions[uqm::UQMOptions::read().planetTexture].width;
}

static int16_t
heightPick(void)
{
	EXTENT map_dimensions[] = {MAP_DIMENSIONS};

	return map_dimensions[uqm::UQMOptions::read().planetTexture].height;
}

static int16_t
scaleMapWidth(int16_t value)
{
	float percentage = 1;

	if (UNSCALED_MAP_WIDTH != widthPick())
	{
		percentage = scaleThing(UNSCALED_MAP_WIDTH, widthPick());
	}

	return RES_SCALE((int16_t)(value * percentage));
}

static int16_t
scaleMapHeight(int16_t value)
{
	float percentage = 1;

	if (SC2_MAP_HEIGHT != heightPick())
	{
		percentage = scaleThing(SC2_MAP_HEIGHT, heightPick());
	}

	return RES_SCALE((int16_t)(value * percentage));
}

void GenerateRandomLocation(GFXPOINT* loc)
{
	uint16_t rand_val;

	rand_val = RandomContext_Random(SysGenRNG);

	loc->x = scaleMapWidth(8 + uqm::lowByte(rand_val) % (widthPick() - (8 << 1)));
	loc->y = scaleMapHeight(8 + uqm::highByte(rand_val) % (heightPick() - (8 << 1)));
}

// Returns:
//   for whichNode==~0 : the number of nodes generated
//   for whichNode<32  : the index of the last node (no known usage exists)
// Sets the SysGenRNG to the required state first.
uint16_t
GenerateRandomNodes(const SYSTEM_INFO* SysInfoPtr, uint16_t scan, uint16_t numNodes,
					uint16_t type, uint16_t whichNode, NODE_INFO* info)
{
	uint16_t i;
	NODE_INFO temp_info;

	if (!info) // user not interested in info but we need space for it
	{
		info = &temp_info;
	}

	RandomContext_SeedRandom(SysGenRNG,
							 SysInfoPtr->PlanetInfo.ScanSeed[scan]);

	for (i = 0; i < numNodes; ++i)
	{
		GenerateRandomLocation(&info->loc_pt);
		// type is irrelevant for energy nodes
		info->type = type;
		// density is irrelevant for energy and bio nodes
		info->density = 0;

		if (i >= whichNode)
		{
			break;
		}
	}

	return i;
}

uint16_t
CustomMineralDeposits(const SYSTEM_INFO* SysInfoPtr, uint16_t which_deposit,
					  NODE_INFO* info, uint16_t numNodes, uint16_t type, uint8_t quality)
{
	uint8_t j;
	uint16_t num_deposits;
	bool OpenSeason = true;
	NODE_INFO temp_info;
	const ELEMENT_ENTRY* eptr;

	if (!info) // user not interested in info but we need space for it
	{
		info = &temp_info;
	}

	RandomContext_SeedRandom(SysGenRNG,
							 SysInfoPtr->PlanetInfo.ScanSeed[MINERAL_SCAN]);

	eptr = &SysInfoPtr->PlanetInfo.PlanDataPtr->UsefulElements[0];
	num_deposits = 0;
	j = NUM_USEFUL_ELEMENTS;
	do
	{
		uint8_t num_possible, depositQuality = 0;

		num_possible = uqm::lowByte(RandomContext_Random(SysGenRNG))
					 % (DEPOSIT_QUANTITY(eptr->Density) + 1);

		if (num_possible == 0 && OpenSeason)
		{
			num_possible = numNodes;
			info->type = type;
			depositQuality = quality;
			OpenSeason = false;
		}
		else
		{
			depositQuality = DEPOSIT_QUALITY(eptr->Density);
			info->type = eptr->ElementType;
		}

		while (num_possible--)
		{
			uint16_t deposit_quality_fine;
			uint16_t deposit_quality_gross;

			// JMS: For making the mineral blip smaller in case it is partially scavenged.
			int32_t temp_deposit_quality;

			deposit_quality_fine = (uqm::lowWord(RandomContext_Random(SysGenRNG)) % 100)
								 + (depositQuality
									+ SysInfoPtr->StarSize)
									   * 50;

			// JMS: This makes the mineral blip smaller in case it is partially scavenged.
			if (which_deposit < 32)
			{
				temp_deposit_quality = deposit_quality_fine
									 - ((SysInfoPtr->PlanetInfo.PartiallyScavengedList[MINERAL_SCAN][which_deposit]) * 10);
			}
			// JMS: In case which_deposit >= 32 (most likely 65535), it means that this
			// function is being called only to count the number of deposit nodes on the
			// surface. In that case we don't need to use the PartiallyScavengedList
			// since the amount of minerals in that node is not stored yet.
			// (AND we cannot use the list since accessing element 65535 would crash the game ;)
			else
			{
				temp_deposit_quality = deposit_quality_fine;
			}

			if (temp_deposit_quality < 0)
			{
				temp_deposit_quality = 0;
			}

			if (temp_deposit_quality < MEDIUM_DEPOSIT_THRESHOLD)
			{
				deposit_quality_gross = 0;
			}
			else if (temp_deposit_quality < LARGE_DEPOSIT_THRESHOLD)
			{
				deposit_quality_gross = 1;
			}
			else
			{
				deposit_quality_gross = 2;
			}

			GenerateRandomLocation(&info->loc_pt);

			info->density = uqm::makeWord(
				deposit_quality_gross, deposit_quality_fine / 10 + 1);
#ifdef DEBUG_SURFACE
			uqm::log::debug("\t\t{} units of %Fs",
							info->density,
							Elements[eptr->ElementType].name);
#endif /* DEBUG_SURFACE */
			if (num_deposits >= which_deposit
				|| ++num_deposits == sizeof(uint32_t) * 8)
			{ // reached the maximum or the requested node
				return num_deposits;
			}
		}
		++eptr;
	} while (--j);

	return num_deposits;
}

uint16_t
CustomMineralDeposit(NODE_INFO* info, uint16_t type, uint8_t quality,
					 GFXPOINT location)
{
	uint16_t deposit_quality_fine;
	uint16_t deposit_quality_gross;

	if (!info)
	{
		return 0;
	}

	info->type = type;

	deposit_quality_fine = quality * 10;

	if (deposit_quality_fine < MEDIUM_DEPOSIT_THRESHOLD)
	{
		deposit_quality_gross = 0;
	}
	else if (deposit_quality_fine < LARGE_DEPOSIT_THRESHOLD)
	{
		deposit_quality_gross = 1;
	}
	else
	{
		deposit_quality_gross = 2;
	}

	info->loc_pt = location;

	info->density = uqm::makeWord(deposit_quality_gross, quality);

	return 1;
}
