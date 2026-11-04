############################################################


set(uqm_tests_uqm_master_files "")

list(APPEND uqm_tests_uqm_master_files 
	"../src/uqm/intel.h"
	"../src/uqm/creditsfile.cpp"
	"../src/uqm/creditsfile.h"
	"../src/uqm/inputscript.cpp"
	"../src/uqm/inputscript.h"
	"../src/libs/input/keyconfig.cpp"
	"../src/libs/input/keyconfig.h"
	"../src/uqm/supermelee/meleeconfig.cpp"
	"../src/uqm/supermelee/meleeconfig.h"
	"../src/uqm/menudef.cpp"
	"../src/uqm/menudef.h"
	"../src/uqm/menuwidgets.cpp"
	"../src/uqm/menuwidgets.h"
	"../src/uqm/displist.cpp"
	"../src/uqm/displist.h"
	"../src/uqm/session/GameClock.cpp"
	"../src/uqm/session/GameClock.h"
	"../src/uqm/session/GameFlags.cpp"
	"../src/uqm/session/GameFlags.h"
	"../src/uqm/slideshow.cpp"
	"../src/uqm/slideshow.h"
)

############################################################
# Local files
set(uqm_tests_uqm_files
	"tests/uqm/uqm.cmake"
	"tests/uqm/test_contentfiles.cpp"
	"tests/uqm/test_creditsfile.cpp"
	"tests/uqm/test_flagshipstate.cpp"
	"tests/uqm/test_gameclock.cpp"
	"tests/uqm/test_gameflags.cpp"
	"tests/uqm/test_intel.cpp"
	"tests/uqm/test_inputscript.cpp"
	"tests/uqm/test_keyconfig.cpp"
	"tests/uqm/test_meleeconfig.cpp"
	"tests/uqm/test_menudef.cpp"
	"tests/uqm/test_menuwidgets.cpp"
	"tests/uqm/test_slideshow.cpp"
)

source_group("tests\\uqm" FILES ${uqm_tests_uqm_files})
list(APPEND uqm_tests_uqm_master_files ${uqm_tests_uqm_files})
