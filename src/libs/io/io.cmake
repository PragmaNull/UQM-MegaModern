############################################################


set(uqm_src_libs_io_master_files "")

############################################################
# Sub-directories
include("src/libs/io/standard/standard.cmake")
list(APPEND uqm_src_libs_io_master_files ${uqm_src_libs_io_standard_master_files})

############################################################
# Local files
set(uqm_src_libs_io_files
	"src/libs/io/FileSystem.h"
	"src/libs/io/io.cmake"
	"src/libs/io/Matcher.cpp"
	"src/libs/io/Matcher.h"
	"src/libs/io/Path.cpp"
	"src/libs/io/Path.h"
	"src/libs/io/Types.h"
)

source_group("src\\libs\\io" FILES ${uqm_src_libs_io_files})
list(APPEND uqm_src_libs_io_master_files ${uqm_src_libs_io_files})
