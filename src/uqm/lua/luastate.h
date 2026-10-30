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

#ifndef UQM_LUA_LUASTATE_H_
#define UQM_LUA_LUASTATE_H_

#include "core/types/bytes.h"
#include "libs/luauqm/luauqm.h"

extern lua_State* luaUqm_globalState;

typedef struct delta_types
{
	lua_Integer _int;
	bool _bool;
	char* _str;
	char _char;
} DELTA_TYPES;

extern DELTA_TYPES luaUqm_delta;

void luaUqm_initState(void);
void luaUqm_uninitState(void);
void luaUqm_reinitState(void);
void luaUqm_getProp(lua_State* luaState, int nameIndex);
void luaUqm_setProp(lua_State* luaState, int nameIndex, int valueIndex);
int luaUqm_checkPropValueType(lua_State* luaState, const char* funName,
							  int nameIndex);

void setGameStateUint(const char* name, uint32_t val);
uint32_t getGameStateUint(const char* name);

void luaUqm_getEventTable(lua_State* luaState);


#endif /* UQM_LUA_LUASTATE_H_ */
