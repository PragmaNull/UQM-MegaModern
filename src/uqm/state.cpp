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

#include "state.h"

#include "starmap.h"
#include "globdata.h"
#include "core/log/log.h"

static constexpr auto StarBufsize {(NumSolarSystems * sizeof(uint32_t) + 3800 * (3 * sizeof(uint32_t)))};
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <memory.h>

uqstl::array<GAME_STATE_FILE, NUM_STATE_FILES> MakeStateFiles(void)
{
	// In the order StarinfoFile, RandgrpinfoFile, DefgrpinfoFile.
	return {
		{
			{.symname = "STARINFO", .size_hint = StarBufsize},
			{.symname = "RANDGRPINFO", .size_hint = RandBufsize},
			{.symname = "DEFGRPINFO", .size_hint = DefBufsize},
		 }
	 };
}


GAME_STATE_FILE* OpenStateFile(int stateFile, const char* mode)
{
	if (stateFile < 0 || stateFile >= NUM_STATE_FILES)
	{
		return nullptr;
	}

	GAME_STATE_FILE* fp {&uqm::gameState().stateFile(stateFile)};
	fp->open_count++;
	if (fp->open_count > 1)
	{
		uqm::log::warn("WARNING: "
					   "State file {} open count is {} after open()",
					   fp->symname, fp->open_count);
	}

	if (fp->data.empty())
	{
		fp->data.assign(fp->size_hint, 0);
		fp->size = fp->size_hint;
	}

	// we allow reading and writing for any open mode
	// but the mode determines what happens to the file contents
	if (mode[0] == 'w')
	{ // blow the file away
		fp->used = 0;
#ifdef DEBUG
		// paint buffer for tracking writes
		memset(fp->data.data(), 0xCC, fp->size);
#endif
	}
	else if (mode[0] == 'r')
	{ // nothing
	}
	else
	{
		uqm::log::warn("WARNING: "
					   "State file {} opened with unsupported mode '{}'",
					   fp->symname, mode);
	}
	fp->ptr = 0;

	return fp;
}

void CloseStateFile(GAME_STATE_FILE* fp)
{
	fp->ptr = 0;
	fp->open_count--;
	if (fp->open_count < 0)
	{
		uqm::log::warn("WARNING: "
					   "State file {} open count is {} after close()",
					   fp->symname, fp->open_count);
	}
	// Erm, Ok, it's closed! Honest!
}

void DeleteStateFile(int stateFile)
{
	if (stateFile < 0 || stateFile >= NUM_STATE_FILES)
	{
		return;
	}

	GAME_STATE_FILE* fp {&uqm::gameState().stateFile(stateFile)};
	if (fp->open_count != 0)
	{
		uqm::log::warn("WARNING: "
					   "State file {} open count is {} during delete()",
					   fp->symname, fp->open_count);
	}

	fp->used = 0;
	fp->ptr = 0;
	fp->data = {};
}

uint32_t LengthStateFile(GAME_STATE_FILE* fp)
{
	return fp->used;
}

int ReadStateFile(void* lpBuf, uint16_t size, uint16_t count, GAME_STATE_FILE* fp)
{
	uint32_t bytes = size * count;

	if (fp->ptr >= fp->size)
	{ // EOF
		return 0;
	}
	else if (fp->ptr + bytes > fp->size)
	{ // dont have that much data
		bytes = fp->size - fp->ptr;
		bytes -= bytes % size;
	}

	if (bytes > 0)
	{
		memcpy(lpBuf, fp->data.data() + fp->ptr, bytes);
		fp->ptr += bytes;
	}
	return (bytes / size);
}

int WriteStateFile(const void* lpBuf, uint16_t size, uint16_t count, GAME_STATE_FILE* fp)
{
	const uint32_t bytes = size * count;

	if (fp->ptr + bytes > fp->size)
	{ // dont have that much space available
		uint32_t newsize = fp->ptr + bytes;
		// grab more space in advance
		if (newsize < fp->size * 3 / 2)
		{
			newsize = fp->size * 3 / 2;
		}

		fp->data.resize(newsize);

		fp->size = newsize;
		if (newsize > fp->size_hint)
		{
			fp->size_hint = newsize;
		}
	}

	if (bytes > 0)
	{
		memcpy(fp->data.data() + fp->ptr, lpBuf, bytes);
		fp->ptr += bytes;
		if (fp->ptr > fp->used)
		{
			fp->used = fp->ptr;
		}
	}
	return (bytes / size);
}

int SeekStateFile(GAME_STATE_FILE* fp, long offset, int whence)
{
	if (whence == SEEK_CUR)
	{
		offset += fp->ptr;
	}
	else if (whence == SEEK_END)
	{
		offset += fp->used;
	}

	if (offset < 0)
	{
		fp->ptr = 0;
		return 0;
	}
	fp->ptr = offset;
	return 1;
}


void InitPlanetInfo(void)
{
	GAME_STATE_FILE* fp {OpenStateFile(StarinfoFile, "wb")};
	if (fp)
	{
		// Set record offsets for all stars to 0 (not present)
		const STAR_DESC* pSD {&star_array[0]};
		do
		{
			swrite_32(fp, 0);
			++pSD;
		} while (pSD->star_pt.x <= MaxXUniverse
				 && pSD->star_pt.y <= MaxYUniverse);

		CloseStateFile(fp);
	}
}

void UninitPlanetInfo(void)
{
	DeleteStateFile(StarinfoFile);
}

static constexpr auto OffsetSize {(sizeof(uint32_t))};
//#define ScanRecordSize  (sizeof (uint32_t) * NUM_SCAN_TYPES)
// JMS: Increased the size of scan record to house partially scavenged minerals.
static constexpr auto ScanRecordSize {((sizeof(uint32_t) * NUM_SCAN_TYPES) + (sizeof(uint8_t) * NUM_SCAN_TYPES * 32))};

void GetPlanetInfo(void)
{
	uint16_t k {};
	uint16_t l {};

	pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[BIOLOGICAL_SCAN] = 0;
	pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[MINERAL_SCAN] = 0;
	pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[ENERGY_SCAN] = 0;

	// JMS: Init also the partially scavenged mineral deposit values.
	for (l = MINERAL_SCAN; l < NUM_SCAN_TYPES; l++)
	{
		for (k = 0; k < 32; k++)
		{
			pSolarSysState->SysInfo.PlanetInfo.PartiallyScavengedList[l][k] = 0;
		}
	}

	GAME_STATE_FILE* fp {OpenStateFile(StarinfoFile, "rb")};
	if (fp)
	{
		uint16_t planet_index {};
		uint16_t moon_index {};
		uint32_t offset {};

		const uint16_t star_index {(uint16_t)(CurStarDescPtr() - star_array)};
		planet_index = (uint16_t)(pSolarSysState->pBaseDesc->pPrevDesc
								  - pSolarSysState->PlanetDesc);
		if (pSolarSysState->pOrbitalDesc->pPrevDesc == pSolarSysState->SunDesc)
		{
			moon_index = 0;
		}
		else
		{
			moon_index = (uint16_t)(pSolarSysState->pOrbitalDesc
									- pSolarSysState->MoonDesc + 1);
		}

		SeekStateFile(fp, star_index * OffsetSize, SEEK_SET);
		sread_32(fp, &offset);

		if (offset)
		{
			// Skip scan records for all preceeding planets to the one we need
			for (uint16_t i {0}; i < planet_index; ++i)
			{
				offset += (pSolarSysState->PlanetDesc[i].NumPlanets + 1) * ScanRecordSize;
			}

			// Skip scan records for all preceeding moons to the one we need
			offset += moon_index * ScanRecordSize;

			SeekStateFile(fp, offset, SEEK_SET);
			sread_a32(fp, pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask,
					  NUM_SCAN_TYPES);

			{
				uint8_t* ar = &(pSolarSysState->SysInfo.PlanetInfo.PartiallyScavengedList[0][0]);

				// JMS: Read which mineral deposits are partially retrieved (and how much).
				for (l = MINERAL_SCAN; l < NUM_SCAN_TYPES; l++)
				{
					for (k = 0; k < 32; k++, ar++)
					{
						sread_8(fp, ar);
					}
				}
			}
		}

		CloseStateFile(fp);
	}
}

void PutPlanetInfo(void)
{
	GAME_STATE_FILE* fp {OpenStateFile(StarinfoFile, "r+b")};
	if (fp)
	{
		uint16_t i {};
		uint16_t k {};
		uint16_t l {};
		uint16_t planet_index {};
		uint16_t moon_index {};
		uint32_t offset {};

		const uint16_t star_index {(uint16_t)(CurStarDescPtr() - star_array)};
		planet_index = (uint16_t)(pSolarSysState->pBaseDesc->pPrevDesc
								  - pSolarSysState->PlanetDesc);
		if (pSolarSysState->pOrbitalDesc->pPrevDesc == pSolarSysState->SunDesc)
		{
			moon_index = 0;
		}
		else
		{
			moon_index = (uint16_t)(pSolarSysState->pOrbitalDesc
									- pSolarSysState->MoonDesc + 1);
		}

		SeekStateFile(fp, star_index * OffsetSize, SEEK_SET);
		sread_32(fp, &offset);

		if (offset == 0)
		{ // Scan record not present yet -- init it
			uint32_t ScanRetrieveMask[NUM_SCAN_TYPES] =
				{
					0,
					0,
					0,
				};

			// JMS: Init also the partially scavenged mineral deposit values.
			uint8_t PartiallyScavengedList[NUM_SCAN_TYPES][32];
			for (l = MINERAL_SCAN; l < NUM_SCAN_TYPES; l++)
			{
				for (k = 0; k < 32; k++)
				{
					PartiallyScavengedList[l][k] = 0;
				}
			}

			offset = LengthStateFile(fp);

			// Write the record offset
			SeekStateFile(fp, star_index * OffsetSize, SEEK_SET);
			swrite_32(fp, offset);

			// Init scan records for all planets and moons in the system
			SeekStateFile(fp, offset, SEEK_SET);
			for (i = 0; i < pSolarSysState->SunDesc[0].NumPlanets; ++i)
			{
				const uint8_t* ar = &(PartiallyScavengedList[0][0]);

				swrite_a32(fp, ScanRetrieveMask, NUM_SCAN_TYPES);

				// JMS: Also init with zeroes the list of partially scavenged mineral amounts.
				for (l = MINERAL_SCAN; l < NUM_SCAN_TYPES; l++)
				{
					for (k = 0; k < 32; k++, ar++)
					{
						swrite_8(fp, *ar);
					}
				}

				// init moons
				for (uint16_t j {0}; j < pSolarSysState->PlanetDesc[i].NumPlanets; ++j)
				{
					const uint8_t* ar = &(PartiallyScavengedList[0][0]);

					swrite_a32(fp, ScanRetrieveMask, NUM_SCAN_TYPES);

					// JMS: Ditto for the moons.
					for (l = MINERAL_SCAN; l < NUM_SCAN_TYPES; l++)
					{
						for (k = 0; k < 32; k++, ar++)
						{
							swrite_8(fp, *ar);
						}
					}
				}
			}
		}

		// Skip scan records for all preceeding planets to the one we need
		for (i = 0; i < planet_index; ++i)
		{
			offset += (pSolarSysState->PlanetDesc[i].NumPlanets + 1) * ScanRecordSize;
		}

		// Skip scan records for all preceeding moons to the one we need
		offset += moon_index * ScanRecordSize;

		SeekStateFile(fp, offset, SEEK_SET);

		// Store which mineral deposits we have already retrieved.
		swrite_a32(fp, pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask,
				   NUM_SCAN_TYPES);

		{
			const uint8_t* ar = &(pSolarSysState->SysInfo.PlanetInfo.PartiallyScavengedList[0][0]);

			// JMS: Store which mineral deposits are partially retrieved (and how much).
			for (l = MINERAL_SCAN; l < NUM_SCAN_TYPES; l++)
			{
				for (k = 0; k < 32; k++, ar++)
				{
					swrite_8(fp, *ar);
				}
			}
		}

		CloseStateFile(fp);
	}
}
