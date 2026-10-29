############################################################


set(uqm_src_libs_uio_master_files "")

############################################################
# Local files
set(uqm_src_libs_uio_files
	"src/libs/uio/compat.cpp"
	"src/libs/uio/io.h"
	"src/libs/uio/uio.cmake"
)

source_group("src\\libs\\uio" FILES ${uqm_src_libs_uio_files})
list(APPEND uqm_src_libs_uio_master_files ${uqm_src_libs_uio_files})
