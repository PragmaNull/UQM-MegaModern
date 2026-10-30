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

#ifndef UQM_STATE_H_
#define UQM_STATE_H_

#include "port.h"
#include "core/misc/bittricks.h"
#include <assert.h>

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
#define GROUPS_RANDOM ((uint32_t)(0L))
#define GROUPS_ADD_NEW ((uint32_t)(~0L))
#define GROUP_LIST ((uint8_t)0)
#define GROUP_INIT_IP ((uint8_t)~0)
// Initialize IP group list (ip_group_q) from the actual groups
// (not GROUP_LIST) in one of the state files
#define GROUP_LOAD_IP GROUP_LIST
// Read IP group list into ip_group_q from the list entry
// (GROUP_LIST) in one of the state files
#define GROUP_SAVE_IP ((uint8_t)~0)
// Write IP group list from ip_group_q to the list entry
// (GROUP_LIST) in one of the state files
extern void BuildGroups(void);
extern void findRaceSOI(void);

typedef struct GAME_STATE_FILE GAME_STATE_FILE;

#define STARINFO_FILE 0
//"starinfo.dat"
#define STAR_BUFSIZE (NUM_SOLAR_SYSTEMS * sizeof(uint32_t) \
					  + 3800 * (3 * sizeof(uint32_t)))
#define RANDGRPINFO_FILE 1
//"randgrp.dat"
#define RAND_BUFSIZE (4 * 1024)
#define DEFGRPINFO_FILE 2
//"defgrp.dat"
#define DEF_BUFSIZE (10 * 1024)

typedef enum
{
	STARINFO,
	RANDGRPINFO,
	DEFGRPINFO
} INFO_TYPE;

GAME_STATE_FILE* OpenStateFile(int stateFile, const char* mode);
void CloseStateFile(GAME_STATE_FILE* fp);
void DeleteStateFile(int stateFile);
uint32_t LengthStateFile(GAME_STATE_FILE* fp);
int ReadStateFile(void* lpBuf, uint16_t size, uint16_t count, GAME_STATE_FILE* fp);
int WriteStateFile(const void* lpBuf, uint16_t size, uint16_t count, GAME_STATE_FILE* fp);
int SeekStateFile(GAME_STATE_FILE* fp, long offset, int whence);

static inline uint16_t
sread_8(GAME_STATE_FILE* fp, uint8_t* v)
{
	uint8_t t;
	if (!v) /* read value ignored */
	{
		v = &t;
	}
	return ReadStateFile(v, 1, 1, fp);
}

static inline uint16_t
sread_16(GAME_STATE_FILE* fp, uint16_t* v)
{
	uint16_t t;
	if (!v) /* read value ignored */
	{
		v = &t;
	}
	return ReadStateFile(v, 2, 1, fp);
}

static inline uint16_t
sread_16s(GAME_STATE_FILE* fp, int16_t* v)
{
	uint16_t t;
	uint16_t ret;
	ret = sread_16(fp, &t);
	// unsigned to signed conversion
	if (v)
	{
		*v = t;
	}
	return ret;
}

static inline uint16_t
sread_32(GAME_STATE_FILE* fp, uint32_t* v)
{
	uint32_t t;
	if (!v) /* read value ignored */
	{
		v = &t;
	}
	return ReadStateFile(v, 4, 1, fp);
}

static inline uint16_t
sread_a32(GAME_STATE_FILE* fp, uint32_t* ar, uint16_t count)
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

static inline uint16_t
swrite_8(GAME_STATE_FILE* fp, uint8_t v)
{
	return WriteStateFile(&v, 1, 1, fp);
}

static inline uint16_t
swrite_16(GAME_STATE_FILE* fp, uint16_t v)
{
	return WriteStateFile(&v, 2, 1, fp);
}

static inline uint16_t
swrite_32(GAME_STATE_FILE* fp, uint32_t v)
{
	return WriteStateFile(&v, 4, 1, fp);
}

static inline uint16_t
swrite_a32(GAME_STATE_FILE* fp, const uint32_t* ar, uint16_t count)
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

#endif /* UQM_STATE_H_ */
