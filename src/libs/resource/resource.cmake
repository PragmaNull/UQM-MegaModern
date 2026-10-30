############################################################


set(uqm_src_libs_resource_master_files "")


############################################################
# Local files
set(uqm_src_libs_resource_files
	"src/libs/resource/direct.cpp"
	"src/libs/resource/filecntl.cpp"
	"src/libs/resource/getres.cpp"
	"src/libs/resource/index.h"
	"src/libs/resource/propfile.cpp"
	"src/libs/resource/propfile.h"
	"src/libs/resource/resinit.cpp"
	"src/libs/resource/resintrn.h"
	"src/libs/resource/resource.cmake"
	"src/libs/resource/stringbank.cpp"
	"src/libs/resource/stringbank.h"
)

source_group("src\\libs\\resource" FILES ${uqm_src_libs_resource_files})
list(APPEND uqm_src_libs_resource_master_files ${uqm_src_libs_resource_files})
