############################################################


set(uqm_src_core_master_files "")

############################################################
# Sub-directories
include("src/core/log/log.cmake")
list(APPEND uqm_src_core_master_files ${uqm_src_core_log_master_files})
include("src/core/memory/memory.cmake")
list(APPEND uqm_src_core_master_files ${uqm_src_core_memory_master_files})
include("src/core/misc/misc.cmake")
list(APPEND uqm_src_core_master_files ${uqm_src_core_misc_master_files})
include("src/core/platform/platform.cmake")
list(APPEND uqm_src_core_master_files ${uqm_src_core_platform_master_files})
include("src/core/stl/stl.cmake")
list(APPEND uqm_src_core_master_files ${uqm_src_core_stl_master_files})
include("src/core/string/string.cmake")
list(APPEND uqm_src_core_master_files ${uqm_src_core_string_master_files})
include("src/core/thread/thread.cmake")
list(APPEND uqm_src_core_master_files ${uqm_src_core_thread_master_files})
include("src/core/types/types.cmake")
list(APPEND uqm_src_core_master_files ${uqm_src_core_types_master_files})

############################################################
# Local files
set(uqm_src_core_files
	"src/core/core.cmake"
)

source_group("src\\core" FILES ${uqm_src_core_files})
list(APPEND uqm_src_core_master_files ${uqm_src_core_files})
