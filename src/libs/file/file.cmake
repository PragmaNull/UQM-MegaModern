############################################################


set(uqm_src_libs_file_master_files "")


############################################################
# Local files
set(uqm_src_libs_file_files
	"src/libs/file/dirs.cpp"
	"src/libs/file/file.cmake"
	"src/libs/file/files.cpp"
	"src/libs/file/filintrn.h"
	"src/libs/file/textfile.cpp"
	"src/libs/file/textfile.h"
)

source_group("src\\libs\\file" FILES ${uqm_src_libs_file_files})
list(APPEND uqm_src_libs_file_master_files ${uqm_src_libs_file_files})
