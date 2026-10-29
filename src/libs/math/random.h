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

/****************************************************************************
* FILE: random.h
* DESC: definitions and externs for random number generators
*
* HISTORY: Created 6/ 6/1989
* LAST CHANGED:
*
* Copyright (c) 1989, Robert Leyland and Scott Anderson
****************************************************************************/

#ifndef LIBS_MATH_RANDOM_H_
#define LIBS_MATH_RANDOM_H_

#include "../../uqm/globdata.h"
#include "uqm/setupmenu.h"
#include "libs/math/randomdefs.h"

/* ----------------------------GLOBALS/EXTERNS---------------------------- */

uint32_t TFB_SeedRandom(uint32_t seed);
uint32_t TFB_Random(void);


typedef struct RandomContext RandomContext;

#ifdef RANDOM2_INTERNAL
struct RandomContext
{
	uint32_t seed;
};
#endif

#define SeedA (((g_seedType == uqm::SeedType::Planet) && (SANE_SEED(GLOBAL_SIS(Seed)))) ? GLOBAL_SIS(Seed) : PrimeA)
// Default SeedA (PrimeA): 16807 - a relatively prime number - also M div Q
#define SeedM (UINT32_MAX / 2) // 0xFFFFFFFF div 2
#define SeedQ (SeedM / SeedA)  // Default: 127773L - M div A
#define SeedR (SeedM % SeedA)  // Default: 2836 - M mod A


RandomContext* RandomContext_New(void);
RandomContext* RandomContext_Set(uint32_t Context);
void RandomContext_Delete(RandomContext* context);
RandomContext* RandomContext_Copy(const RandomContext* source);
uint32_t RandomContext_Random(RandomContext* context);
uint32_t RandomContext_SeedRandom(RandomContext* context, uint32_t new_seed);
uint32_t RandomContext_GetSeed(RandomContext* context);
uint32_t RandomContext_FastRandom(uint32_t seed);
int RangeMinMax(int min, int max, uint32_t rand);
extern void printBits(size_t const size, void const* const ptr);


#endif /* LIBS_MATH_RANDOM_H_ */
