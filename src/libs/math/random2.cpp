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

// This file contains variants of the random functions in random.c
// that store the state of the RNG in a context, allowing for multiple
// independant RNGs to be used simultaneously.
// The RNG behavior itself is the same.

#include "core/types/bytes.h"

#define RANDOM2_INTERNAL
#include "random.h"

#include "libs/memlib.h"

RandomContext*
RandomContext_New(void)
{
	RandomContext* result = (RandomContext*)HMalloc(sizeof(RandomContext));
	result->seed = 12345;
	return result;
}

RandomContext*
RandomContext_Set(uint32_t Context)
{
	RandomContext* result = (RandomContext*)HMalloc(sizeof(RandomContext));
	result->seed = Context;
	return result;
}

void RandomContext_Delete(RandomContext* context)
{
	HFree((void*)context);
}

RandomContext*
RandomContext_Copy(const RandomContext* source)
{
	RandomContext* result = (RandomContext*)HMalloc(sizeof(RandomContext));
	*result = *source;
	return result;
}

uint32_t
RandomContext_Random(RandomContext* context)
{
	context->seed = SeedA * (context->seed % SeedQ) - SeedR * (context->seed / SeedQ);
	if (context->seed > SeedM)
	{
		context->seed -= SeedM;
	}
	else if (context->seed == 0)
	{
		context->seed = 1;
	}

	return context->seed;
}

uint32_t
RandomContext_SeedRandom(RandomContext* context, uint32_t new_seed)
{
	uint32_t old_seed;

	/* coerce the seed to be in the range 1..M */
	if (new_seed == 0) /* 0 becomes 1 */
	{
		new_seed = 1;
	}
	else if (new_seed > SeedM) /* and less than M */
	{
		new_seed -= SeedM;
	}

	old_seed = context->seed;
	context->seed = new_seed;
	return old_seed;
}

uint32_t
RandomContext_GetSeed(RandomContext* context)
{
	return context->seed;
}

uint32_t
RandomContext_FastRandom(uint32_t seed)
{
	uint32_t res = SeedA * (seed % SeedQ) - SeedR * (seed / SeedQ);
	if (res > SeedM)
	{
		res -= SeedM;
	}
	else if (res == 0)
	{
		res = 1;
	}

	return res;
}

int RangeMinMax(int min, int max, uint32_t rand)
{
	if (min > max)
	{
		return max;
	}

	return rand % ((max + 1) - min) + min;
}
