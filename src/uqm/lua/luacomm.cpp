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
#include "libs/scriptlib.h"
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
		loadOk = luaUqm_loadScript(luaUqm_commState, *contentDir,
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

bool luaUqm_comm_stringNeedsInterpolate(const char* str)
{
	return strstr(str, "<%") != nullptr;
}

// Resizes *buf if necessary. Makes sure that there is always enough
// space for a final '\0' to be added.
static void
luaUqm_comm_addToBuffer(char** buf, size_t* bufLen, char** bufPtr,
						const char* add, size_t addLen)
{
	size_t bufFill = *bufPtr - *buf;
	size_t newLen = *bufLen;

	while ((size_t)(bufFill + addLen >= newLen))
	{
		// Need enough space for the terminating '\0' too.
		newLen = newLen * 2;
	}

	if (newLen != *bufLen)
	{
		char* newBuf = (char*)HRealloc(*buf, newLen);
		if (newBuf == nullptr)
		{
			uqm::log::error("Error: luaUqm_addToBuffer(): could not "
							"allocate memory.\n");
			return;
			// We continue, without adding 'add' to the buffer.
		}

		*bufLen = newLen;
		*buf = newBuf;
		*bufPtr = newBuf + bufFill;
	}

	memcpy(*bufPtr, add, addLen);
	*bufPtr += addLen;
}

// Pre: *bufLen contains the space available in buf.
// Post: *bufLen contains the size of the string in *buf.
char* luaUqm_comm_stringInterpolate(const char* str)
{
	const char* strPtr;
	size_t interI;
	// Interpolation counter.
	char* buf;
	//
	char* bufPtr;
	size_t bufLen;
	const char* part;
	size_t partLen;

	assert(luaUqm_commState != nullptr);

	bufLen = 2048;
	buf = (char*)HMalloc(bufLen);
	if (buf == nullptr)
	{
		return nullptr;
	}

	strPtr = str;
	bufPtr = buf;

	// We go through the string and put all the parts into a Lua table.
	for (interI = 0;; interI++)
	{
		const char* startTag;
		const char* endTag;
		const char* luaStart;

		startTag = strstr(strPtr, "<%");
		if (startTag == nullptr)
		{
			break;
		}
		luaStart = startTag + 2;

		// Store the string before the '<%'.
		luaUqm_comm_addToBuffer(&buf, &bufLen, &bufPtr,
								strPtr, startTag - strPtr);

		endTag = strstr(startTag + 2, "%>");
		if (endTag == nullptr)
		{
			uqm::log::error("luaUqm_stringInterpolate(): Unterminated "
							"'<%% .. %%>' sequence in string '{}'.",
							str);
			// We ignore the rest of the string.
			goto out;
		}

		strPtr = endTag + 2;
		interI++;

		// Compile the string to a Lua function.
		{
			size_t exprLen = endTag - luaStart;
#define LUAEXPR_START "return "
			const uint32_t exprBufSize = sizeof LUAEXPR_START + exprLen;
			char* exprBuf = (char*)HMalloc(exprBufSize);
			// 'sizeof LUAEXPR_START' includes a null byte
			char* exprBufPtr = exprBuf;
			uqm::strncpy_safe({exprBuf, exprBufSize}, {LUAEXPR_START, sizeof(LUAEXPR_START)});
			exprBufPtr += sizeof LUAEXPR_START - 1;
			memcpy(exprBufPtr, luaStart, exprLen);
			exprBufPtr += exprLen;
			*exprBufPtr = '\0';

			if (luaL_loadstring(luaUqm_commState, exprBuf) != LUA_OK)
			{
				uqm::log::error("luaUqm_stringInterpolate(): "
								"lua_loadstring() failed: {}",
								lua_tostring(luaUqm_commState, -1));
				lua_pop(luaUqm_commState, 1);
				// Pop the error.
				continue;
			}
		}

		// Call the Lua function.
		if (lua_pcall(luaUqm_commState, 0, 1, 0) != 0)
		{
			uqm::log::error("[script] luaUqm_stringInterpolate(): A "
							"script error occurred in interpolation {} in string "
							"'{}': {}.",
							interI, str,
							lua_tostring(luaUqm_commState, -1));
			lua_pop(luaUqm_commState, 1);
			// Pop the error.
			continue;
		}
		// Success. Result is on the stack.

		// Convert the result to a string, and get the length.
		part = lua_tolstring(luaUqm_commState, -1, &partLen);
		if (part == nullptr)
		{
			// Not a string and not convertable to a string.
			uqm::log::error("[script] luaUqm_stringInterpolate(): Value "
							"returned by interpolation {} has type {}, which can not "
							"be converted to a string, in string "
							"'{}'.",
							interI, lua_typename(luaUqm_commState, lua_type(luaUqm_commState, -1)), str);
			lua_pop(luaUqm_commState, 1);
			continue;
		}

		// Store the result of the Lua expression in '<% .. %>'.
		luaUqm_comm_addToBuffer(&buf, &bufLen, &bufPtr, part, partLen);

		// Pop the result from the stack.
		lua_pop(luaUqm_commState, 1);
	}

	// Store the part of the string after the last '<% .. %>'.
	luaUqm_comm_addToBuffer(&buf, &bufLen, &bufPtr,
							strPtr, strlen(strPtr));

out:
	*bufPtr = '\0';
	// luaUqm_addToBuffer() always leaves one byte for the '\0'.

	{
		char* newBuf = (char*)HRealloc(buf, bufPtr - buf + 1);
		if (newBuf == nullptr)
		{
			// If we can't shorten 'newBuf', we'll just keep using the
			// unnecessarilly long 'buf', and let the next allocating
			// function worry about the impending memory shortage.
		}
		else
		{
			buf = newBuf;
		}
	}

	return buf;
}
