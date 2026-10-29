############################################################


set(uqm_src_libs_io_standard_master_files "")


############################################################
# Local files
set(uqm_src_libs_io_standard_files
	"src/libs/io/standard/DirectorySource.cpp"
	"src/libs/io/standard/DirectorySource.h"
	"src/libs/io/standard/File.cpp"
	"src/libs/io/standard/File.h"
	"src/libs/io/standard/FileSystem.cpp"
	"src/libs/io/standard/FileSystem.h"
	"src/libs/io/standard/MountSource.h"
	"src/libs/io/standard/MountTable.cpp"
	"src/libs/io/standard/MountTable.h"
	"src/libs/io/standard/standard.cmake"
	"src/libs/io/standard/Stream.h"
	"src/libs/io/standard/ZipSource.cpp"
	"src/libs/io/standard/ZipSource.h"
)

source_group("src\\libs\\io\\standard" FILES ${uqm_src_libs_io_standard_files})
list(APPEND uqm_src_libs_io_standard_master_files ${uqm_src_libs_io_standard_files})
