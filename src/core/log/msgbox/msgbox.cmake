############################################################


set(uqm_src_core_log_msgbox_master_files "")


############################################################
# Local files
set(uqm_src_core_log_msgbox_files
	"src/core/log/msgbox/msgbox.cmake"
	"src/core/log/msgbox/msgbox.h"
#	"src/core/log/msgbox/msgbox_macosx.m"
#	"src/core/log/msgbox/msgbox_null.cpp"
)
if (WIN32)
	list(APPEND uqm_src_core_log_msgbox_files "src/core/log/msgbox/msgbox_win.cpp")
else()
	list(APPEND uqm_src_core_log_msgbox_files "src/core/log/msgbox/msgbox_sdl.cpp")
endif()

source_group("src\\core\\log\\msgbox" FILES ${uqm_src_core_log_msgbox_files})
list(APPEND uqm_src_core_log_msgbox_master_files ${uqm_src_core_log_msgbox_files})
