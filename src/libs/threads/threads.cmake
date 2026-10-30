############################################################


set(uqm_src_libs_threads_master_files "")

############################################################
# Local files
set(uqm_src_libs_threads_files
	"src/libs/threads/threads.cmake"
	"src/libs/threads/threads.cpp"
)

source_group("src\\libs\\threads" FILES ${uqm_src_libs_threads_files})
list(APPEND uqm_src_libs_threads_master_files ${uqm_src_libs_threads_files})
