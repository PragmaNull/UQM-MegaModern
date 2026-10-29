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

//#include <string_view>
#include "options.h"
#include "port.h"
#include "resintrn.h"
#include "libs/memlib.h"
#include "core/log/log.h"

// At other times, it is nullptr.

ResourceDesc*
lookupResourceDesc(RESOURCE_INDEX idx, RESOURCE res)
{
	auto it = idx->map->find(res);
	return it != idx->map->end() ? it->second : nullptr;
}

void loadResourceDesc(ResourceDesc* desc)
{
	desc->vtable->loadFun(desc->fname, &desc->resdata);
}

void* LoadResourceFromPath(const char* path, ResourceLoadFileFun* loadFun)
{
	void* const resdata {res_LoadResFile(uqm::contentDir(), path, loadFun)};
	if (resdata == nullptr)
	{
		uqm::log::warn("Warning: Can't load '{}'", path);
	}
	return resdata;
}

const char*
res_GetResourceType(RESOURCE res)
{
	RESOURCE_INDEX resourceIndex;
	ResourceDesc* desc;

	if (res == NULL_RESOURCE)
	{
		uqm::log::warn("Trying to get type of null resource");
		return nullptr;
	}

	resourceIndex = _get_current_index_header();
	desc = lookupResourceDesc(resourceIndex, res);
	if (desc == nullptr)
	{
		uqm::log::warn("Trying to get type of undefined resource '{}'",
					   res);
		return nullptr;
	}

	return desc->vtable->resType;
}


// Get a resource by its resource ID.
void* res_GetResource(RESOURCE res)
{
	RESOURCE_INDEX resourceIndex;
	ResourceDesc* desc;

	if (res == NULL_RESOURCE)
	{
		uqm::log::warn("Trying to get null resource");
		return nullptr;
	}

	resourceIndex = _get_current_index_header();

	desc = lookupResourceDesc(resourceIndex, res);
	if (desc == nullptr)
	{
		uqm::log::warn("Trying to get undefined resource '{}'",
					   res);
		return nullptr;
	}

	if (desc->resdata.ptr == nullptr)
	{
		loadResourceDesc(desc);
	}
	if (desc->resdata.ptr != nullptr)
	{
		++desc->refcount;
	}

	return desc->resdata.ptr;
	// May still be nullptr, if the load failed.
}

uint32_t
res_GetIntResource(RESOURCE res)
{
	RESOURCE_INDEX resourceIndex;
	ResourceDesc* desc;

	if (res == NULL_RESOURCE)
	{
		uqm::log::warn("Trying to get null resource");
		return 0;
	}

	resourceIndex = _get_current_index_header();

	desc = lookupResourceDesc(resourceIndex, res);
	if (desc == nullptr)
	{
		uqm::log::warn("Trying to get undefined resource '{}'",
					   res);
		return 0;
	}

	return desc->resdata.num;
}

bool res_GetBooleanResource(RESOURCE res)
{
	return (bool)(res_GetIntResource(res) != 0);
}

// NB: this function appears to be never called!
void res_FreeResource(RESOURCE res)
{
	ResourceDesc* desc;
	ResourceFreeFun* freeFun;

	desc = lookupResourceDesc(_get_current_index_header(), res);
	if (desc == nullptr)
	{
		uqm::log::debug("Warning: trying to free an unrecognised "
						"resource.");
		return;
	}

	if (desc->refcount > 0)
	{
		--desc->refcount;
	}
	else
	{
		uqm::log::debug("Warning: freeing an unreferenced resource.");
	}
	if (desc->refcount > 0)
	{
		return; // Still references left
	}

	freeFun = desc->vtable->freeFun;
	if (freeFun == nullptr)
	{
		uqm::log::debug("Warning: trying to free a non-heap resource.");
		return;
	}

	if (desc->resdata.ptr == nullptr)
	{
		uqm::log::debug("Warning: trying to free not loaded "
						"resource.");
		return;
	}

	(*freeFun)(desc->resdata.ptr);
	desc->resdata.ptr = nullptr;
}

// By calling this function the caller will be responsible of unloading
// the resource. If res_GetResource() get called again for this
// resource, a NEW copy will be loaded, regardless of whether a detached
// copy still exists.
void* res_DetachResource(RESOURCE res)
{
	ResourceDesc* desc;
	ResourceFreeFun* freeFun;
	void* result;

	desc = lookupResourceDesc(_get_current_index_header(), res);
	if (desc == nullptr)
	{
		uqm::log::debug("Warning: trying to detach from an unrecognised "
						"resource.");
		return nullptr;
	}

	freeFun = desc->vtable->freeFun;
	if (freeFun == nullptr)
	{
		uqm::log::debug("Warning: trying to detach from a non-heap resource.");
		return nullptr;
	}

	if (desc->resdata.ptr == nullptr)
	{
		uqm::log::debug("Warning: trying to detach from a not loaded "
						"resource.");
		return nullptr;
	}

	if (desc->refcount > 1)
	{
		uqm::log::debug("Warning: trying to detach a resource referenced "
						"{} times",
						desc->refcount);
		return nullptr;
	}

	result = desc->resdata.ptr;
	desc->resdata.ptr = nullptr;
	desc->refcount = 0;

	return result;
}

bool FreeResourceData(void* data)
{
	HFree(data);
	return true;
}
