############################################################


set(uqm_src_uqm_random_master_files "")


############################################################
# Local files
set(uqm_src_uqm_random_files
	"src/uqm/random/UQMRandom.cpp"
	"src/uqm/random/UQMRandom.h"
	"src/uqm/random/random.cmake"
)

source_group("src\\uqm\\random"FILES ${uqm_src_uqm_random_files})
list(APPEND uqm_src_uqm_random_master_files ${uqm_src_uqm_random_files})
