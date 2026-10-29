############################################################


set(uqm_src_libs_io_master_files "")

############################################################
# Sub-directories
include("src/libs/io/${UQM_IO_BACKEND}/${UQM_IO_BACKEND}.cmake")
list(APPEND uqm_src_libs_io_master_files ${uqm_src_libs_io_${UQM_IO_BACKEND}_master_files})

############################################################
# Local files
set(uqm_src_libs_io_files
	"src/libs/io/Backend.h.in"
	"src/libs/io/Concepts.h"
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
