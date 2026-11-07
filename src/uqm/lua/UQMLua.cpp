// Lua support for the game: the Lua state, the script environments, and script loading.
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

#include "UQMLua.h"

#include <cassert>
#include <cstring>
#include <fmt/format.h>

#include "core/log/log.h"
#include "core/stl/array.h"
#include "core/stl/vector.h"
#include "options.h"
#include "uqm/lua/luafuncs/commfuncs.h"
#include "uqm/lua/luafuncs/customfuncs.h"
#include "uqm/lua/luafuncs/eventfuncs.h"
#include "uqm/lua/luafuncs/logfuncs.h"
#include "uqm/lua/luafuncs/statefuncs.h"
#include "uqm/resinst.h"


namespace uqm
{
namespace
{

constexpr size_t LoadScriptBufSize {0x10000};
constexpr size_t DebugLineSize {2048};
constexpr uqgsl::czstring EventRegistryKey {"uqm_event_registryKey"};

// Bad Lua scripts must not affect anything outside of the game.
// XXX TODO: a better sandbox: http://lua-users.org/wiki/SandBoxes
constexpr uqstl::array SafeLibs {
	luaL_Reg {"",			  luaopen_base  },
	luaL_Reg {LUA_MATHLIBNAME, luaopen_math  },
	luaL_Reg {LUA_STRLIBNAME,  luaopen_string},
	luaL_Reg {LUA_TABLIBNAME,  luaopen_table },
};

constexpr uqstl::array InitLibs {
	luaL_Reg {"event", luaUqm_event_open},
	luaL_Reg {"log",	 luaUqm_log_open	},
	luaL_Reg {"state", luaUqm_state_open},
};

constexpr uqstl::array CommLibs {
	luaL_Reg {"comm",  luaUqm_comm_open },
	luaL_Reg {"event", luaUqm_event_open},
	luaL_Reg {"log",	 luaUqm_log_open	},
	luaL_Reg {"state", luaUqm_state_open},
};

constexpr uqstl::array EventLibs {
	luaL_Reg {"event", luaUqm_event_open},
	luaL_Reg {"log",	 luaUqm_log_open	},
	luaL_Reg {"state", luaUqm_state_open},
};

constexpr uqstl::array DebugLibs {
	luaL_Reg {"comm",  luaUqm_comm_open },
	luaL_Reg {"event", luaUqm_event_open},
	luaL_Reg {"log",	 luaUqm_log_open	},
	luaL_Reg {"state", luaUqm_state_open},
};

bool releaseScriptResData(void* data)
{
	delete[] static_cast<char*>(data);
	return true;
}

// Keeps only the file name. The script loads onto the Lua stack when it is run.
void getScriptResData(const char* pathName, RESOURCE_DATA* resdata)
{
	const size_t size {std::strlen(pathName) + 1};
	auto* const path {new char[size]};
	std::memcpy(path, pathName, size);
	resdata->ptr = path;
}

void loadLib(lua_State* luaState, const luaL_Reg& lib)
{
	const LuaStackGuard stackGuard {luaState};
	lua_pushglobaltable(luaState);

	lua_pushcfunction(luaState, lib.func);
	lua_pushstring(luaState, lib.name);
	lua_call(luaState, 1, 1);

	// [-2] -> table globalTable
	// [-1] -> table libTable
	lua_setfield(luaState, -2, lib.name);
}

struct ReaderState
{
	uqstl::string_view fileName {};
	io::File& in;
	uqstl::vector<char> buf {};
};

const char* readScript(lua_State* /*luaState*/, void* data, size_t* size)
{
	auto* const readerState {static_cast<ReaderState*>(data)};

	const size_t numRead {readerState->in.read(uqstl::as_writable_bytes(uqstl::span {readerState->buf}))};
	if (numRead == 0 && readerState->in.error())
	{
		log::error("UQMLua::loadScript(): Read error reading script file '{}'.", readerState->fileName);
		*size = 0;
		return nullptr;
	}

	*size = numRead;
	return readerState->buf.data();
}

} // namespace

UQMLua::UQMLua()
{
	InstallResTextTypeVectors("SCRIPT", getScriptResData, releaseScriptResData);
}

void UQMLua::openState()
{
	if (m_state)
	{
		log::warn("Lua state multiply initialized");
	}
	m_state.reset(luaL_newstate());

	lua_newtable(m_state.get());
	lua_setfield(m_state.get(), LUA_REGISTRYINDEX, EventRegistryKey);

	// XXX TODO: set up an alternative to the Lua 'require' function, which makes use of uio.

	runInitScripts();
}

void UQMLua::closeState()
{
	if (!m_state)
	{
		log::warn("Lua state multiply uninitialized");
	}
	m_state.reset();
}

void UQMLua::runInitScripts()
{
	// The environment is not used after the init scripts.
	prepareEnvironment(m_state.get());
	loadLibs(m_state.get(), InitLibs);

	const char* const scriptDir {res_GetString(ScriptDirInitgameRes)};
	if (scriptDir == nullptr)
	{
		log::warn("Location of game initialisation scripts ('{}') was not specified.", ScriptDirInitgameRes);
		return;
	}
	runLuaDir(m_state.get(), contentDir(), scriptDir);
}

bool UQMLua::runScriptRes(ResourceName scriptRes)
{
	const uqstl::optional<uqstl::string> fileName {scriptFileName(scriptRes)};
	if (!fileName || !loadScript(m_state.get(), contentDir(), *fileName))
	{
		return false;
	}
	return callStackFunction(m_state.get());
}

/////////////////////////////////////////////////////////////////////////////
// Communication
/////////////////////////////////////////////////////////////////////////////

bool UQMLua::beginComm(const luaUqm_custom_Function* customFuncs, ResourceName scriptRes)
{
	assert(!m_commActive);
	m_commActive = true;

	prepareEnvironment(m_state.get());
	loadLibs(m_state.get(), CommLibs);
	if (customFuncs != nullptr)
	{
		const LuaStackGuard stackGuard {m_state.get()};
		luaUqm_custom_init(m_state.get(), customFuncs);
	}

	if (scriptRes == NullResource)
	{
		return true;
	}
	return runScriptRes(scriptRes);
}

void UQMLua::endComm()
{
	assert(m_commActive);
	m_commActive = false;
}

lua_State* UQMLua::commState() const noexcept
{
	return m_commActive ? m_state.get() : nullptr;
}

bool UQMLua::needsInterpolate(uqstl::string_view str)
{
	return str.contains("<%");
}

uqstl::string UQMLua::interpolate(uqstl::string_view str)
{
	assert(m_commActive);
	lua_State* const luaState {m_state.get()};

	uqstl::string result {};
	size_t pos {};
	for (size_t interI {};; ++interI)
	{
		const size_t startTag {str.find("<%", pos)};
		if (startTag == uqstl::string_view::npos)
		{
			break;
		}

		result += str.substr(pos, startTag - pos);

		const size_t endTag {str.find("%>", startTag + 2)};
		if (endTag == uqstl::string_view::npos)
		{
			log::error("UQMLua::interpolate(): Unterminated '<% .. %>' sequence in string '{}'.", str);
			// Ignore the rest of the string.
			return result;
		}
		const uqstl::string expression {fmt::format("return {}", str.substr(startTag + 2, endTag - startTag - 2))};
		pos = endTag + 2;

		const LuaStackGuard stackGuard {luaState};

		if (luaL_loadstring(luaState, expression.c_str()) != LUA_OK)
		{
			log::error("UQMLua::interpolate(): luaL_loadstring() failed: {}", lua_tostring(luaState, -1));
			continue;
		}

		if (lua_pcall(luaState, 0, 1, 0) != LUA_OK)
		{
			log::error("[script] UQMLua::interpolate(): A script error occurred in interpolation {} in string '{}': {}.",
					   interI, str, lua_tostring(luaState, -1));
			continue;
		}

		size_t partLen {};
		const char* const part {lua_tolstring(luaState, -1, &partLen)};
		if (part == nullptr)
		{
			log::error("[script] UQMLua::interpolate(): Value returned by interpolation {} has type {}, which can not be "
					   "converted to a string, in string '{}'.",
					   interI, lua_typename(luaState, lua_type(luaState, -1)), str);
		}
		else
		{
			result.append(part, partLen);
		}
	}

	result += str.substr(pos);
	return result;
}

/////////////////////////////////////////////////////////////////////////////
// Game events
/////////////////////////////////////////////////////////////////////////////

bool UQMLua::beginEvents(const luaUqm_custom_Function* customFuncs, ResourceName scriptRes)
{
	assert(!m_eventsActive);
	m_eventsActive = true;

	prepareEnvironment(m_state.get());
	loadLibs(m_state.get(), EventLibs);
	if (customFuncs != nullptr)
	{
		const LuaStackGuard stackGuard {m_state.get()};
		luaUqm_custom_init(m_state.get(), customFuncs);
	}

	return runScriptRes(scriptRes);
}

void UQMLua::endEvents()
{
	assert(m_eventsActive);
	m_eventsActive = false;
}

void UQMLua::callEvent(uqgsl::czstring eventId)
{
	assert(m_eventsActive);
	lua_State* const luaState {m_state.get()};
	const LuaStackGuard stackGuard {luaState};

	pushEventTable(luaState);
	lua_pushstring(luaState, eventId);
	// [-2] -> table eventTable
	// [-1] -> string eventId
	lua_gettable(luaState, -2);

	// [-2] -> table eventTable
	// [-1] -> function eventFun
	if (lua_isnil(luaState, -1))
	{
		log::warn("[script] UQMLua::callEvent(): Event '{}' is not registered.", eventId);
		return;
	}

	lua_replace(luaState, -2);
	// [-1] -> function eventFun
	callStackFunction(luaState);
}

void UQMLua::pushEventTable(lua_State* luaState)
{
	lua_getfield(luaState, LUA_REGISTRYINDEX, EventRegistryKey);
}

/////////////////////////////////////////////////////////////////////////////
// Debug console
/////////////////////////////////////////////////////////////////////////////

void UQMLua::runDebugConsole()
{
	assert(!m_debugActive);
	m_debugActive = true;

	prepareEnvironment(m_state.get());
	loadLibs(m_state.get(), DebugLibs);

	uqstl::array<char, DebugLineSize> lineBuf {};
	for (;;)
	{
		fmt::print(stdout, "> ");

		if (std::fgets(lineBuf.data(), static_cast<int>(lineBuf.size()), stdin) == nullptr)
		{
			if (std::feof(stdin) != 0)
			{
				break;
			}
			std::clearerr(stdin);
			continue;
		}
		const uqstl::string_view line {lineBuf.data()};
		if (!line.ends_with('\n') && !line.ends_with('\r'))
		{
			fmt::print(stderr, "Error: Too long command line.\n");
			// TODO: read until EOL
			continue;
		}

		runDebugLine(lineBuf.data(), stdout, stderr);
	}

	m_debugActive = false;
}

void UQMLua::runDebugLine(uqgsl::czstring line, std::FILE* out, std::FILE* err)
{
	lua_State* const luaState {m_state.get()};
	const LuaStackGuard stackGuard {luaState};

	if (luaL_loadstring(luaState, line) != LUA_OK)
	{
		fmt::print(err, "Syntax error: {}\n", lua_tostring(luaState, -1));
		return;
	}

	if (lua_pcall(luaState, 0, 1, 0) != LUA_OK)
	{
		fmt::print(err, "Runtime error: {}\n", lua_tostring(luaState, -1));
		return;
	}

	const char* const resultTypeStr {lua_typename(luaState, lua_type(luaState, -1))};
	const char* const resultStr {lua_tolstring(luaState, -1, nullptr)};
	if (resultStr == nullptr)
	{
		fmt::print(out, "({})\n", resultTypeStr);
	}
	else
	{
		fmt::print(out, "({}) {}\n", resultTypeStr, resultStr);
	}
}

/////////////////////////////////////////////////////////////////////////////
// Helpers
/////////////////////////////////////////////////////////////////////////////

uqstl::optional<uqstl::string> UQMLua::scriptFileName(ResourceName res)
{
	void* const data {res_GetResource(res)};
	if (data == nullptr)
	{
		return uqstl::nullopt;
	}

	res_DetachResource(res);
	uqstl::string fileName {static_cast<const char*>(data)};
	releaseScriptResData(data);
	return fileName;
}

void UQMLua::prepareEnvironment(lua_State* luaState)
{
	// The global environment (_ENV) of loaded code is set from _G.
	lua_newtable(luaState);
	lua_rawseti(luaState, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
	loadLibs(luaState, SafeLibs);
}

void UQMLua::loadLibs(lua_State* luaState, uqstl::span<const luaL_Reg> libs)
{
	for (const luaL_Reg& lib : libs)
	{
		loadLib(luaState, lib);
	}
}

bool UQMLua::loadScript(lua_State* luaState, const io::Directory& dir, uqstl::string_view fileName)
{
	log::debug("Loading script '{}'.", fileName);

	io::Result<io::File> in {dir.open(fileName, io::OpenMode::Read | io::OpenMode::Text)};
	if (!in)
	{
		log::error("UQMLua::loadScript(): Unable to open script file '{}' for reading.", fileName);
		return false;
	}

	ReaderState readerState {fileName, *in, uqstl::vector<char>(LoadScriptBufSize)};
	if (lua_load(luaState, readScript, &readerState, nullptr, nullptr) != LUA_OK)
	{
		log::error("UQMLua::loadScript(): lua_load() failed: {}", lua_tostring(luaState, -1));
		lua_pop(luaState, 1);
		return false;
	}
	return true;
}

bool UQMLua::runScript(lua_State* luaState, const io::Directory& dir, uqstl::string_view fileName)
{
	if (!loadScript(luaState, dir, fileName))
	{
		return false;
	}

	return callStackFunction(luaState);
}

void UQMLua::runLuaDir(lua_State* luaState, const io::Directory& dir, uqstl::string_view luaDirName)
{
	const uqstl::optional<io::Directory> luaDir {dir.subdir(luaDirName)};
	if (!luaDir)
	{
		log::warn("Could not open Lua script directory '{}'.", luaDirName);
		return;
	}

	const io::Result<uqstl::vector<uqstl::string>> luaFiles {luaDir->list("", io::Matcher {io::Matcher::Kind::Suffix, ".lua"})};
	if (!luaFiles)
	{
		log::warn("Could not read Lua script directory '{}'.", luaDirName);
		return;
	}

	log::debug("Script directory '{}': loading {} file(s).", luaDirName, luaFiles->size());
	for (const uqstl::string& fileName : *luaFiles)
	{
		runScript(luaState, *luaDir, fileName);
	}
}

bool UQMLua::callStackFunction(lua_State* luaState)
{
	if (lua_pcall(luaState, 0, 0, 0) != LUA_OK)
	{
		log::error("[script] A script error occurred in UQMLua::callStackFunction(): {}", lua_tostring(luaState, -1));
		lua_pop(luaState, 1);
		return false;
	}

	return true;
}

bool UQMLua::callFunction(lua_State* luaState, uqgsl::czstring name)
{
	const LuaStackGuard stackGuard {luaState};
	lua_getglobal(luaState, name);
	if (!lua_isfunction(luaState, -1))
	{
		return false;
	}

	return callStackFunction(luaState);
}

void UQMLua::makeEnum(lua_State* luaState, uqstl::span<const EnumValue> values)
{
	lua_createtable(luaState, 0, static_cast<int>(values.size()));
	for (const EnumValue& value : values)
	{
		lua_pushinteger(luaState, value.value);

		// [-2] -> table enumTable
		// [-1] -> int enumValue
		lua_setfield(luaState, -2, value.name);
	}
}

#ifdef DEBUG
void UQMLua::dumpStack(lua_State* luaState)
{
	const int top {lua_gettop(luaState)};
	for (int stackI {1}; stackI <= top; ++stackI)
	{
		const int type {lua_type(luaState, stackI)};
		log::debug("[{}] ({})", stackI, lua_typename(luaState, type));
		switch (type)
		{
			case LUA_TBOOLEAN:
				log::debug("    {}", lua_toboolean(luaState, stackI) != 0);
				break;
			case LUA_TNUMBER:
				log::debug("    {}", lua_tonumber(luaState, stackI));
				break;
			case LUA_TSTRING:
				log::debug("    \"{}\"", lua_tostring(luaState, stackI));
				break;
			default:
				break;
		}
	}
}
#endif

} // namespace uqm
