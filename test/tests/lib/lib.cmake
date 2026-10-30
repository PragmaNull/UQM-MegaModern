############################################################


set(uqm_tests_lib_master_files "")

list(APPEND uqm_tests_lib_master_files 
	"../src/lib/Math2D/Math2D.h"
	"../src/lib/Math2D/Math2D.cpp"
	"../src/lib/Math2D/Math2DStringUtils.h"
	"../src/lib/Math2D/Math2DStringUtils.cpp"
	"../src/core/platform/platform.h"
	"../src/core/platform/platform_win.cpp"
	"../src/libs/file/paths.cpp"
	"../src/libs/file/paths.h"
	"../src/libs/io/FileSystem.h"
	"../src/libs/io/Matcher.cpp"
	"../src/libs/io/Matcher.h"
	"../src/libs/io/Path.cpp"
	"../src/libs/io/Path.h"
	"../src/libs/io/Types.h"
	"../src/libs/io/standard/DirectorySource.cpp"
	"../src/libs/io/standard/DirectorySource.h"
	"../src/libs/io/standard/File.cpp"
	"../src/libs/io/standard/File.h"
	"../src/libs/io/standard/FileSystem.cpp"
	"../src/libs/io/standard/FileSystem.h"
	"../src/libs/io/standard/MountSource.h"
	"../src/libs/io/standard/MountTable.cpp"
	"../src/libs/io/standard/MountTable.h"
	"../src/libs/io/standard/Stream.h"
	"../src/libs/io/standard/ZipSource.cpp"
	"../src/libs/io/standard/ZipSource.h"
)


############################################################
# Local files
set(uqm_tests_lib_files
	"tests/lib/lib.cmake"
	"tests/lib/test_io.cpp"
	"tests/lib/test_paths.cpp"
	"tests/lib/test_Math2D.cpp"
)

source_group("tests\\lib" FILES ${uqm_tests_lib_files})
list(APPEND uqm_tests_lib_master_files ${uqm_tests_lib_files})
