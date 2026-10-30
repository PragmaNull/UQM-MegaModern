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
 * This file contains code for connecting storing the game state in Lua.
 */

#include "luastate.h"
#include "luainit.h"
#include "uqm/globdata.h"
#include "core/log/log.h"
#include "libs/luauqm/luauqm.h"


// The Lua state of the current game. The game state flags are in GameState.

static void luaUqm_initEventTable(lua_State* luaState);

lua_State* luaUqm_globalState = nullptr;
static const char eventRegistryKey[] =
	"uqm_event_registryKey";

DELTA_TYPES luaUqm_delta;

// Init the global Lua state. Called at the start of a game.
void luaUqm_initState(void)
{
	if (luaUqm_globalState != nullptr)
	{
		uqm::log::warn("Lua state multiply uninitialized");
		luaUqm_uninitState();
	}
	luaUqm_globalState = luaL_newstate();
	luaUqm_initEventTable(luaUqm_globalState);

	// XXX TODO: set up an alternative to the Lua 'require' function,
	// which makes use of uio.

	luaUqm_runInitScripts();
}

// Uninit the global Lua state.
void luaUqm_uninitState(void)
{
	if (luaUqm_globalState != nullptr)
	{
		lua_close(luaUqm_globalState);
		luaUqm_globalState = nullptr;
	}
	else
	{
		uqm::log::warn("Lua state multiply uninitialized");
	}
}

/////////////////////////////////////////////////////////////////////////////
// Game events
/////////////////////////////////////////////////////////////////////////////

static void
luaUqm_initEventTable(lua_State* luaState)
{
	lua_pushstring(luaState, eventRegistryKey);
	lua_newtable(luaState);
	lua_settable(luaState, LUA_REGISTRYINDEX);
}

void luaUqm_getEventTable(lua_State* luaState)
{
	lua_getfield(luaState, LUA_REGISTRYINDEX, eventRegistryKey);
}
