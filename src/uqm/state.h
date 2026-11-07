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

#pragma once
#ifndef UQM_STATE_H
#define UQM_STATE_H

#include "core/types/enum.h"
#include "port.h"
#include "core/misc/bittricks.h"
#include "core/stl/array.h"
#include "core/stl/vector.h"
#include <assert.h>
#include <cstdint>

#if 0 //defined(__cplusplus)
extern "C" {
#endif

extern void InitPlanetInfo(void);
extern void UninitPlanetInfo(void);
extern void GetPlanetInfo(void);
extern void PutPlanetInfo(void);

extern void InitGroupInfo(bool FirstTime);
extern void UninitGroupInfo(void);
extern bool GetGroupInfo(uint32_t offset, uint8_t which_group);
extern uint32_t PutGroupInfo(uint32_t offset, uint8_t which_group);
inline constexpr auto GroupsRandom {((uint32_t)(0L))};
inline constexpr auto GroupsAddNew {((uint32_t)(~0L))};
inline constexpr auto GroupList {((uint8_t)0)};
inline constexpr auto GroupInitIp {((uint8_t)~0)};
// Initialize IP group list (ip_group_q) from the actual groups
// (not GroupList) in one of the state files
inline constexpr auto GroupLoadIp {GroupList};
// Read IP group list into ip_group_q from the list entry
// (GroupList) in one of the state files
inline constexpr auto GroupSaveIp {((uint8_t)~0)};
// Write IP group list from ip_group_q to the list entry
// (GroupList) in one of the state files
extern void BuildGroups(void);
extern void findRaceSOI(void);

// A file in memory that holds a part of the game state.
struct GameStateFile
{
	const char* symname {};
	uint32_t size_hint {};
	int open_count {};
	uqstl::vector<uint8_t> data {};
	uint32_t used {};
	uint32_t size {};
	uint32_t ptr {};
};

inline constexpr int NUM_STATE_FILES {3};

// Returns the state files of a new game, empty.
[[nodiscard]] uqstl::array<GameStateFile, NUM_STATE_FILES> MakeStateFiles(void);

inline constexpr auto StarinfoFile {0};
//"starinfo.dat"
inline constexpr auto RandgrpinfoFile {1};
//"randgrp.dat"
inline constexpr auto RandBufsize {(4 * 1024)};
inline constexpr auto DefgrpinfoFile {2};
//"defgrp.dat"
inline constexpr auto DefBufsize {(10 * 1024)};

enum class InfoType
{
	StarInfo,
	RandomGroupInfo,
	DefinedGroupInfo
};
template <>
inline constexpr bool IsArithmeticEnum<InfoType> = true;

GameStateFile* OpenStateFile(int stateFile, const char* mode);
void CloseStateFile(GameStateFile* fp);
void DeleteStateFile(int stateFile);
uint32_t LengthStateFile(GameStateFile* fp);
int ReadStateFile(void* lpBuf, uint16_t size, uint16_t count, GameStateFile* fp);
int WriteStateFile(const void* lpBuf, uint16_t size, uint16_t count, GameStateFile* fp);
int SeekStateFile(GameStateFile* fp, long offset, int whence);

static inline uint16_t sread_8(GameStateFile* fp, uint8_t* v)
{
	uint8_t t {};
	if (!v) /* read value ignored */
	{
		v = &t;
	}
	return ReadStateFile(v, 1, 1, fp);
}

static inline uint16_t sread_16(GameStateFile* fp, uint16_t* v)
{
	uint16_t t {};
	if (!v) /* read value ignored */
	{
		v = &t;
	}
	return ReadStateFile(v, 2, 1, fp);
}

static inline uint16_t sread_16s(GameStateFile* fp, int16_t* v)
{
	uint16_t t {};
	const uint16_t ret {sread_16(fp, &t)};
	// unsigned to signed conversion
	if (v)
	{
		*v = t;
	}
	return ret;
}

static inline uint16_t sread_32(GameStateFile* fp, uint32_t* v)
{
	uint32_t t {};
	if (!v) /* read value ignored */
	{
		v = &t;
	}
	return ReadStateFile(v, 4, 1, fp);
}

static inline uint16_t sread_a32(GameStateFile* fp, uint32_t* ar, uint16_t count)
{
	assert(ar != nullptr);

	for (; count > 0; --count, ++ar)
	{
		if (sread_32(fp, ar) != 1)
		{
			return 0;
		}
	}
	return 1;
}

static inline uint16_t swrite_8(GameStateFile* fp, uint8_t v)
{
	return WriteStateFile(&v, 1, 1, fp);
}

static inline uint16_t swrite_16(GameStateFile* fp, uint16_t v)
{
	return WriteStateFile(&v, 2, 1, fp);
}

static inline uint16_t swrite_32(GameStateFile* fp, uint32_t v)
{
	return WriteStateFile(&v, 4, 1, fp);
}

static inline uint16_t swrite_a32(GameStateFile* fp, const uint32_t* ar, uint16_t count)
{
	for (; count > 0; --count, ++ar)
	{
		if (swrite_32(fp, *ar) != 1)
		{
			return 0;
		}
	}
	return 1;
}

#if 0 //defined(__cplusplus)
}
#endif

#endif // UQM_STATE_H
