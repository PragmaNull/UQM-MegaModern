############################################################


set(uqm_src_uqm_supermelee_netplay_master_files "")

############################################################
# Sub-directories
include("src/uqm/supermelee/netplay/proto/proto.cmake")
list(APPEND uqm_src_uqm_supermelee_netplay_master_files ${uqm_src_uqm_supermelee_netplay_proto_master_files})

############################################################
# Local files
set(uqm_src_uqm_supermelee_netplay_files
#	"src/uqm/supermelee/netplay/FILES"
	"src/uqm/supermelee/netplay/checkbuf.cpp"
	"src/uqm/supermelee/netplay/checkbuf.h"
	"src/uqm/supermelee/netplay/checksum.cpp"
	"src/uqm/supermelee/netplay/checksum.h"
	"src/uqm/supermelee/netplay/crc.cpp"
	"src/uqm/supermelee/netplay/crc.h"
	"src/uqm/supermelee/netplay/nc_connect.ci"
	"src/uqm/supermelee/netplay/netconnection.cpp"
	"src/uqm/supermelee/netplay/netconnection.h"
	"src/uqm/supermelee/netplay/netcontext.cpp"
	"src/uqm/supermelee/netplay/netcontext.h"
	"src/uqm/supermelee/netplay/netinput.cpp"
	"src/uqm/supermelee/netplay/netinput.h"
	"src/uqm/supermelee/netplay/netmelee.cpp"
	"src/uqm/supermelee/netplay/netmelee.h"
	"src/uqm/supermelee/netplay/netmisc.cpp"
	"src/uqm/supermelee/netplay/netmisc.h"
	"src/uqm/supermelee/netplay/netplay.cmake"
	"src/uqm/supermelee/netplay/netplay.h"
	"src/uqm/supermelee/netplay/netrcv.cpp"
	"src/uqm/supermelee/netplay/netrcv.h"
	"src/uqm/supermelee/netplay/netsend.cpp"
	"src/uqm/supermelee/netplay/netsend.h"
	"src/uqm/supermelee/netplay/netstate.cpp"
	"src/uqm/supermelee/netplay/netstate.h"
	"src/uqm/supermelee/netplay/notify.cpp"
	"src/uqm/supermelee/netplay/notify.h"
	"src/uqm/supermelee/netplay/notifyall.cpp"
	"src/uqm/supermelee/netplay/notifyall.h"
	"src/uqm/supermelee/netplay/packet.cpp"
	"src/uqm/supermelee/netplay/packet.h"
	"src/uqm/supermelee/netplay/packethandlers.cpp"
	"src/uqm/supermelee/netplay/packethandlers.h"
	"src/uqm/supermelee/netplay/packetq.cpp"
	"src/uqm/supermelee/netplay/packetq.h"
	"src/uqm/supermelee/netplay/packetsenders.cpp"
	"src/uqm/supermelee/netplay/packetsenders.h"
)

source_group("src\\uqm\\supermelee\\netplay" FILES ${uqm_src_uqm_supermelee_netplay_files})
list(APPEND uqm_src_uqm_supermelee_netplay_master_files ${uqm_src_uqm_supermelee_netplay_files})
