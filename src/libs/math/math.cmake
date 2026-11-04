############################################################


set(uqm_src_libs_math_master_files "")


############################################################
# Local files
set(uqm_src_libs_math_files
	"src/libs/math/math.cmake"
	"src/libs/math/mthintrn.h"
	"src/libs/math/Random.h"
	"src/libs/math/randomdefs.h"
)

source_group("src\\libs\\math" FILES ${uqm_src_libs_math_files})
list(APPEND uqm_src_libs_math_master_files ${uqm_src_libs_math_files})
