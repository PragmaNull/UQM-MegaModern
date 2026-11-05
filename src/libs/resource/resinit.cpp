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

#include "core/stl/stl.h"
#include "core/stl/vector.h"

#include "resintrn.h"
#include "options.h"
#include <cstdint>
#include "core/log/log.h"
#include "libs/file/textfile.h"

#include <fmt/format.h>
#include "core/string/StringUtils.h"
#include "libs/reslib.h"
#include "libs/sndlib.h"
#include "libs/vidlib.h"
// XXX: we should not include anything from uqm/ inside libs/
#include "uqm/coderes.h"

static RESOURCE_INDEX allocResourceIndex(void)
{
	auto* ndx {new RESOURCE_INDEX_DESC {}};
	ndx->map = new uqstl::unordered_map<uqstl::string, ResourceDesc*>();
	return ndx;
}

static void freeResourceIndex(RESOURCE_INDEX h)
{
	if (h != nullptr)
	{
		// Loaded resource data is not freed here.
		for (auto& [key, desc] : *h->map)
		{
			if (desc->vtable == nullptr)
			{
				// A type entry: its data is the handlers.
				delete static_cast<ResourceHandlers*>(desc->resdata.ptr);
			}
			delete desc;
		}
		delete h->map;
		delete h;
	}
}

static ResourceDesc* newResourceDesc(const char* res_id, uqstl::string_view type, ResourceArg arg)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	const ResourceDesc* handlerdesc {lookupResourceDesc(idx, fmt::format("sys.{}", type).c_str())};
	if (handlerdesc == nullptr)
	{
		uqm::log::warn("Illegal type '{}' for resource '{}'; treating as UNKNOWNRES", type, res_id);
		handlerdesc = lookupResourceDesc(idx, "sys.UNKNOWNRES");
	}

	auto* const vtable {static_cast<ResourceHandlers*>(handlerdesc->resdata.ptr)};
	if (vtable->textLoadFun != nullptr && resourceString(arg) == nullptr)
	{
		uqm::log::warn("Warning: The data of resource '{}' is not a string.", res_id);
		return nullptr;
	}

	auto* const result {new ResourceDesc {}};
	result->arg = uqstl::move(arg);
	result->vtable = vtable;
	result->refcount = 0;

	if (vtable->freeFun == nullptr)
	{
		/* Non-heap resources are raw values. Work those out at load time. */
		loadResourceDesc(result);
	}
	else
	{
		result->resdata.ptr = nullptr;
	}
	return result;
}

static void process_resource_desc(const char* key, uqstl::string_view type, ResourceArg arg)
{
	auto* map = _get_current_index_header()->map;
	ResourceDesc* newDesc = newResourceDesc(key, type, uqstl::move(arg));
	if (newDesc != nullptr)
	{
		auto [it, inserted] = map->insert({key, newDesc});
		if (!inserted)
		{
			res_Remove(key);
			(*map)[key] = newDesc;
		}
	}
}

static void UseDescriptorAsRes(const ResourceArg& arg, RESOURCE_DATA* resdata)
{
	resdata->str = resourceString(arg);
}

static void UseTextAsRes(const char* text, RESOURCE_DATA* resdata)
{
	resdata->str = text;
}

static void DescriptorToInt(const ResourceArg& arg, RESOURCE_DATA* resdata)
{
	const int64_t* const value {uqstl::get_if<int64_t>(&arg)};
	resdata->num = value != nullptr ? static_cast<uint32_t>(*value) : 0;
}

static void DescriptorToFloat(const ResourceArg& arg, RESOURCE_DATA* resdata)
{
	if (const double* const value {uqstl::get_if<double>(&arg)})
	{
		resdata->fnum = static_cast<float>(*value);
	}
	else if (const int64_t* const number {uqstl::get_if<int64_t>(&arg)})
	{
		resdata->fnum = static_cast<float>(*number);
	}
	else
	{
		resdata->fnum = 0.0f;
	}
}

static void DescriptorToBoolean(const ResourceArg& arg, RESOURCE_DATA* resdata)
{
	const bool* const value {uqstl::get_if<bool>(&arg)};
	resdata->num = value != nullptr && *value;
}

static RESOURCE_INDEX curResourceIndex;

void _set_current_index_header(RESOURCE_INDEX newResourceIndex)
{
	curResourceIndex = newResourceIndex;
}

RESOURCE_INDEX InitResourceSystem(void)
{
	if (curResourceIndex)
	{
		return curResourceIndex;
	}
	RESOURCE_INDEX ndx {allocResourceIndex()};

	_set_current_index_header(ndx);

	InstallResTypeVectors("UNKNOWNRES", UseDescriptorAsRes, nullptr);
	InstallResTextTypeVectors("STRING", UseTextAsRes, nullptr);
	InstallResTypeVectors("INT32", DescriptorToInt, nullptr);
	InstallResTypeVectors("FLT32", DescriptorToFloat, nullptr);
	InstallResTypeVectors("bool", DescriptorToBoolean, nullptr);
	InstallGraphicResTypes();
	InstallStringTableResType();
	InstallAudioResTypes();
	InstallVideoResType();
	InstallCodeResType();

	return ndx;
}

RESOURCE_INDEX _get_current_index_header(void)
{
	if (!curResourceIndex)
	{
		InitResourceSystem();
	}
	return curResourceIndex;
}

void LoadResourceIndex(const uqm::io::Directory& dir, const char* filename, const char* prefix)
{
	const uqstl::optional<uqstl::string> text {uqm::readTextFile(dir, filename)};
	if (!text)
	{
		return;
	}
	auto entries {uqm::resourceIndexFromJson(*text, prefix != nullptr ? prefix : "")};
	if (!entries)
	{
		uqm::log::warn("Warning: Could not read resource index '{}'.", filename);
		return;
	}
	for (uqm::ResourceEntry& entry : *entries)
	{
		process_resource_desc(entry.key.c_str(), entry.type, uqstl::move(entry.arg));
	}
}

bool ConvertLegacyResourceIndex(const uqm::io::Directory& dir, const char* oldName, const char* newName)
{
	if (dir.exists(newName))
	{
		return false;
	}
	const auto text {uqm::readTextFile(dir, oldName)};
	if (!text)
	{
		return false;
	}
	const uqstl::vector<uqm::ResourceEntry> entries {uqm::resourceIndexFromLegacyText(*text)};
	if (!uqm::writeTextFile(dir, newName, uqm::resourceIndexToJson(entries)))
	{
		uqm::log::warn("Warning: Could not write '{}'.", newName);
		return false;
	}
	const uqstl::string backupName {fmt::format("{}.bak", oldName)};
	(void)dir.remove(backupName);
	if (dir.rename(oldName, dir, backupName))
	{
		uqm::log::warn("Warning: Could not rename '{}' to '{}'.", oldName, backupName);
	}
	uqm::log::info("Converted '{}' to '{}'.", oldName, newName);
	return true;
}

void UninitResourceSystem(void)
{
	freeResourceIndex(_get_current_index_header());
	_set_current_index_header(nullptr);
}

static bool installResourceHandlers(const char* resType, ResourceLoadFun* loadFun, ResourceTextLoadFun* textLoadFun,
									ResourceFreeFun* freeFun)
{
	auto* const handlers {new ResourceHandlers {}};
	handlers->loadFun = loadFun;
	handlers->textLoadFun = textLoadFun;
	handlers->freeFun = freeFun;
	handlers->resType = resType;

	auto* const result {new ResourceDesc {}};
	result->vtable = nullptr;
	result->resdata.ptr = handlers;

	auto* map = _get_current_index_header()->map;
	return map->insert({fmt::format("sys.{}", resType), result}).second;
}

bool InstallResTypeVectors(const char* resType, ResourceLoadFun* loadFun, ResourceFreeFun* freeFun)
{
	return installResourceHandlers(resType, loadFun, nullptr, freeFun);
}

bool InstallResTextTypeVectors(const char* resType, ResourceTextLoadFun* loadFun, ResourceFreeFun* freeFun)
{
	return installResourceHandlers(resType, nullptr, loadFun, freeFun);
}

/* These replace the mapres.c calls and probably should be split out at some point. */
bool res_IsString(const char* key)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	const ResourceDesc* desc = lookupResourceDesc(idx, key);
	return (bool)(desc && !strcmp(desc->vtable->resType, "STRING"));
}

const char* res_GetString(const char* key)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	const ResourceDesc* desc = lookupResourceDesc(idx, key);
	if (!desc || !desc->resdata.str || strcmp(desc->vtable->resType, "STRING"))
	{
		return "";
	}
	/* TODO: Work out exact STRING semantics, specifically, the lifetime of
	 *   the returned value. If caller is allowed to reference the returned
	 *   value forever, STRING has to be ref-counted. */
	return desc->resdata.str;
}

void res_PutString(const char* key, const char* value)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	ResourceDesc* desc = lookupResourceDesc(idx, key);
	if (!desc || !desc->resdata.str || strcmp(desc->vtable->resType, "STRING"))
	{
		process_resource_desc(key, "STRING", uqstl::string {});
		desc = lookupResourceDesc(idx, key);
	}
	desc->arg = uqstl::string {value};
	desc->resdata.str = resourceString(desc->arg);
}

bool res_IsInteger(const char* key)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	const ResourceDesc* desc = lookupResourceDesc(idx, key);
	return (bool)(desc && !strcmp(desc->vtable->resType, "INT32"));
}

int res_GetInteger(const char* key)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	const ResourceDesc* desc = lookupResourceDesc(idx, key);
	if (!desc || strcmp(desc->vtable->resType, "INT32"))
	{
		// TODO: Better error handling
		return 0;
	}
	return desc->resdata.num;
}

bool res_IsFloat(const char* key)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	const ResourceDesc* desc = lookupResourceDesc(idx, key);
	return (bool)(desc && !strcmp(desc->vtable->resType, "FLT32"));
}

int res_GetFloat(const char* key)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	const ResourceDesc* desc = lookupResourceDesc(idx, key);
	if (!desc || strcmp(desc->vtable->resType, "FLT32"))
	{
		// TODO: Better error handling
		return 0;
	}
	return desc->resdata.fnum;
}

void res_PutInteger(const char* key, int value)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	ResourceDesc* desc = lookupResourceDesc(idx, key);
	if (!desc || strcmp(desc->vtable->resType, "INT32"))
	{
		process_resource_desc(key, "INT32", int64_t {0});
		desc = lookupResourceDesc(idx, key);
	}
	desc->resdata.num = value;
}

void res_PutFloat(const char* key, float value)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	ResourceDesc* desc = lookupResourceDesc(idx, key);
	if (!desc || strcmp(desc->vtable->resType, "FLT32"))
	{
		process_resource_desc(key, "FLT32", 0.0);
		desc = lookupResourceDesc(idx, key);
	}
	desc->resdata.fnum = value;
}

bool res_IsBoolean(const char* key)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	const ResourceDesc* desc = lookupResourceDesc(idx, key);
	return (bool)(desc && !strcmp(desc->vtable->resType, "bool"));
}

bool res_GetBoolean(const char* key)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	const ResourceDesc* desc = lookupResourceDesc(idx, key);
	if (!desc || strcmp(desc->vtable->resType, "bool"))
	{
		// TODO: Better error handling
		return false;
	}
	return desc->resdata.num ? true : false;
}

void res_PutBoolean(const char* key, bool value)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	ResourceDesc* desc = lookupResourceDesc(idx, key);
	if (!desc || strcmp(desc->vtable->resType, "bool"))
	{
		process_resource_desc(key, "bool", false);
		desc = lookupResourceDesc(idx, key);
	}
	desc->resdata.num = value;
}

bool res_HasKey(const char* key)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	return (bool)(lookupResourceDesc(idx, key) != nullptr);
}

bool res_Remove(const char* key)
{
	auto* map = _get_current_index_header()->map;
	auto it = map->find(key);
	const ResourceDesc* oldDesc = (it != map->end()) ? it->second : nullptr;
	if (oldDesc != nullptr)
	{
		if (oldDesc->resdata.ptr != nullptr)
		{
			if (oldDesc->refcount > 0)
			{
				uqm::log::warn("WARNING: Replacing '{}' while it is live", key);
			}
			if (oldDesc->vtable && oldDesc->vtable->freeFun)
			{
				oldDesc->vtable->freeFun(oldDesc->resdata.ptr);
			}
		}
		delete oldDesc;
	}
	return map->erase(key) > 0;
}
