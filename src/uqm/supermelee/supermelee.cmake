############################################################


set(uqm_src_uqm_supermelee_master_files "")

############################################################
# Sub-directories
include("src/uqm/supermelee/netplay/netplay.cmake")
list(APPEND uqm_src_uqm_supermelee_master_files ${uqm_src_uqm_supermelee_netplay_master_files})

############################################################
# Local files
set(uqm_src_uqm_supermelee_files
	"src/uqm/supermelee/buildpick.cpp"
	"src/uqm/supermelee/buildpick.h"
	"src/uqm/supermelee/loadmele.cpp"
	"src/uqm/supermelee/loadmele.h"
	"src/uqm/supermelee/melee.cpp"
	"src/uqm/supermelee/meleeconfig.cpp"
	"src/uqm/supermelee/meleeconfig.h"
	"src/uqm/supermelee/melee.h"
	"src/uqm/supermelee/meleesetup.cpp"
	"src/uqm/supermelee/meleesetup.h"
	"src/uqm/supermelee/meleeship.h"
	"src/uqm/supermelee/pickmele.cpp"
	"src/uqm/supermelee/pickmele.h"
	"src/uqm/supermelee/supermelee.cmake"
)

source_group("src\\uqm\\supermelee" FILES ${uqm_src_uqm_supermelee_files})
list(APPEND uqm_src_uqm_supermelee_master_files ${uqm_src_uqm_supermelee_files})
