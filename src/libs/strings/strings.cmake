############################################################


set(uqm_src_libs_strings_master_files "")


############################################################
# Local files
set(uqm_src_libs_strings_files
	"src/libs/strings/getstr.cpp"
	"src/libs/strings/sfileins.cpp"
	"src/libs/strings/sresins.cpp"
	"src/libs/strings/strings.cmake"
	"src/libs/strings/strings.cpp"
	"src/libs/strings/strintrn.h"
	"src/libs/strings/textalign.cpp"
)

source_group("src\\libs\\strings" FILES ${uqm_src_libs_strings_files})
list(APPEND uqm_src_libs_strings_master_files ${uqm_src_libs_strings_files})
