############################################################


set(uqm_src_libs_math_master_files "")


############################################################
# Local files
set(uqm_src_libs_math_files
	"src/libs/math/math.cmake"
	"src/libs/math/mthintrn.h"
	"src/libs/math/random.cpp"
	"src/libs/math/random.h"
	"src/libs/math/random2.cpp"
	"src/libs/math/randomdefs.h"
)

source_group("src\\libs\\math" FILES ${uqm_src_libs_math_files})
list(APPEND uqm_src_libs_math_master_files ${uqm_src_libs_math_files})
