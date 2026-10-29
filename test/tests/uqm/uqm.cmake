############################################################


set(uqm_tests_uqm_master_files "")

list(APPEND uqm_tests_uqm_master_files 
	"../src/uqm/intel.h"
	"../src/libs/input/keyconfig.cpp"
	"../src/libs/input/keyconfig.h"
	"../src/uqm/supermelee/meleeconfig.cpp"
	"../src/uqm/supermelee/meleeconfig.h"
	"../src/uqm/menudef.cpp"
	"../src/uqm/menudef.h"
	"../src/uqm/menuwidgets.cpp"
	"../src/uqm/menuwidgets.h"
)

############################################################
# Local files
set(uqm_tests_uqm_files
	"tests/uqm/uqm.cmake"
	"tests/uqm/test_intel.cpp"
	"tests/uqm/test_keyconfig.cpp"
	"tests/uqm/test_meleeconfig.cpp"
	"tests/uqm/test_menudef.cpp"
	"tests/uqm/test_menuwidgets.cpp"
)

source_group("tests\\uqm" FILES ${uqm_tests_uqm_files})
list(APPEND uqm_tests_uqm_master_files ${uqm_tests_uqm_files})
