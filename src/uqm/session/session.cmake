############################################################


set(uqm_src_uqm_session_master_files "")


############################################################
# Local files
set(uqm_src_uqm_session_files
	"src/uqm/session/GameClock.cpp"
	"src/uqm/session/FlagshipState.h"
	"src/uqm/session/GameClock.h"
	"src/uqm/session/GameFlags.cpp"
	"src/uqm/session/GameFlags.h"
	"src/uqm/session/GameFlags.inc"
	"src/uqm/session/GameSession.cpp"
	"src/uqm/session/GameSession.h"
	"src/uqm/session/GameState.cpp"
	"src/uqm/session/GameState.h"
	"src/uqm/session/SaveCheck.cpp"
	"src/uqm/session/SaveCheck.h"
	"src/uqm/session/session.cmake"
)

source_group("src\\uqm\\session" FILES ${uqm_src_uqm_session_files})
list(APPEND uqm_src_uqm_session_master_files ${uqm_src_uqm_session_files})
