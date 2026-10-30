############################################################


set(uqm_src_libs_graphics_imgui_master_files "")


############################################################
# Local files
set(uqm_src_libs_graphics_imgui_files
	"src/libs/graphics/imgui/imgui.cmake"
	"src/libs/graphics/imgui/imguilayer.cpp"
	"src/libs/graphics/imgui/imguilayer.h"
	"src/libs/graphics/imgui/toolwindows.cpp"
	"src/libs/graphics/imgui/toolwindows.h"
)

source_group("src\\libs\\graphics\\imgui" FILES ${uqm_src_libs_graphics_imgui_files})
list(APPEND uqm_src_libs_graphics_imgui_master_files ${uqm_src_libs_graphics_imgui_files})
