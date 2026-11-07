// Lua support for the game: the script resource type, script environments, and script loading.
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

#pragma once
#ifndef UQM_LUA_UQMLUA_H
#define UQM_LUA_UQMLUA_H

#include "core/stl/memory.h"
#include "core/stl/optional.h"
#include "core/stl/stl.h"
#include "core/stl/string.h"
#include "libs/io/FileSystem.h"
#include "libs/reslib.h"

#include <cstdio>
#include <lua.hpp>

struct luaUqm_custom_Function;

namespace uqm
{

// The Lua state of the game, and the script environments that use it.
class UQMLua
{
public:
	struct EnumValue
	{
		uqgsl::czstring name {};
		lua_Integer value {};
	};

	// Installs the SCRIPT resource type.
	UQMLua();
	UQMLua(const UQMLua&) = delete;
	UQMLua& operator=(const UQMLua&) = delete;
	UQMLua(UQMLua&&) = delete;
	UQMLua& operator=(UQMLua&&) = delete;
	~UQMLua() = default;

	// Makes a new Lua state and runs the game init scripts. Use at the start of a game.
	void openState();
	void closeState();

	// Prepares the communication environment and runs the script 'scriptRes', if it is not NullResource.
	// Adds a 'custom' table if 'customFuncs' is not nullptr.
	bool beginComm(const luaUqm_custom_Function* customFuncs, RESOURCE scriptRes);
	void endComm();
	// The Lua state while a communication environment is active, or nullptr.
	[[nodiscard]] lua_State* commState() const noexcept;
	[[nodiscard]] static bool needsInterpolate(uqstl::string_view str);
	// Replaces each '<% expression %>' with the result of the Lua expression.
	[[nodiscard]] uqstl::string interpolate(uqstl::string_view str);

	// Prepares the event environment and runs the script 'scriptRes'.
	bool beginEvents(const luaUqm_custom_Function* customFuncs, RESOURCE scriptRes);
	void endEvents();
	void callEvent(uqgsl::czstring eventId);

	// Reads Lua lines from stdin and runs them until end of file.
	void runDebugConsole();

	// Pushes the table of registered events.
	static void pushEventTable(lua_State* luaState);
	// Pushes a table that maps each name to its value.
	static void makeEnum(lua_State* luaState, uqstl::span<const EnumValue> values);
	// Pops a function from the stack and calls it with no arguments and no results.
	static bool callStackFunction(lua_State* luaState);
	// Calls the global function 'name' with no arguments and no results.
	static bool callFunction(lua_State* luaState, uqgsl::czstring name);

#ifdef DEBUG
	static void dumpStack(lua_State* luaState);
#endif

private:
	struct StateCloser
	{
		void operator()(lua_State* luaState) const noexcept
		{
			lua_close(luaState);
		}
	};

	// Returns the file name of the script resource 'res'.
	[[nodiscard]] static uqstl::optional<uqstl::string> scriptFileName(RESOURCE res);
	// Replaces the global table with a new table that has the safe standard libraries.
	static void prepareEnvironment(lua_State* luaState);
	// Calls the open function of each library and sets the result as a global with the name of the library.
	static void loadLibs(lua_State* luaState, uqstl::span<const luaL_Reg> libs);
	// On success, pushes the script on the stack as a function.
	[[nodiscard]] static bool loadScript(lua_State* luaState, const io::Directory& dir, uqstl::string_view fileName);
	static bool runScript(lua_State* luaState, const io::Directory& dir, uqstl::string_view fileName);
	// Runs all .lua files in the directory, in the same environment.
	static void runLuaDir(lua_State* luaState, const io::Directory& dir, uqstl::string_view luaDirName);
	// Loads the script resource and calls it.
	bool runScriptRes(RESOURCE scriptRes);

	void runInitScripts();
	void runDebugLine(uqgsl::czstring line, std::FILE* out, std::FILE* err);

	uqstl::unique_ptr<lua_State, StateCloser> m_state {};
	bool m_commActive {};
	bool m_eventsActive {};
	bool m_debugActive {};
};

// The Lua support of the running game.
[[nodiscard]] UQMLua& lua();

} // namespace uqm

#endif // UQM_LUA_UQMLUA_H
