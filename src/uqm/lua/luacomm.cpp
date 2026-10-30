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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * This file contains code for using Lua with the game conversations.
 */

#include <stdlib.h>
#include <string.h>
#include "core/string/StringUtils.h"
#define LUAUQM_INTERNAL
#include "port.h"
#include "luacomm.h"

#include "options.h"
// For contentDir
#include "libs/luauqm/luauqm.h"
#include "uqm/lua/luastate.h"
#include "luafuncs/commfuncs.h"
#include "luafuncs/customfuncs.h"
#include "luafuncs/eventfuncs.h"
#include "luafuncs/logfuncs.h"
#include "luafuncs/statefuncs.h"
#include "core/log/log.h"

lua_State* luaUqm_commState = nullptr;

static const luaL_Reg commLibs[] = {
	{"comm",	 luaUqm_comm_open },
	{"event", luaUqm_event_open},
	{"log",	luaUqm_log_open  },
	{"state", luaUqm_state_open},
	{nullptr, nullptr			 }
};

// Not reentrant.
// If 'customFuncs' is nullptr, no 'custom' table is added to the Lua environment.
// If 'scriptRes' is NULL_RESOURCE, then no script is loaded. Lua is only
// available for string interpolation in this case.
bool luaUqm_comm_init(const luaUqm_custom_Function* customFuncs,
					  RESOURCE scriptRes)
{
	assert(luaUqm_commState == nullptr);

	luaUqm_commState = luaUqm_globalState;

	// Prepare the global environment.
	luaUqm_prepareEnvironment(luaUqm_commState);
	luaUqm_loadLibs(luaUqm_commState, commLibs);
	if (customFuncs != nullptr)
	{
		luaUqm_custom_init(luaUqm_commState, customFuncs);
		lua_pop(luaUqm_commState, 1);
	}

	if (scriptRes != NULL_RESOURCE)
	{
		// Load the script.
		char* scriptFileName;
		bool loadOk;

		// Get the name of the script.
		scriptFileName = LoadScriptInstance(scriptRes);
		if (scriptFileName == nullptr)
		{
			return false;
		}

		// Load the script.
		loadOk = luaUqm_loadScript(luaUqm_commState, uqm::contentDir(),
								   scriptFileName);
		ReleaseScriptResData(scriptFileName);
		if (!loadOk)
		{
			return false;
		}

		// Call the script.
		luaUqm_callStackFunction(luaUqm_commState);
	}

	return true;
}

void luaUqm_comm_uninit(void)
{
	assert(luaUqm_commState != nullptr);
	luaUqm_commState = nullptr;
}

// Use as LOCDATA.init_encounter_func
void luaUqm_comm_genericInit(void)
{
	assert(luaUqm_commState != nullptr);
	luaUqm_callFunction(luaUqm_commState, "init");
}

// Use as LOCDATA.post_encounter_func
void luaUqm_comm_genericPost(void)
{
	assert(luaUqm_commState != nullptr);
	luaUqm_callFunction(luaUqm_commState, "post");
}

// Use as LOCDATA.uninit_encounter_func
void luaUqm_comm_genericUninit(void)
{
	assert(luaUqm_commState != nullptr);
	luaUqm_callFunction(luaUqm_commState, "uninit");

	luaUqm_comm_uninit();
}

bool luaUqm_comm_stringNeedsInterpolate(uqstl::string_view str)
{
	return str.find("<%") != uqstl::string_view::npos;
}

uqstl::string luaUqm_comm_stringInterpolate(uqstl::string_view str)
{
	assert(luaUqm_commState != nullptr);

	uqstl::string result {};
	size_t pos {};
	for (size_t interI {};; ++interI)
	{
		const size_t startTag {str.find("<%", pos)};
		if (startTag == uqstl::string_view::npos)
		{
			break;
		}

		// Store the string before the '<%'.
		result += str.substr(pos, startTag - pos);

		const size_t endTag {str.find("%>", startTag + 2)};
		if (endTag == uqstl::string_view::npos)
		{
			uqm::log::error("luaUqm_stringInterpolate(): Unterminated "
							"'<%% .. %%>' sequence in string '{}'.",
							str);
			// We ignore the rest of the string.
			return result;
		}
		const uqstl::string expression {fmt::format("return {}", str.substr(startTag + 2, endTag - startTag - 2))};
		pos = endTag + 2;

		// Compile the string to a Lua function.
		if (luaL_loadstring(luaUqm_commState, expression.c_str()) != LUA_OK)
		{
			uqm::log::error("luaUqm_stringInterpolate(): "
							"lua_loadstring() failed: {}",
							lua_tostring(luaUqm_commState, -1));
			// Pop the error.
			lua_pop(luaUqm_commState, 1);
			continue;
		}

		// Call the Lua function.
		if (lua_pcall(luaUqm_commState, 0, 1, 0) != 0)
		{
			uqm::log::error("[script] luaUqm_stringInterpolate(): A "
							"script error occurred in interpolation {} in string "
							"'{}': {}.",
							interI, str,
							lua_tostring(luaUqm_commState, -1));
			// Pop the error.
			lua_pop(luaUqm_commState, 1);
			continue;
		}

		// Convert the result to a string.
		size_t partLen {};
		const char* part {lua_tolstring(luaUqm_commState, -1, &partLen)};
		if (part == nullptr)
		{
			// Not a string and not convertable to a string.
			uqm::log::error("[script] luaUqm_stringInterpolate(): Value "
							"returned by interpolation {} has type {}, which can not "
							"be converted to a string, in string "
							"'{}'.",
							interI, lua_typename(luaUqm_commState, lua_type(luaUqm_commState, -1)), str);
		}
		else
		{
			// Store the result of the Lua expression in '<% .. %>'.
			result.append(part, partLen);
		}

		// Pop the result from the stack.
		lua_pop(luaUqm_commState, 1);
	}

	// Store the part of the string after the last '<% .. %>'.
	result += str.substr(pos);
	return result;
}
