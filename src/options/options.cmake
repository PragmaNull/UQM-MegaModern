############################################################


set(uqm_src_options_master_files "")


############################################################
# Local files
set(uqm_src_options_files
	"src/options/OptionConstants.h"
	"src/options/OptionDefs.cpp"
	"src/options/OptionDefs.h"
	"src/options/OptionTypes.h"
	"src/options/netoptions.cpp"
	"src/options/netoptions.h"
	"src/options/options.cmake"
	"src/options/options.cpp"
	"src/options/options.h"
	"src/options/Setting.cpp"
	"src/options/Setting.h"
	"src/options/SettingsCli.cpp"
	"src/options/SettingsCli.h"
)

source_group("src\\options" FILES ${uqm_src_options_files})
list(APPEND uqm_src_options_master_files ${uqm_src_options_files})
