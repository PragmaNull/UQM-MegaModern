############################################################


set(uqm_src_libs_network_master_files "")

############################################################
# Sub-directories
include("src/libs/network/connect/connect.cmake")
list(APPEND uqm_src_libs_network_master_files ${uqm_src_libs_network_connect_master_files})
include("src/libs/network/netmanager/netmanager.cmake")
list(APPEND uqm_src_libs_network_master_files ${uqm_src_libs_network_netmanager_master_files})
include("src/libs/network/socket/socket.cmake")
list(APPEND uqm_src_libs_network_master_files ${uqm_src_libs_network_socket_master_files})

############################################################
# Local files
set(uqm_src_libs_network_files
#	"src/libs/network/FILES"
	"src/libs/network/bytesex.h"
	"src/libs/network/Connection.cpp"
	"src/libs/network/Connection.h"
	"src/libs/network/Connector.cpp"
	"src/libs/network/Connector.h"
	"src/libs/network/Listener.cpp"
	"src/libs/network/Listener.h"
	"src/libs/network/NetContext.cpp"
	"src/libs/network/NetContext.h"
	"src/libs/network/NetImpl.h"
	"src/libs/network/netport.cpp"
	"src/libs/network/netport.h"
	"src/libs/network/network.cmake"
	"src/libs/network/network.h"
#	"src/libs/network/network_bsd.cpp"
	"src/libs/network/network_win.cpp"
	"src/libs/network/wspiapiwrap.cpp"
	"src/libs/network/wspiapiwrap.h"
)

source_group("src\\libs\\network" FILES ${uqm_src_libs_network_files})
list(APPEND uqm_src_libs_network_master_files ${uqm_src_libs_network_files})
