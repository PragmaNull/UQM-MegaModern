############################################################


set(uqm_src_core_thread_master_files "")


############################################################
# Local files
set(uqm_src_core_thread_files
	"src/core/thread/Mutex.h"
	"src/core/thread/thread.cmake"
	"src/core/thread/Thread.cpp"
	"src/core/thread/Thread.h"
	"src/core/thread/ThreadSafety.h"
)

source_group("src\\core\\thread" FILES ${uqm_src_core_thread_files})
list(APPEND uqm_src_core_thread_master_files ${uqm_src_core_thread_files})
