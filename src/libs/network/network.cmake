############################################################


set(uqm_src_libs_network_master_files "")

############################################################
# Local files
set(uqm_src_libs_network_files
	"src/libs/network/Connection.cpp"
	"src/libs/network/Connection.h"
	"src/libs/network/Connector.cpp"
	"src/libs/network/Connector.h"
	"src/libs/network/Listener.cpp"
	"src/libs/network/Listener.h"
	"src/libs/network/NetContext.cpp"
	"src/libs/network/NetContext.h"
	"src/libs/network/NetImpl.h"
	"src/libs/network/network.cmake"
)

source_group("src\\libs\\network" FILES ${uqm_src_libs_network_files})
list(APPEND uqm_src_libs_network_master_files ${uqm_src_libs_network_files})
