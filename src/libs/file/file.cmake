############################################################


set(uqm_src_libs_file_master_files "")


############################################################
# Local files
set(uqm_src_libs_file_files
	"src/libs/file/file.cmake"
	"src/libs/file/paths.cpp"
	"src/libs/file/paths.h"
	"src/libs/file/textfile.cpp"
	"src/libs/file/textfile.h"
)

source_group("src\\libs\\file" FILES ${uqm_src_libs_file_files})
list(APPEND uqm_src_libs_file_master_files ${uqm_src_libs_file_files})
