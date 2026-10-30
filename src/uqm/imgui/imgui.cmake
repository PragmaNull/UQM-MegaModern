############################################################


set(uqm_src_uqm_imgui_master_files "")


############################################################
# Local files
set(uqm_src_uqm_imgui_files
	"src/uqm/imgui/gamestatewindow.cpp"
	"src/uqm/imgui/gamestatewindow.h"
	"src/uqm/imgui/gamethread.cpp"
	"src/uqm/imgui/gamethread.h"
	"src/uqm/imgui/imgui.cmake"
	"src/uqm/imgui/settingswindow.cpp"
	"src/uqm/imgui/settingswindow.h"
)

source_group("src\\uqm\\imgui" FILES ${uqm_src_uqm_imgui_files})
list(APPEND uqm_src_uqm_imgui_master_files ${uqm_src_uqm_imgui_files})
