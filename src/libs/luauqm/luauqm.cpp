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

#include <stdlib.h>

#define LUAUQM_INTERNAL
#include "luauqm.h"

#include "core/log/log.h"


#define LOADSCRIPT_BUFSIZE 0x10000


// We want to give the UQM module writer a good set of functions to work
// with, but we must be careful not to give access to functions which may
// break security. Bad Lua scripts should not be able to affect anything
// outside of UQM.
// XXX TODO: a better sandbox: http://lua-users.org/wiki/SandBoxes
static const luaL_Reg safeLibs[] = {
	{"",			  luaopen_base	 },
	//{ LUA_COLIBNAME,     luaopen_coroutine },
	//{ LUA_DBLIBNAME,     luaopen_debug },
	//{ LUA_LUA_IOLIBNAME, luaopen_io },
	{LUA_MATHLIBNAME, luaopen_math  },
	//{ LUA_LUA_OSLIBNAME, luaopen_os },
	//{ LUA_LUA_LOADLIBNAME, luaopen_package },
	{LUA_STRLIBNAME,	 luaopen_string},
	{LUA_TABLIBNAME,	 luaopen_table },
	{nullptr,		  nullptr		 }
};

void luaUqm_loadLib(lua_State* luaState, const luaL_Reg* lib)
{
	lua_pushglobaltable(luaState);

	lua_pushcfunction(luaState, lib->func);
	lua_pushstring(luaState, lib->name);
	lua_call(luaState, 1, 1);

	// [-2] -> table globalTable
	// [-1] -> string libTable
	lua_setfield(luaState, -2, lib->name);
	lua_pop(luaState, 1);
}

void luaUqm_loadLibs(lua_State* luaState, const luaL_Reg* libs)
{
	while (libs->func != nullptr)
	{
		luaUqm_loadLib(luaState, libs);
		libs++;
	}
}

void luaUqm_loadSafeDefaultLibs(lua_State* luaState)
{
	luaUqm_loadLibs(luaState, safeLibs);
}

// Init the lua UQM system.
void luaUqm_init(void)
{
	InstallScriptResType();
}

// Uninit the lua UQM system. No-op for now.
void luaUqm_uninit(void)
{
}

typedef struct
{
	const char* fileName;
	uqm::io::File* in;
	char* buf;
} luaUqm_ReaderState;

static const char*
luaUqm_reader(lua_State* luaState, void* data, size_t* size)
{
	luaUqm_ReaderState* readerState = (luaUqm_ReaderState*)data;

	const size_t numRead {readerState->in->read(uqstl::as_writable_bytes(uqstl::span {readerState->buf, LOADSCRIPT_BUFSIZE}))};
	if (numRead == 0 && readerState->in->error())
	{
		uqm::log::error("luaUqm_loadScript(): Read error readin "
						"script file '{}'.",
						readerState->fileName);
		*size = 0;
		return nullptr;
	}

	(void)luaState;
	*size = numRead;
	return readerState->buf;
}

void luaUqm_prepareEnvironment(lua_State* luaState)
{
	// Redefines _G, to which the global environment (_ENV) of loaded code
	// is initialised.
	lua_newtable(luaState);
	lua_rawseti(luaState, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
	luaUqm_loadSafeDefaultLibs(luaState);
}

// On success, the script is on the stack as a function.
// Returns true on success, and false on error.
bool luaUqm_loadScript(lua_State* luaState, const uqm::io::Directory& dir,
					   const char* fileName)
{
	uqm::log::debug("Loading script '{}'.", fileName);

	uqstl::error_code error {};
	uqstl::optional<uqm::io::File> in {dir.open(fileName, uqm::io::OpenMode::Read | uqm::io::OpenMode::Text, error)};
	if (!in)
	{
		uqm::log::error("luaUqm_loadScript(): Unable to open script file "
						"'{}' for reading.",
						fileName);
		return false;
	}

	uqstl::vector<char> buf(LOADSCRIPT_BUFSIZE);
	luaUqm_ReaderState readerState {fileName, &*in, buf.data()};
	if (lua_load(luaState, luaUqm_reader, (void*)&readerState, nullptr, nullptr)
		!= LUA_OK)
	{
		uqm::log::error("luaUqm_loadScript(): lua_load() failed: {}",
						lua_tostring(luaState, -1));
		lua_pop(luaState, 1);
		return false;
	}
	return true;
}

// Load a script from file and run it.
bool luaUqm_runScript(lua_State* luaState, const uqm::io::Directory& dir,
					  const char* fileName)
{
	if (!luaUqm_loadScript(luaState, dir, fileName))
	{
		// Could not load script. Error is already printed.
		return false;
	}

	return luaUqm_callStackFunction(luaState);
}

// Run all Lua scripts in a directory.
// Currently, all scripts are run in the same environment.
// Maybe we do not want this.
void luaUqm_runLuaDir(lua_State* luaState, const uqm::io::Directory& dir,
					  const char* luaDirName)
{
	const uqstl::optional<uqm::io::Directory> luaDir {dir.subdir(luaDirName)};
	if (!luaDir)
	{
		uqm::log::warn("Warning: Could not open Lua script directory "
					   "'{}'.",
					   luaDirName);
		return;
	}

	uqstl::error_code error {};
	const uqstl::vector<uqstl::string> luaFiles {luaDir->list("", *uqm::io::Matcher::create(uqm::io::Matcher::Kind::Suffix, ".lua"), error)};
	if (error)
	{
		uqm::log::warn("Warning: Could not read Lua script directory "
					   "'{}'.",
					   luaDirName);
		return;
	}

	uqm::log::debug("Script directory '{}': loading {} file(s).",
					luaDirName, luaFiles.size());
	for (const uqstl::string& fileName : luaFiles)
	{
		luaUqm_runScript(luaState, *luaDir, fileName.c_str());
	}
}

// [-1] -> function fun
// Call a Lua function which is on the stack, taking no parameters and
// returning no value.
// returns false on failure and true on success.
bool luaUqm_callStackFunction(lua_State* luaState)
{
	if (lua_pcall(luaState, 0, 0, 0) != 0)
	{
		uqm::log::error("[script] A script error occurred in "
						"luaUqm_callStackFunction(): {}",
						lua_tostring(luaState, -1));
		lua_pop(luaState, 1);
		// Pop the error.
		return false;
	}

	return true;
}

// Call a Lua function by (char *) name, taking no parameters and returning
// no value. returns false on failure and true on success.
bool luaUqm_callFunction(lua_State* luaState, const char* str)
{
	lua_getglobal(luaState, str);
	if (!lua_isfunction(luaState, -1))
	{
		lua_pop(luaState, 1);
		return false;
	}

	return luaUqm_callStackFunction(luaState);
}

// Pushes a table on the stack with properties with names and values
// from enumVals;
void luaUqm_makeEnum(lua_State* luaState, const luaUqm_EnumValue* enumVals)
{
	const luaUqm_EnumValue* enumPtr;
	size_t enumCount = 0;

	// Count the number of enum values.
	for (enumPtr = enumVals; enumPtr->name != nullptr; enumPtr++)
	{
		enumCount++;
	}

	lua_createtable(luaState, 0, enumCount);
	for (enumPtr = enumVals; enumPtr->name != nullptr; enumPtr++)
	{
		lua_pushinteger(luaState, enumPtr->value);

		// [-2] -> table enumTable
		// [-1] -> int enumValue
		lua_setfield(luaState, -2, enumPtr->name);
	}
}

#ifdef DEBUG
void luaUqm_dumpStack(lua_State* luaState)
{
	int top = lua_gettop(luaState);
	int stackI;

	for (stackI = 1; stackI <= top; stackI++)
	{
		int type = lua_type(luaState, stackI);
		const char* typeName = lua_typename(luaState, type);
		uqm::log::debug("[{}] ({})", stackI, typeName);
		switch (type)
		{
			case LUA_TNONE:
				break;
			case LUA_TNIL:
				break;
			case LUA_TBOOLEAN:
				uqm::log::debug("    {}",
								lua_toboolean(luaState, stackI) ? "true" : "false");
				break;
			case LUA_TLIGHTUSERDATA:
				break;
			case LUA_TNUMBER:
				uqm::log::debug("    %g", lua_tonumber(luaState, stackI));
				break;
			case LUA_TSTRING:
				uqm::log::debug("    \"{}\"",
								lua_tostring(luaState, stackI));
				break;
			case LUA_TTABLE:
				break;
			case LUA_TFUNCTION:
				break;
			case LUA_TUSERDATA:
				break;
			case LUA_TTHREAD:
				break;
		}
	}
}
#endif
