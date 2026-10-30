############################################################


set(uqm_src_core_types_master_files "")


############################################################
# Local files
set(uqm_src_core_types_files
	"src/core/types/enum.h"
	"src/core/types/types.cmake"
)

source_group("src\\core\\types" FILES ${uqm_src_core_types_files})
list(APPEND uqm_src_core_types_master_files ${uqm_src_core_types_files})
