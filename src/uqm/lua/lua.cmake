############################################################


set(uqm_src_uqm_lua_master_files "")

############################################################
# Sub-directories
include("src/uqm/lua/luafuncs/luafuncs.cmake")
list(APPEND uqm_src_uqm_lua_master_files ${uqm_src_uqm_lua_luafuncs_master_files})

############################################################
# Local files
set(uqm_src_uqm_lua_files
	"src/uqm/lua/lua.cmake"
	"src/uqm/lua/UQMLua.cpp"
	"src/uqm/lua/UQMLua.h"
)

source_group("src\\uqm\\lua" FILES ${uqm_src_uqm_lua_files})
list(APPEND uqm_src_uqm_lua_master_files ${uqm_src_uqm_lua_files})
