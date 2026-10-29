############################################################


set(uqm_src_core_string_master_files "")


############################################################
# Local files
set(uqm_src_core_string_files
	"src/core/string/StringUtils.cpp"
	"src/core/string/StringUtils.h"
	"src/core/string/string.cmake"
	"src/core/string/Utf8.cpp"
	"src/core/string/Utf8.h"
)

source_group("src\\core\\string" FILES ${uqm_src_core_string_files})
list(APPEND uqm_src_core_string_master_files ${uqm_src_core_string_files})
