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
#ifndef UQM_LIBS_RESOURCE_INDEX_H
#define UQM_LIBS_RESOURCE_INDEX_H

typedef struct resource_handlers ResourceHandlers;
typedef struct resource_desc ResourceDesc;

#include <stdio.h>
#include "core/stl/string.h"
#include "core/stl/map.h"
#include "libs/reslib.h"

struct resource_handlers
{
	const char* resType;
	// One of loadFun and textLoadFun is set.
	ResourceLoadFun* loadFun;
	ResourceTextLoadFun* textLoadFun;
	ResourceFreeFun* freeFun;
};

struct resource_desc
{
	RESOURCE res_id;
	// STRING values point into it.
	ResourceArg arg;
	ResourceHandlers* vtable;
	RESOURCE_DATA resdata;
	// refcount is rudimentary as nothing really frees the descriptors
	unsigned refcount;
};

struct ResourceIndexDesc
{
	uqstl::unordered_map<uqstl::string, ResourceDesc*>* map;
	size_t numRes;
};

#endif // UQM_LIBS_RESOURCE_INDEX_H
