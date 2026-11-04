############################################################


set(uqm_doc_devel_master_files "")

############################################################
# Sub-directories
include("doc/devel/netplay/netplay.cmake")
list(APPEND uqm_doc_devel_master_files ${uqm_doc_devel_netplay_master_files})

############################################################
# Local files
set(uqm_doc_devel_files
	"doc/devel/Hyperspace Coordinate Math [LOG_X-Y].txt"
	"doc/devel/Hyperspace Coordinate Math [UNIVERSE_X-Y].txt"
	"doc/devel/aniformat"
	"doc/devel/battleinput"
	"doc/devel/blue-gas-giant-pal.png"
	"doc/devel/checklist"
	"doc/devel/contentfiles"
	"doc/devel/debug"
	"doc/devel/devel.cmake"
	"doc/devel/dialogs"
	"doc/devel/files"
	"doc/devel/fntformat"
	"doc/devel/fontres"
	"doc/devel/generate"
	"doc/devel/gfxlib"
	"doc/devel/gfxres"
	"doc/devel/gfxversions"
	"doc/devel/glossary"
	"doc/devel/historical"
	"doc/devel/input"
	"doc/devel/io"
	"doc/devel/livecd"
	"doc/devel/lua"
	"doc/devel/luasyntax"
	"doc/devel/meleeteams"
	"doc/devel/musicres"
	"doc/devel/orggenerate"
	"doc/devel/pkgformat"
	"doc/devel/planetrender"
	"doc/devel/planetrotate"
	"doc/devel/planettopo"
	"doc/devel/queues"
	"doc/devel/racestrings"
	"doc/devel/resources"
	"doc/devel/savefile"
	"doc/devel/sc1"
	"doc/devel/script"
	"doc/devel/sfx"
	"doc/devel/statefiles"
	"doc/devel/strtab"
	"doc/devel/threads"
	"doc/devel/timing"
	"doc/devel/versions"
	"doc/devel/voiceeffects"
)

source_group("doc\\devel" FILES ${uqm_doc_devel_files})
list(APPEND uqm_doc_devel_master_files ${uqm_doc_devel_files})
