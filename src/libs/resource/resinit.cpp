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

#include <scn/scan.h>
#include <algorithm>
#include <ctype.h>
#include <stdlib.h>
#include <vector>

#include "resintrn.h"
#include "libs/memlib.h"
#include "options.h"
#include <cstdint>
#include "core/log/log.h"
#include "libs/file/textfile.h"

#include <fmt/format.h>
#include "core/string/StringUtils.h"
#include "libs/gfxlib.h"
#include "libs/reslib.h"
#include "libs/sndlib.h"
#include "libs/vidlib.h"
#include "propfile.h"
// XXX: we should not include anything from uqm/ inside libs/
#include "uqm/coderes.h"

static RESOURCE_INDEX
allocResourceIndex(void)
{
	RESOURCE_INDEX ndx = (RESOURCE_INDEX)HMalloc(sizeof(RESOURCE_INDEX_DESC));
	ndx->map = new std::unordered_map<std::string, ResourceDesc*>();
	return ndx;
}

static void
freeResourceIndex(RESOURCE_INDEX h)
{
	if (h != nullptr)
	{
		/* TODO: This leaks the contents of h->map */
		delete h->map;
		HFree(h);
	}
}

#define TYPESIZ 32

static ResourceDesc*
newResourceDesc(const char* res_id, const char* resval)
{
	const char* path;
	int pathlen;
	ResourceHandlers* vtable;
	ResourceDesc *result, *handlerdesc;
	RESOURCE_INDEX idx = _get_current_index_header();
	char typestr[TYPESIZ] {};
	uqstl::span<char> typestrDest {typestr};
	path = strchr(resval, ':');
	if (path == nullptr)
	{
		uqm::log::warn("Could not find type information for resource '{}'", res_id);
		uqm::strncpy_safe(typestrDest, "sys.UNKNOWNRES");
		path = resval;
	}
	else
	{
		const auto resValLength = static_cast<uint32_t>(uqstl::distance(resval, path));

		const auto copied = uqm::strncpy_safe(typestrDest, "sys.");
		typestrDest = typestrDest.subspan(copied);
		uqm::strncpy_safe(typestrDest, {resval, resValLength});
		path++;
	}
	pathlen = strlen(path);

	handlerdesc = lookupResourceDesc(idx, typestr);
	if (handlerdesc == nullptr)
	{
		path = resval;
		uqm::log::warn("Illegal type '{}' for resource '{}'; treating as UNKNOWNRES", typestr, res_id);
		handlerdesc = lookupResourceDesc(idx, "sys.UNKNOWNRES");
	}

	vtable = (ResourceHandlers*)handlerdesc->resdata.ptr;

	if (vtable->loadFun == nullptr)
	{
		uqm::log::warn("Warning: Unable to load '{}'; no handler "
					   "for type {} defined.",
					   res_id, typestr);
		return nullptr;
	}

	result = (ResourceDesc*)HMalloc(sizeof(ResourceDesc));
	if (result == nullptr)
	{
		return nullptr;
	}

	result->fname = (char*)HMalloc(pathlen + 1);
	uqm::strncpy_safe({result->fname, static_cast<uint32_t>(pathlen + 1)}, path);
	result->fname[pathlen] = '\0';
	result->vtable = vtable;
	result->refcount = 0;

	if (vtable->freeFun == nullptr)
	{
		/* Non-heap resources are raw values. Work those out at load time. */
		vtable->loadFun(result->fname, &result->resdata);
	}
	else
	{
		result->resdata.ptr = nullptr;
	}
	return result;
}

static void
process_resource_desc(const char* key, const char* value)
{
	auto* map = _get_current_index_header()->map;
	ResourceDesc* newDesc = newResourceDesc(key, value);
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

static void
UseDescriptorAsRes(const char* descriptor, RESOURCE_DATA* resdata)
{
	resdata->str = descriptor;
}

static void
DescriptorToInt(const char* descriptor, RESOURCE_DATA* resdata)
{
	resdata->num = std::atoi(descriptor);
}

static void
DescriptorToFloat(const char* descriptor, RESOURCE_DATA* resdata)
{
	resdata->fnum = static_cast<float>(std::atof(descriptor));
}


static void
DescriptorToBoolean(const char* descriptor, RESOURCE_DATA* resdata)
{
	if (!strcasecmp(descriptor, "true"))
	{
		resdata->num = true;
	}
	else
	{
		resdata->num = false;
	}
}

static inline size_t
skipWhiteSpace(const char* start)
{
	const char* ptr = start;
	while (isspace(*ptr))
	{
		ptr++;
	}
	return (ptr - start);
}

// On success, resdata->num will be filled with a 32-bits RGBA value.
static void
DescriptorToColor(const char* descriptor, RESOURCE_DATA* resdata)
{
	// One element for each of r, g, b, a.

	resdata->num = 0;
	descriptor += skipWhiteSpace(descriptor);


	// Color is of the form "rgb(r, g, b)", "rgba(r, g, b, a)",
	// or "rgb15(r, g, b)".

	size_t componentCount {};
	uint32_t comps[4] {0, 0, 0, 0xFF};

	uint8_t maxComponentValue {0xFF};
	uint8_t componentBits {8};
	uqstl::string_view descView {descriptor};
	uqstl::string_view trailingJunk {};
	if (const auto rgbRes {scn::scan<uint8_t, uint8_t, uint8_t>(descView, "rgb ( {} , {} , {} )")})
	{
		const auto [r, g, b] = rgbRes->values();
		comps[0] = r;
		comps[1] = g;
		comps[2] = b;

		componentCount = 3;

		if (!rgbRes->range().empty())
		{
			trailingJunk = {rgbRes->begin(), rgbRes->end()};
		}
	}

	else if (const auto rgbRes {scn::scan<uint8_t, uint8_t, uint8_t, uint8_t>(descView, "rgba ( {} , {} , {} , {} )")})
	{
		const auto [r, g, b, a] = rgbRes->values();
		comps[0] = r;
		comps[1] = g;
		comps[2] = b;
		comps[3] = a;

		componentCount = 4;

		if (!rgbRes->range().empty())
		{
			trailingJunk = {rgbRes->begin(), rgbRes->end()};
		}
	}
	else if (const auto rgbRes {scn::scan<uint8_t, uint8_t, uint8_t>(descView, "rgb15 ( {} , {} , {} )")})
	{
		const auto [r, g, b] = rgbRes->values();
		comps[0] = r;
		comps[1] = g;
		comps[2] = b;

		maxComponentValue = (uint8_t {1} << 5) - 1;
		componentBits = 5;
		componentCount = 3;
	}

	if (componentCount > 0)
	{

		if (!trailingJunk.empty())
		{
			uqm::log::warn("Junk after color resource string (\"{}\").", trailingJunk);
		}

		// Check the range of the components.
		for (size_t compI = 0; compI < componentCount; ++compI)
		{
			if (comps[compI] > maxComponentValue)
			{
				uqm::log::warn("Color component value too large; value {} clipped to max of {}.", comps[compI], maxComponentValue);
				comps[compI] = maxComponentValue;
			}
		}

		if (componentBits == 5)
		{
			resdata->num = ((CC5TO8(comps[0]) << 24) | (CC5TO8(comps[1]) << 16) | (CC5TO8(comps[2]) << 8) | comps[3]);
		}
		else
		{
			resdata->num = ((comps[0] << 24) | (comps[1] << 16) | (comps[2] << 8) | comps[3]);
		}
	}
	else
	{
		uqm::log::error("Invalid color description string for resource. \"{}\"", descView);
	}
}

static void
RawDescriptor(RESOURCE_DATA* resdata, char* buf, unsigned int size)
{
	fmt::format_to_sz_n(buf, size, "{}", resdata->str);
}

static void
IntToString(RESOURCE_DATA* resdata, char* buf, unsigned int size)
{
	fmt::format_to_sz_n(buf, size, "{}", resdata->num);
}

static void
FloatToString(RESOURCE_DATA* resdata, char* buf, unsigned int size)
{
	fmt::format_to_sz_n(buf, size, "{}", resdata->fnum);
}

static void
BooleanToString(RESOURCE_DATA* resdata, char* buf, unsigned int size)
{
	fmt::format_to_sz_n(buf, size, "{}", resdata->num ? "true" : "false");
}

static void
ColorToString(RESOURCE_DATA* resdata, char* buf, unsigned int size)
{
	if ((resdata->num & 0xff) == 0xff)
	{
		// Opaque color, save as "rgb".
		fmt::format_to_sz_n(buf, size, "rgb({:#02x}, {:#02x}, {:#02x})",
							(resdata->num >> 24), (resdata->num >> 16) & 0xff,
							(resdata->num >> 8) & 0xff);
	}
	else
	{
		// (Partially) transparent color, save as "rgba".
		fmt::format_to_sz_n(buf, size, "rgba({:#02x}, {:#02x}, {:#02x}, {:#02x}x)",
							(resdata->num >> 24), (resdata->num >> 16) & 0xff,
							(resdata->num >> 8) & 0xff, resdata->num & 0xff);
	}
}

static RESOURCE_INDEX curResourceIndex;

void _set_current_index_header(RESOURCE_INDEX newResourceIndex)
{
	curResourceIndex = newResourceIndex;
}

RESOURCE_INDEX
InitResourceSystem(void)
{
	RESOURCE_INDEX ndx;
	if (curResourceIndex)
	{
		return curResourceIndex;
	}
	ndx = allocResourceIndex();

	_set_current_index_header(ndx);

	InstallResTypeVectors("UNKNOWNRES", UseDescriptorAsRes, nullptr, nullptr);
	InstallResTypeVectors("STRING", UseDescriptorAsRes, nullptr, RawDescriptor);
	InstallResTypeVectors("INT32", DescriptorToInt, nullptr, IntToString);
	InstallResTypeVectors("FLT32", DescriptorToFloat, nullptr, FloatToString);
	InstallResTypeVectors("bool", DescriptorToBoolean, nullptr, BooleanToString);
	InstallResTypeVectors("COLOR", DescriptorToColor, nullptr, ColorToString);
	InstallGraphicResTypes();
	InstallStringTableResType();
	InstallAudioResTypes();
	InstallVideoResType();
	InstallCodeResType();

	return ndx;
}

RESOURCE_INDEX
_get_current_index_header(void)
{
	if (!curResourceIndex)
	{
		InitResourceSystem();
	}
	return curResourceIndex;
}

void LoadResourceIndex(const uqm::io::Directory& dir, const char* rmpfile, const char* prefix)
{
	PropFile_from_filename(dir, rmpfile, process_resource_desc, prefix);
}

void SaveResourceIndex(const uqm::io::Directory& dir, const char* rmpfile, const char* root, bool strip_root)
{
#ifdef WIN32
	constexpr uqstl::string_view newline {"\r\n"};
#else
	constexpr uqstl::string_view newline {"\n"};
#endif
	const size_t prefix_len {root ? strlen(root) : 0};
	uqstl::string text {};

	std::vector<std::string> keys;
	for (const auto& [key, val] : *(_get_current_index_header()->map))
	{
		if (!root || !strncmp(root, key.c_str(), prefix_len))
		{
			keys.push_back(key);
		}
	}
	std::sort(keys.begin(), keys.end());

	for (const auto& key : keys)
	{
		auto* map = _get_current_index_header()->map;
		auto mapIt = map->find(key);
		ResourceDesc* value = (mapIt != map->end()) ? mapIt->second : nullptr;
		if (!value)
		{
			uqm::log::warn("Resource {} had no value", key);
		}
		else if (!value->vtable)
		{
			uqm::log::warn("Resource {} had no type", key);
		}
		else if (value->vtable->toString)
		{
			char buf[256];
			value->vtable->toString(&value->resdata, buf, 256);
			buf[255] = 0;
			const uqstl::string_view name {root && strip_root ? uqstl::string_view {key}.substr(prefix_len) : uqstl::string_view {key}};
			fmt::format_to(uqstl::back_inserter(text), "{} = {}:{}{}", name, value->vtable->resType, buf, newline);
		}
	}

	if (!uqm::writeTextFile(dir, rmpfile, text))
	{
		uqm::log::warn("Could not write resource index '{}'.", rmpfile);
	}
}

void UninitResourceSystem(void)
{
	freeResourceIndex(_get_current_index_header());
	_set_current_index_header(nullptr);
}

bool InstallResTypeVectors(const char* resType, ResourceLoadFun* loadFun,
						   ResourceFreeFun* freeFun, ResourceStringFun* stringFun)
{
	ResourceHandlers* handlers;
	ResourceDesc* result;
	char key[TYPESIZ] {};
	int typelen;

	fmt::format_to_sz_n(key, TYPESIZ, "sys.{}", resType);
	typelen = strlen(resType);

	handlers = (ResourceHandlers*)HMalloc(sizeof(ResourceHandlers));
	if (handlers == nullptr)
	{
		return false;
	}
	handlers->loadFun = loadFun;
	handlers->freeFun = freeFun;
	handlers->toString = stringFun;
	handlers->resType = resType;

	result = (ResourceDesc*)HMalloc(sizeof(ResourceDesc));
	if (result == nullptr)
	{
		return false;
	}

	const auto resTypeLen = strlen(resType);
	result->fname = (char*)HMalloc(resTypeLen + 1);
	uqm::strncpy_safe({result->fname, resTypeLen}, resType);
	result->vtable = nullptr;
	result->resdata.ptr = handlers;

	auto* map = _get_current_index_header()->map;
	return map->insert({key, result}).second;
}

/* These replace the mapres.c calls and probably should be split out at some point. */
bool res_IsString(const char* key)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	ResourceDesc* desc = lookupResourceDesc(idx, key);
	return (bool)(desc && !strcmp(desc->vtable->resType, "STRING"));
}

const char*
res_GetString(const char* key)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	ResourceDesc* desc = lookupResourceDesc(idx, key);
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
	int srclen, dstlen;
	if (!desc || !desc->resdata.str || strcmp(desc->vtable->resType, "STRING"))
	{
		/* TODO: This is kind of roundabout. We can do better by refactoring newResourceDesc */
		process_resource_desc(key, "STRING:undefined");
		desc = lookupResourceDesc(idx, key);
	}
	srclen = strlen(value);
	dstlen = strlen(desc->fname);
	if (srclen > dstlen)
	{
		char* newValue = (char*)HMalloc(srclen + 1);
		char* oldValue = desc->fname;
		uqm::log::warn("Reallocating string space for '{}'", key);
		uqm::strncpy_safe({newValue, static_cast<uint32_t>(srclen + 1)}, value);
		desc->resdata.str = newValue;
		desc->fname = newValue;
		HFree(oldValue);
	}
	else
	{
		uqm::strncpy_safe({desc->fname, static_cast<uint32_t>(dstlen + 1)}, value);
	}
}

bool res_IsInteger(const char* key)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	ResourceDesc* desc = lookupResourceDesc(idx, key);
	return (bool)(desc && !strcmp(desc->vtable->resType, "INT32"));
}

int res_GetInteger(const char* key)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	ResourceDesc* desc = lookupResourceDesc(idx, key);
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
	ResourceDesc* desc = lookupResourceDesc(idx, key);
	return (bool)(desc && !strcmp(desc->vtable->resType, "FLT32"));
}

int res_GetFloat(const char* key)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	ResourceDesc* desc = lookupResourceDesc(idx, key);
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
		/* TODO: This is kind of roundabout. We can do better by refactoring newResourceDesc */
		process_resource_desc(key, "INT32:0");
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
		/* TODO: This is kind of roundabout. We can do better by refactoring newResourceDesc */
		process_resource_desc(key, "FLT32:0.0");
		desc = lookupResourceDesc(idx, key);
	}
	desc->resdata.fnum = value;
}

bool res_IsBoolean(const char* key)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	ResourceDesc* desc = lookupResourceDesc(idx, key);
	return (bool)(desc && !strcmp(desc->vtable->resType, "bool"));
}

bool res_GetBoolean(const char* key)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	ResourceDesc* desc = lookupResourceDesc(idx, key);
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
		/* TODO: This is kind of roundabout. We can do better by refactoring newResourceDesc */
		process_resource_desc(key, "bool:false");
		desc = lookupResourceDesc(idx, key);
	}
	desc->resdata.num = value;
}

bool res_IsColor(const char* key)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	ResourceDesc* desc = lookupResourceDesc(idx, key);
	return (bool)(desc && !strcmp(desc->vtable->resType, "COLOR"));
}

Color res_GetColor(const char* key)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	ResourceDesc* desc = lookupResourceDesc(idx, key);
	uint32_t num;
	if (!desc || strcmp(desc->vtable->resType, "COLOR"))
	{
		// TODO: Better error handling
		return buildColorRgba(0, 0, 0, 0);
	}

	num = desc->resdata.num;
	return buildColorRgba(num >> 24, (num >> 16) & 0xff,
						  (desc->resdata.num >> 8) & 0xff, num & 0xff);
}

void res_PutColor(const char* key, Color value)
{
	RESOURCE_INDEX idx = _get_current_index_header();
	ResourceDesc* desc = lookupResourceDesc(idx, key);
	if (!desc || strcmp(desc->vtable->resType, "COLOR"))
	{
		/* TODO: This is kind of roundabout. We can do better by refactoring
		 * newResourceDesc */
		process_resource_desc(key, "COLOR:rgb(0, 0, 0)");
		desc = lookupResourceDesc(idx, key);
	}
	desc->resdata.num =
		(value.r << 24) | (value.g << 16) | (value.b << 8) | value.a;
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
	ResourceDesc* oldDesc = (it != map->end()) ? it->second : nullptr;
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
		HFree(oldDesc->fname);
		HFree(oldDesc);
	}
	return map->erase(key) > 0;
}
