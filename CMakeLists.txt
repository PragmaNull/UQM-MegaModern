cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

set(CMAKE_SYSTEM_VERSION 10.0 CACHE STRING "" FORCE)

project(UrQuanMastersMegaModern CXX)

################################################################################
# Set target arch type if empty. Visual studio solution generator provides it.
################################################################################
if(NOT CMAKE_VS_PLATFORM_NAME)
    set(CMAKE_VS_PLATFORM_NAME "x64")
endif()
message("${CMAKE_VS_PLATFORM_NAME} architecture in use")

#if(NOT ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32"))
#    message(FATAL_ERROR "${CMAKE_VS_PLATFORM_NAME} arch is not supported!")
#endif()

################################################################################
# Global configuration types
################################################################################
set(CMAKE_CONFIGURATION_TYPES
    "Debug"
    "Release"
    CACHE STRING "" FORCE
)

################################################################################
# Global compiler options
################################################################################
if(MSVC)
    # remove default flags provided with CMake for MSVC
    set(CMAKE_C_FLAGS "")
    set(CMAKE_C_FLAGS_DEBUG "")
    set(CMAKE_C_FLAGS_RELEASE "")
endif()

################################################################################
# Global linker options
################################################################################
if(MSVC)
    # remove default flags provided with CMake for MSVC
    set(CMAKE_EXE_LINKER_FLAGS "")
    set(CMAKE_MODULE_LINKER_FLAGS "")
    set(CMAKE_SHARED_LINKER_FLAGS "")
    set(CMAKE_STATIC_LINKER_FLAGS "")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS}")
endif()

################################################################################
# Nuget packages function stub.
################################################################################
function(use_package TARGET PACKAGE VERSION)
    message(WARNING "No implementation of use_package. Create yours. "
                    "Package \"${PACKAGE}\" with version \"${VERSION}\" "
                    "for target \"${TARGET}\" is ignored!")
endfunction()

################################################################################
# Common utils
################################################################################
include(CMake/Utils.cmake)

################################################################################
# Additional Global Settings(add specific info there)
################################################################################
include(CMake/GlobalSettingsInclude.cmake OPTIONAL)

################################################################################
# Use solution folders feature
################################################################################
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

################################################################################
# Sub-projects
################################################################################



























set(PROJECT_NAME UrQuanMasters)

################################################################################
# Source groups
################################################################################
set(Doc
    "BUGS"
    "ChangeLog"
    "TODO.txt"
)
source_group("Doc" FILES ${Doc})

set(Doc__Devel
    "doc/devel/aniformat"
    "doc/devel/checklist"
    "doc/devel/debug"
    "doc/devel/dialogs"
    "doc/devel/files"
    "doc/devel/fntformat"
    "doc/devel/fontres"
    "doc/devel/generate"
    "doc/devel/gfxlib"
    "doc/devel/gfxres"
    "doc/devel/gfxversions"
    "doc/devel/glossary"
    "doc/devel/input"
    "doc/devel/lua"
    "doc/devel/luasyntax"
    "doc/devel/musicres"
    "doc/devel/pkgformat"
    "doc/devel/planetrender"
    "doc/devel/planetrotate"
    "doc/devel/plugins"
    "doc/devel/resources"
    "doc/devel/script"
    "doc/devel/sfx"
    "doc/devel/strtab"
    "doc/devel/threads"
    "doc/devel/timing"
)
source_group("Doc\\Devel" FILES ${Doc__Devel})

set(Doc__Users
    "doc/users/manual.txt"
    "doc/users/unixinstall"
)
source_group("Doc\\Users" FILES ${Doc__Users})

set(Resources
    "src/res/kohr-ah1.ico"
    "src/res/sis1.ico"
    "src/res/starcon2.ico"
    "src/res/ur-quan-icon-alpha.ico"
    "src/res/ur-quan-icon-std.ico"
    "src/res/ur-quan1.ico"
    "src/res/ur-quan2.ico"
)
source_group("Resources" FILES ${Resources})

set(Source_Files
    "src/config.h"
    "src/config_vc6.h"
    "src/endian_uqm.h"
    "src/options.c"
    "src/options.h"
    "src/port.c"
    "src/port.h"
    "src/types.h"
    "src/uqm.c"
    "src/uqmversion.h"
)
source_group("Source Files" FILES ${Source_Files})

set(Source_Files___core__memory
    "src/core/memory/memory.h"
)
source_group("Source Files\\_core\\memory" FILES ${Source_Files___core__memory})

set(Source_Files__getopt
    "src/getopt/getopt.c"
    "src/getopt/getopt.h"
    "src/getopt/getopt1.c"
)
source_group("Source Files\\getopt" FILES ${Source_Files__getopt})

set(Source_Files__libs
    "src/libs/alarm.h"
    "src/libs/async.h"
    "src/libs/callback.h"
    "src/libs/compiler.h"
    "src/libs/declib.h"
    "src/libs/file.h"
    "src/libs/gfxlib.h"
    "src/libs/heap.h"
    "src/libs/inplib.h"
    "src/libs/list.h"
    "src/libs/log.h"
    "src/libs/mathlib.h"
    "src/libs/md5.h"
    "src/libs/memlib.h"
    "src/libs/misc.h"
    "src/libs/net.h"
    "src/libs/platform.h"
    "src/libs/reslib.h"
    "src/libs/sndlib.h"
    "src/libs/strlib.h"
    "src/libs/tasklib.h"
    "src/libs/threadlib.h"
    "src/libs/timelib.h"
    "src/libs/uio.h"
    "src/libs/unicode.h"
    "src/libs/vidlib.h"
)
source_group("Source Files\\libs" FILES ${Source_Files__libs})

set(Source_Files__libs__callback
    "src/libs/callback/alarm.c"
    "src/libs/callback/alarm.h"
    "src/libs/callback/async.c"
    "src/libs/callback/async.h"
    "src/libs/callback/callback.c"
    "src/libs/callback/callback.h"
)
source_group("Source Files\\libs\\callback" FILES ${Source_Files__libs__callback})

set(Source_Files__libs__decomp
    "src/libs/decomp/lzdecode.c"
    "src/libs/decomp/lzencode.c"
    "src/libs/decomp/lzh.h"
    "src/libs/decomp/update.c"
)
source_group("Source Files\\libs\\decomp" FILES ${Source_Files__libs__decomp})

set(Source_Files__libs__file
    "src/libs/file/dirs.c"
    "src/libs/file/files.c"
    "src/libs/file/filintrn.h"
)
source_group("Source Files\\libs\\file" FILES ${Source_Files__libs__file})

set(Source_Files__libs__graphics
    "src/libs/graphics/bbox.c"
    "src/libs/graphics/bbox.h"
    "src/libs/graphics/boxint.c"
    "src/libs/graphics/clipline.c"
    "src/libs/graphics/cmap.c"
    "src/libs/graphics/cmap.h"
    "src/libs/graphics/context.c"
    "src/libs/graphics/context.h"
    "src/libs/graphics/dcqueue.c"
    "src/libs/graphics/dcqueue.h"
    "src/libs/graphics/drawable.c"
    "src/libs/graphics/drawable.h"
    "src/libs/graphics/drawcmd.h"
    "src/libs/graphics/filegfx.c"
    "src/libs/graphics/font.c"
    "src/libs/graphics/font.h"
    "src/libs/graphics/frame.c"
    "src/libs/graphics/gfx_common.c"
    "src/libs/graphics/gfx_common.h"
    "src/libs/graphics/gfxintrn.h"
    "src/libs/graphics/gfxload.c"
    "src/libs/graphics/intersec.c"
    "src/libs/graphics/loaddisp.c"
    "src/libs/graphics/pixmap.c"
    "src/libs/graphics/prim.h"
    "src/libs/graphics/resgfx.c"
    "src/libs/graphics/tfb_draw.c"
    "src/libs/graphics/tfb_draw.h"
    "src/libs/graphics/tfb_prim.c"
    "src/libs/graphics/tfb_prim.h"
    "src/libs/graphics/widgets.c"
    "src/libs/graphics/widgets.h"
)
source_group("Source Files\\libs\\graphics" FILES ${Source_Files__libs__graphics})

set(Source_Files__libs__graphics__sdl
    "src/libs/graphics/sdl/2xscalers.c"
    "src/libs/graphics/sdl/2xscalers.h"
    "src/libs/graphics/sdl/2xscalers_3dnow.c"
    "src/libs/graphics/sdl/2xscalers_mmx.c"
    "src/libs/graphics/sdl/2xscalers_mmx.h"
    "src/libs/graphics/sdl/2xscalers_sse.c"
    "src/libs/graphics/sdl/biadv2x.c"
    "src/libs/graphics/sdl/bilinear2x.c"
    "src/libs/graphics/sdl/canvas.c"
    "src/libs/graphics/sdl/clipboard.c"
    "src/libs/graphics/sdl/hq2x.c"
    "src/libs/graphics/sdl/nearest2x.c"
    "src/libs/graphics/sdl/opengl.c"
    "src/libs/graphics/sdl/opengl.h"
    "src/libs/graphics/sdl/palette.c"
    "src/libs/graphics/sdl/palette.h"
    "src/libs/graphics/sdl/png2sdl.c"
    "src/libs/graphics/sdl/png2sdl.h"
    "src/libs/graphics/sdl/primitives.c"
    "src/libs/graphics/sdl/primitives.h"
    "src/libs/graphics/sdl/pure.c"
    "src/libs/graphics/sdl/pure.h"
    "src/libs/graphics/sdl/rotozoom.c"
    "src/libs/graphics/sdl/rotozoom.h"
    "src/libs/graphics/sdl/scaleint.h"
    "src/libs/graphics/sdl/scalemmx.h"
    "src/libs/graphics/sdl/scalers.c"
    "src/libs/graphics/sdl/scalers.h"
    "src/libs/graphics/sdl/sdl1_common.c"
    "src/libs/graphics/sdl/sdl2_common.c"
    "src/libs/graphics/sdl/sdl2_pure.c"
    "src/libs/graphics/sdl/sdl_common.c"
    "src/libs/graphics/sdl/sdl_common.h"
    "src/libs/graphics/sdl/sdluio.c"
    "src/libs/graphics/sdl/sdluio.h"
    "src/libs/graphics/sdl/triscan2x.c"
)
source_group("Source Files\\libs\\graphics\\sdl" FILES ${Source_Files__libs__graphics__sdl})

set(Source_Files__libs__heap
    "src/libs/heap/heap.c"
    "src/libs/heap/heap.h"
)
source_group("Source Files\\libs\\heap" FILES ${Source_Files__libs__heap})

set(Source_Files__libs__input
    "src/libs/input/inpintrn.h"
    "src/libs/input/input_common.c"
    "src/libs/input/input_common.h"
)
source_group("Source Files\\libs\\input" FILES ${Source_Files__libs__input})

set(Source_Files__libs__input__sdl_No._1
    "src/libs/input/sdl/input.c"
    "src/libs/input/sdl/input.h"
    "src/libs/input/sdl/keynames.c"
    "src/libs/input/sdl/keynames.h"
    "src/libs/input/sdl/vcontrol.c"
    "src/libs/input/sdl/vcontrol.h"
)
source_group("Source Files\\libs\\input\\sdl No. 1" FILES ${Source_Files__libs__input__sdl_No._1})

set(Source_Files__libs__list
    "src/libs/list/list.c"
    "src/libs/list/list.h"
)
source_group("Source Files\\libs\\list" FILES ${Source_Files__libs__list})

set(Source_Files__libs__log
    "src/libs/log/loginternal.h"
    "src/libs/log/msgbox.h"
    "src/libs/log/msgbox_win.c"
    "src/libs/log/uqmlog.c"
    "src/libs/log/uqmlog.h"
)
source_group("Source Files\\libs\\log" FILES ${Source_Files__libs__log})

set(Source_Files__libs__lua
    "src/libs/lua/lapi.c"
    "src/libs/lua/lapi.h"
    "src/libs/lua/lauxlib.c"
    "src/libs/lua/lauxlib.h"
    "src/libs/lua/lbaselib.c"
    "src/libs/lua/lbitlib.c"
    "src/libs/lua/lcode.c"
    "src/libs/lua/lcode.h"
    "src/libs/lua/lctype.c"
    "src/libs/lua/lctype.h"
    "src/libs/lua/ldebug.c"
    "src/libs/lua/ldebug.h"
    "src/libs/lua/ldo.c"
    "src/libs/lua/ldo.h"
    "src/libs/lua/ldump.c"
    "src/libs/lua/lfunc.c"
    "src/libs/lua/lfunc.h"
    "src/libs/lua/lgc.c"
    "src/libs/lua/lgc.h"
    "src/libs/lua/llex.c"
    "src/libs/lua/llex.h"
    "src/libs/lua/llimits.h"
    "src/libs/lua/lmathlib.c"
    "src/libs/lua/lmem.c"
    "src/libs/lua/lmem.h"
    "src/libs/lua/lobject.c"
    "src/libs/lua/lobject.h"
    "src/libs/lua/lopcodes.c"
    "src/libs/lua/lopcodes.h"
    "src/libs/lua/lparser.c"
    "src/libs/lua/lparser.h"
    "src/libs/lua/lstate.c"
    "src/libs/lua/lstate.h"
    "src/libs/lua/lstring.c"
    "src/libs/lua/lstring.h"
    "src/libs/lua/lstrlib.c"
    "src/libs/lua/ltable.c"
    "src/libs/lua/ltable.h"
    "src/libs/lua/ltablib.c"
    "src/libs/lua/ltm.c"
    "src/libs/lua/ltm.h"
    "src/libs/lua/lua.h"
    "src/libs/lua/luaconf.h"
    "src/libs/lua/lualib.h"
    "src/libs/lua/lundump.c"
    "src/libs/lua/lundump.h"
    "src/libs/lua/lvm.c"
    "src/libs/lua/lvm.h"
    "src/libs/lua/lzio.c"
    "src/libs/lua/lzio.h"
)
source_group("Source Files\\libs\\lua" FILES ${Source_Files__libs__lua})

set(Source_Files__libs__luauqm
    "src/libs/luauqm/luauqm.c"
    "src/libs/luauqm/luauqm.h"
    "src/libs/luauqm/scriptres.c"
    "src/libs/luauqm/scriptres.h"
)
source_group("Source Files\\libs\\luauqm" FILES ${Source_Files__libs__luauqm})

set(Source_Files__libs__math
    "src/libs/math/mthintrn.h"
    "src/libs/math/random.c"
    "src/libs/math/random.h"
    "src/libs/math/random2.c"
    "src/libs/math/sqrt.c"
)
source_group("Source Files\\libs\\math" FILES ${Source_Files__libs__math})

set(Source_Files__libs__md5
    "src/libs/md5/md5.c"
    "src/libs/md5/md5.h"
)
source_group("Source Files\\libs\\md5" FILES ${Source_Files__libs__md5})

set(Source_Files__libs__memory
    "src/libs/memory/w_memlib.c"
)
source_group("Source Files\\libs\\memory" FILES ${Source_Files__libs__memory})

set(Source_Files__libs__mikmod
    "src/libs/mikmod/drv_nos.c"
    "src/libs/mikmod/load_it.c"
    "src/libs/mikmod/load_mod.c"
    "src/libs/mikmod/load_s3m.c"
    "src/libs/mikmod/load_stm.c"
    "src/libs/mikmod/load_xm.c"
    "src/libs/mikmod/mdreg.c"
    "src/libs/mikmod/mdriver.c"
    "src/libs/mikmod/mikmod.h"
    "src/libs/mikmod/mikmod_build.h"
    "src/libs/mikmod/mikmod_internals.h"
    "src/libs/mikmod/mloader.c"
    "src/libs/mikmod/mlreg.c"
    "src/libs/mikmod/mlutil.c"
    "src/libs/mikmod/mmalloc.c"
    "src/libs/mikmod/mmerror.c"
    "src/libs/mikmod/mmio.c"
    "src/libs/mikmod/mplayer.c"
    "src/libs/mikmod/munitrk.c"
    "src/libs/mikmod/mwav.c"
    "src/libs/mikmod/npertab.c"
    "src/libs/mikmod/sloader.c"
    "src/libs/mikmod/virtch.c"
    "src/libs/mikmod/virtch2.c"
    "src/libs/mikmod/virtch_common.c"
)
source_group("Source Files\\libs\\mikmod" FILES ${Source_Files__libs__mikmod})

set(Source_Files__libs__network
    "src/libs/network/bytesex.h"
    "src/libs/network/netport.c"
    "src/libs/network/netport.h"
    "src/libs/network/network.h"
    "src/libs/network/network_win.c"
    "src/libs/network/wspiapiwrap.c"
    "src/libs/network/wspiapiwrap.h"
)
source_group("Source Files\\libs\\network" FILES ${Source_Files__libs__network})

set(Source_Files__libs__network__connect
    "src/libs/network/connect/connect.c"
    "src/libs/network/connect/connect.h"
    "src/libs/network/connect/listen.c"
    "src/libs/network/connect/listen.h"
    "src/libs/network/connect/resolve.c"
    "src/libs/network/connect/resolve.h"
)
source_group("Source Files\\libs\\network\\connect" FILES ${Source_Files__libs__network__connect})

set(Source_Files__libs__network__netmanager
    "src/libs/network/netmanager/ndesc.c"
    "src/libs/network/netmanager/ndesc.h"
    "src/libs/network/netmanager/ndindex.ci"
    "src/libs/network/netmanager/netmanager.h"
    "src/libs/network/netmanager/netmanager_common.ci"
    "src/libs/network/netmanager/netmanager_win.c"
    "src/libs/network/netmanager/netmanager_win.h"
)
source_group("Source Files\\libs\\network\\netmanager" FILES ${Source_Files__libs__network__netmanager})

set(Source_Files__libs__network__socket
    "src/libs/network/socket/socket.c"
    "src/libs/network/socket/socket.h"
    "src/libs/network/socket/socket_win.c"
    "src/libs/network/socket/socket_win.h"
)
source_group("Source Files\\libs\\network\\socket" FILES ${Source_Files__libs__network__socket})

set(Source_Files__libs__resource
    "src/libs/resource/direct.c"
    "src/libs/resource/filecntl.c"
    "src/libs/resource/getres.c"
    "src/libs/resource/index.h"
    "src/libs/resource/loadres.c"
    "src/libs/resource/propfile.c"
    "src/libs/resource/propfile.h"
    "src/libs/resource/resinit.c"
    "src/libs/resource/resintrn.h"
    "src/libs/resource/stringbank.c"
    "src/libs/resource/stringbank.h"
)
source_group("Source Files\\libs\\resource" FILES ${Source_Files__libs__resource})

set(Source_Files__libs__sound
    "src/libs/sound/audiocore.c"
    "src/libs/sound/audiocore.h"
    "src/libs/sound/fileinst.c"
    "src/libs/sound/music.c"
    "src/libs/sound/resinst.c"
    "src/libs/sound/sfx.c"
    "src/libs/sound/sndintrn.h"
    "src/libs/sound/sound.c"
    "src/libs/sound/sound.h"
    "src/libs/sound/stream.c"
    "src/libs/sound/stream.h"
    "src/libs/sound/trackint.h"
    "src/libs/sound/trackplayer.c"
    "src/libs/sound/trackplayer.h"
)
source_group("Source Files\\libs\\sound" FILES ${Source_Files__libs__sound})

set(Source_Files__libs__sound__decoders
    "src/libs/sound/decoders/aiffaud.c"
    "src/libs/sound/decoders/aiffaud.h"
    "src/libs/sound/decoders/decoder.c"
    "src/libs/sound/decoders/decoder.h"
    "src/libs/sound/decoders/dukaud.c"
    "src/libs/sound/decoders/dukaud.h"
    "src/libs/sound/decoders/modaud.c"
    "src/libs/sound/decoders/modaud.h"
    "src/libs/sound/decoders/oggaud.c"
    "src/libs/sound/decoders/oggaud.h"
    "src/libs/sound/decoders/wav.c"
    "src/libs/sound/decoders/wav.h"
)
source_group("Source Files\\libs\\sound\\decoders" FILES ${Source_Files__libs__sound__decoders})

set(Source_Files__libs__sound__mixer
    "src/libs/sound/mixer/mixer.c"
    "src/libs/sound/mixer/mixer.h"
    "src/libs/sound/mixer/mixerint.h"
)
source_group("Source Files\\libs\\sound\\mixer" FILES ${Source_Files__libs__sound__mixer})

set(Source_Files__libs__sound__mixer__mixsdl
    "src/libs/sound/mixer/sdl/audiodrv_sdl.c"
    "src/libs/sound/mixer/sdl/audiodrv_sdl.h"
)
source_group("Source Files\\libs\\sound\\mixer\\mixsdl" FILES ${Source_Files__libs__sound__mixer__mixsdl})

set(Source_Files__libs__sound__mixer__nosound
    "src/libs/sound/mixer/nosound/audiodrv_nosound.c"
    "src/libs/sound/mixer/nosound/audiodrv_nosound.h"
)
source_group("Source Files\\libs\\sound\\mixer\\nosound" FILES ${Source_Files__libs__sound__mixer__nosound})

set(Source_Files__libs__sound__openal
    "src/libs/sound/openal/audiodrv_openal.c"
    "src/libs/sound/openal/audiodrv_openal.h"
)
source_group("Source Files\\libs\\sound\\openal" FILES ${Source_Files__libs__sound__openal})

set(Source_Files__libs__strings
    "src/libs/strings/getstr.c"
    "src/libs/strings/sfileins.c"
    "src/libs/strings/sresins.c"
    "src/libs/strings/stringhashtable.c"
    "src/libs/strings/stringhashtable.h"
    "src/libs/strings/strings.c"
    "src/libs/strings/strintrn.h"
    "src/libs/strings/unicode.c"
)
source_group("Source Files\\libs\\strings" FILES ${Source_Files__libs__strings})

set(Source_Files__libs__task
    "src/libs/task/tasklib.c"
)
source_group("Source Files\\libs\\task" FILES ${Source_Files__libs__task})

set(Source_Files__libs__threads
    "src/libs/threads/thrcommon.c"
    "src/libs/threads/thrcommon.h"
)
source_group("Source Files\\libs\\threads" FILES ${Source_Files__libs__threads})

set(Source_Files__libs__threads__sdl_No._3
    "src/libs/threads/sdl/sdlthreads.c"
    "src/libs/threads/sdl/sdlthreads.h"
)
source_group("Source Files\\libs\\threads\\sdl No. 3" FILES ${Source_Files__libs__threads__sdl_No._3})

set(Source_Files__libs__time
    "src/libs/time/timecommon.c"
    "src/libs/time/timecommon.h"
)
source_group("Source Files\\libs\\time" FILES ${Source_Files__libs__time})

set(Source_Files__libs__time__sdl_No._4
    "src/libs/time/sdl/sdltime.c"
    "src/libs/time/sdl/sdltime.h"
)
source_group("Source Files\\libs\\time\\sdl No. 4" FILES ${Source_Files__libs__time__sdl_No._4})

set(Source_Files__libs__uio
    "src/libs/uio/charhashtable.c"
    "src/libs/uio/charhashtable.h"
#    "src/libs/uio/debug.c"
    "src/libs/uio/debug.h"
    "src/libs/uio/defaultfs.c"
    "src/libs/uio/defaultfs.h"
    "src/libs/uio/fileblock.c"
    "src/libs/uio/fileblock.h"
    "src/libs/uio/fstypes.c"
    "src/libs/uio/fstypes.h"
    "src/libs/uio/getint.h"
    "src/libs/uio/gphys.c"
    "src/libs/uio/gphys.h"
#    "src/libs/uio/hashtable.c"
    "src/libs/uio/hashtable.h"
    "src/libs/uio/io.c"
    "src/libs/uio/io.h"
    "src/libs/uio/ioaux.c"
    "src/libs/uio/ioaux.h"
    "src/libs/uio/iointrn.h"
    "src/libs/uio/match.c"
    "src/libs/uio/match.h"
    "src/libs/uio/mem.h"
#    "src/libs/uio/memdebug.c"
    "src/libs/uio/memdebug.h"
    "src/libs/uio/mount.c"
    "src/libs/uio/mount.h"
    "src/libs/uio/mounttree.c"
    "src/libs/uio/mounttree.h"
    "src/libs/uio/paths.c"
    "src/libs/uio/paths.h"
    "src/libs/uio/physical.c"
    "src/libs/uio/physical.h"
    "src/libs/uio/types.h"
    "src/libs/uio/uioport.h"
    "src/libs/uio/uiostream.c"
    "src/libs/uio/uiostream.h"
    "src/libs/uio/uioutils.c"
    "src/libs/uio/uioutils.h"
    "src/libs/uio/utils.c"
    "src/libs/uio/utils.h"
)
source_group("Source Files\\libs\\uio" FILES ${Source_Files__libs__uio})

set(Source_Files__libs__uio__stdio
    "src/libs/uio/stdio/stdio.c"
    "src/libs/uio/stdio/stdio.h"
)
source_group("Source Files\\libs\\uio\\stdio" FILES ${Source_Files__libs__uio__stdio})

set(Source_Files__libs__uio__zip
    "src/libs/uio/zip/zip.c"
    "src/libs/uio/zip/zip.h"
)
source_group("Source Files\\libs\\uio\\zip" FILES ${Source_Files__libs__uio__zip})

set(Source_Files__libs__video
    "src/libs/video/dukvid.c"
    "src/libs/video/dukvid.h"
    "src/libs/video/legacyplayer.c"
    "src/libs/video/vfileins.c"
    "src/libs/video/video.c"
    "src/libs/video/video.h"
    "src/libs/video/videodec.c"
    "src/libs/video/videodec.h"
    "src/libs/video/vidintrn.h"
    "src/libs/video/vidplayer.c"
    "src/libs/video/vidplayer.h"
    "src/libs/video/vresins.c"
)
source_group("Source Files\\libs\\video" FILES ${Source_Files__libs__video})

set(Source_Files__uqm
    "src/uqm/battle.c"
    "src/uqm/battle.h"
    "src/uqm/battlecontrols.c"
    "src/uqm/battlecontrols.h"
    "src/uqm/border.c"
    "src/uqm/build.c"
    "src/uqm/build.h"
    "src/uqm/cleanup.c"
    "src/uqm/clock.c"
    "src/uqm/clock.h"
    "src/uqm/cnctdlg.c"
    "src/uqm/cnctdlg.h"
    "src/uqm/coderes.h"
    "src/uqm/collide.c"
    "src/uqm/collide.h"
    "src/uqm/colors.h"
    "src/uqm/comm.c"
    "src/uqm/comm.h"
    "src/uqm/commanim.c"
    "src/uqm/commanim.h"
    "src/uqm/commglue.c"
    "src/uqm/commglue.h"
    "src/uqm/confirm.c"
    "src/uqm/cons_res.c"
    "src/uqm/cons_res.h"
    "src/uqm/controls.h"
    "src/uqm/corecode.h"
    "src/uqm/credits.c"
    "src/uqm/credits.h"
    "src/uqm/cyborg.c"
    "src/uqm/demo.h"
    "src/uqm/displist.c"
    "src/uqm/displist.h"
    "src/uqm/dummy.c"
    "src/uqm/dummy.h"
    "src/uqm/element.h"
    "src/uqm/encount.c"
    "src/uqm/encount.h"
    "src/uqm/flash.c"
    "src/uqm/flash.h"
    "src/uqm/fmv.c"
    "src/uqm/fmv.h"
    "src/uqm/galaxy.c"
    "src/uqm/gameev.c"
    "src/uqm/gameev.h"
    "src/uqm/gameinp.c"
    "src/uqm/gameopt.c"
    "src/uqm/gameopt.h"
    "src/uqm/gamestr.h"
    "src/uqm/gendef.c"
    "src/uqm/gendef.h"
    "src/uqm/getchar.c"
    "src/uqm/globdata.c"
    "src/uqm/globdata.h"
    "src/uqm/gravity.c"
    "src/uqm/grpinfo.c"
    "src/uqm/grpinfo.h"
    "src/uqm/grpintrn.h"
    "src/uqm/hyper.c"
    "src/uqm/hyper.h"
    "src/uqm/ifontres.h"
    "src/uqm/igfxres.h"
    "src/uqm/ikey_con.h"
    "src/uqm/imusicre.h"
    "src/uqm/init.c"
    "src/uqm/init.h"
    "src/uqm/intel.c"
    "src/uqm/intel.h"
    "src/uqm/intro.c"
    "src/uqm/ipdisp.c"
    "src/uqm/ipdisp.h"
    "src/uqm/isndres.h"
    "src/uqm/istrtab.h"
    "src/uqm/load.c"
    "src/uqm/load_legacy.c"
    "src/uqm/loadship.c"
    "src/uqm/master.c"
    "src/uqm/master.h"
    "src/uqm/menu.c"
    "src/uqm/menustat.h"
    "src/uqm/misc.c"
    "src/uqm/nameref.h"
    "src/uqm/oscill.c"
    "src/uqm/oscill.h"
    "src/uqm/outfit.c"
    "src/uqm/pickship.c"
    "src/uqm/pickship.h"
    "src/uqm/process.c"
    "src/uqm/process.h"
    "src/uqm/races.h"
    "src/uqm/resinst.h"
    "src/uqm/restart.c"
    "src/uqm/restart.h"
    "src/uqm/save.c"
    "src/uqm/save.h"
    "src/uqm/settings.c"
    "src/uqm/settings.h"
    "src/uqm/setup.c"
    "src/uqm/setup.h"
    "src/uqm/setupmenu.c"
    "src/uqm/setupmenu.h"
    "src/uqm/ship.c"
    "src/uqm/ship.h"
    "src/uqm/shipcont.h"
    "src/uqm/shipstat.c"
    "src/uqm/shipyard.c"
    "src/uqm/sis.c"
    "src/uqm/sis.h"
    "src/uqm/sounds.c"
    "src/uqm/sounds.h"
    "src/uqm/starbase.c"
    "src/uqm/starbase.h"
    "src/uqm/starcon.c"
    "src/uqm/starcon.h"
    "src/uqm/starmap.c"
    "src/uqm/starmap.h"
    "src/uqm/StarmapGlobalData.h"
    "src/uqm/state.c"
    "src/uqm/state.h"
    "src/uqm/status.c"
    "src/uqm/status.h"
    "src/uqm/tactrans.c"
    "src/uqm/tactrans.h"
    "src/uqm/trans.c"
    "src/uqm/units.h"
    "src/uqm/uqmdebug.c"
    "src/uqm/uqmdebug.h"
    "src/uqm/util.c"
    "src/uqm/util.h"
    "src/uqm/velocity.c"
    "src/uqm/velocity.h"
    "src/uqm/weapon.c"
    "src/uqm/weapon.h"
)
source_group("Source Files\\uqm" FILES ${Source_Files__uqm})

set(Source_Files__uqm__comm
    "src/uqm/comm/commall.h"
)
source_group("Source Files\\uqm\\comm" FILES ${Source_Files__uqm__comm})

set(Source_Files__uqm__comm__arilou.comm
    "src/uqm/comm/arilou/arilouc.c"
    "src/uqm/comm/arilou/resinst.h"
    "src/uqm/comm/arilou/strings.h"
)
source_group("Source Files\\uqm\\comm\\arilou.comm" FILES ${Source_Files__uqm__comm__arilou.comm})

set(Source_Files__uqm__comm__blackur.comm
    "src/uqm/comm/blackur/blackurc.c"
    "src/uqm/comm/blackur/resinst.h"
    "src/uqm/comm/blackur/strings.h"
)
source_group("Source Files\\uqm\\comm\\blackur.comm" FILES ${Source_Files__uqm__comm__blackur.comm})

set(Source_Files__uqm__comm__chmmr.comm
    "src/uqm/comm/chmmr/chmmrc.c"
    "src/uqm/comm/chmmr/resinst.h"
    "src/uqm/comm/chmmr/strings.h"
)
source_group("Source Files\\uqm\\comm\\chmmr.comm" FILES ${Source_Files__uqm__comm__chmmr.comm})

set(Source_Files__uqm__comm__comandr.comm
    "src/uqm/comm/comandr/comandr.c"
    "src/uqm/comm/comandr/resinst.h"
    "src/uqm/comm/comandr/strings.h"
)
source_group("Source Files\\uqm\\comm\\comandr.comm" FILES ${Source_Files__uqm__comm__comandr.comm})

set(Source_Files__uqm__comm__druuge.comm
    "src/uqm/comm/druuge/druugec.c"
    "src/uqm/comm/druuge/resinst.h"
    "src/uqm/comm/druuge/strings.h"
)
source_group("Source Files\\uqm\\comm\\druuge.comm" FILES ${Source_Files__uqm__comm__druuge.comm})

set(Source_Files__uqm__comm__ilwrath.comm
    "src/uqm/comm/ilwrath/ilwrathc.c"
    "src/uqm/comm/ilwrath/resinst.h"
    "src/uqm/comm/ilwrath/strings.h"
)
source_group("Source Files\\uqm\\comm\\ilwrath.comm" FILES ${Source_Files__uqm__comm__ilwrath.comm})

set(Source_Files__uqm__comm__melnorm.comm
    "src/uqm/comm/melnorm/melnorm.c"
    "src/uqm/comm/melnorm/resinst.h"
    "src/uqm/comm/melnorm/strings.h"
)
source_group("Source Files\\uqm\\comm\\melnorm.comm" FILES ${Source_Files__uqm__comm__melnorm.comm})

set(Source_Files__uqm__comm__mycon.comm
    "src/uqm/comm/mycon/myconc.c"
    "src/uqm/comm/mycon/resinst.h"
    "src/uqm/comm/mycon/strings.h"
)
source_group("Source Files\\uqm\\comm\\mycon.comm" FILES ${Source_Files__uqm__comm__mycon.comm})

set(Source_Files__uqm__comm__orz.comm
    "src/uqm/comm/orz/orzc.c"
    "src/uqm/comm/orz/resinst.h"
    "src/uqm/comm/orz/strings.h"
)
source_group("Source Files\\uqm\\comm\\orz.comm" FILES ${Source_Files__uqm__comm__orz.comm})

set(Source_Files__uqm__comm__pkunk.comm
    "src/uqm/comm/pkunk/pkunkc.c"
    "src/uqm/comm/pkunk/resinst.h"
    "src/uqm/comm/pkunk/strings.h"
)
source_group("Source Files\\uqm\\comm\\pkunk.comm" FILES ${Source_Files__uqm__comm__pkunk.comm})

set(Source_Files__uqm__comm__rebel.comm
    "src/uqm/comm/rebel/rebel.c"
    "src/uqm/comm/rebel/strings.h"
)
source_group("Source Files\\uqm\\comm\\rebel.comm" FILES ${Source_Files__uqm__comm__rebel.comm})

set(Source_Files__uqm__comm__shofixt.comm
    "src/uqm/comm/shofixt/resinst.h"
    "src/uqm/comm/shofixt/shofixt.c"
    "src/uqm/comm/shofixt/strings.h"
)
source_group("Source Files\\uqm\\comm\\shofixt.comm" FILES ${Source_Files__uqm__comm__shofixt.comm})

set(Source_Files__uqm__comm__slyhome.comm
    "src/uqm/comm/slyhome/resinst.h"
    "src/uqm/comm/slyhome/slyhome.c"
    "src/uqm/comm/slyhome/strings.h"
)
source_group("Source Files\\uqm\\comm\\slyhome.comm" FILES ${Source_Files__uqm__comm__slyhome.comm})

set(Source_Files__uqm__comm__slyland.comm
    "src/uqm/comm/slyland/resinst.h"
    "src/uqm/comm/slyland/slyland.c"
    "src/uqm/comm/slyland/strings.h"
)
source_group("Source Files\\uqm\\comm\\slyland.comm" FILES ${Source_Files__uqm__comm__slyland.comm})

set(Source_Files__uqm__comm__spahome.comm
    "src/uqm/comm/spahome/spahome.c"
    "src/uqm/comm/spahome/strings.h"
)
source_group("Source Files\\uqm\\comm\\spahome.comm" FILES ${Source_Files__uqm__comm__spahome.comm})

set(Source_Files__uqm__comm__spathi.comm
    "src/uqm/comm/spathi/resinst.h"
    "src/uqm/comm/spathi/spathic.c"
    "src/uqm/comm/spathi/strings.h"
)
source_group("Source Files\\uqm\\comm\\spathi.comm" FILES ${Source_Files__uqm__comm__spathi.comm})

set(Source_Files__uqm__comm__starbas.comm
    "src/uqm/comm/starbas/starbas.c"
    "src/uqm/comm/starbas/strings.h"
)
source_group("Source Files\\uqm\\comm\\starbas.comm" FILES ${Source_Files__uqm__comm__starbas.comm})

set(Source_Files__uqm__comm__supox.comm
    "src/uqm/comm/supox/resinst.h"
    "src/uqm/comm/supox/strings.h"
    "src/uqm/comm/supox/supoxc.c"
)
source_group("Source Files\\uqm\\comm\\supox.comm" FILES ${Source_Files__uqm__comm__supox.comm})

set(Source_Files__uqm__comm__syreen.comm
    "src/uqm/comm/syreen/resinst.h"
    "src/uqm/comm/syreen/strings.h"
    "src/uqm/comm/syreen/syreenc.c"
)
source_group("Source Files\\uqm\\comm\\syreen.comm" FILES ${Source_Files__uqm__comm__syreen.comm})

set(Source_Files__uqm__comm__talkpet.comm
    "src/uqm/comm/talkpet/resinst.h"
    "src/uqm/comm/talkpet/strings.h"
    "src/uqm/comm/talkpet/talkpet.c"
)
source_group("Source Files\\uqm\\comm\\talkpet.comm" FILES ${Source_Files__uqm__comm__talkpet.comm})

set(Source_Files__uqm__comm__thradd.comm
    "src/uqm/comm/thradd/resinst.h"
    "src/uqm/comm/thradd/strings.h"
    "src/uqm/comm/thradd/thraddc.c"
)
source_group("Source Files\\uqm\\comm\\thradd.comm" FILES ${Source_Files__uqm__comm__thradd.comm})

set(Source_Files__uqm__comm__umgah.comm
    "src/uqm/comm/umgah/resinst.h"
    "src/uqm/comm/umgah/strings.h"
    "src/uqm/comm/umgah/umgahc.c"
)
source_group("Source Files\\uqm\\comm\\umgah.comm" FILES ${Source_Files__uqm__comm__umgah.comm})

set(Source_Files__uqm__comm__urquan.comm
    "src/uqm/comm/urquan/resinst.h"
    "src/uqm/comm/urquan/strings.h"
    "src/uqm/comm/urquan/urquanc.c"
)
source_group("Source Files\\uqm\\comm\\urquan.comm" FILES ${Source_Files__uqm__comm__urquan.comm})

set(Source_Files__uqm__comm__utwig.comm
    "src/uqm/comm/utwig/resinst.h"
    "src/uqm/comm/utwig/strings.h"
    "src/uqm/comm/utwig/utwigc.c"
)
source_group("Source Files\\uqm\\comm\\utwig.comm" FILES ${Source_Files__uqm__comm__utwig.comm})

set(Source_Files__uqm__comm__vux.comm
    "src/uqm/comm/vux/resinst.h"
    "src/uqm/comm/vux/strings.h"
    "src/uqm/comm/vux/vuxc.c"
)
source_group("Source Files\\uqm\\comm\\vux.comm" FILES ${Source_Files__uqm__comm__vux.comm})

set(Source_Files__uqm__comm__yehat.comm
    "src/uqm/comm/yehat/resinst.h"
    "src/uqm/comm/yehat/strings.h"
    "src/uqm/comm/yehat/yehatc.c"
)
source_group("Source Files\\uqm\\comm\\yehat.comm" FILES ${Source_Files__uqm__comm__yehat.comm})

set(Source_Files__uqm__comm__zoqfot.comm
    "src/uqm/comm/zoqfot/resinst.h"
    "src/uqm/comm/zoqfot/strings.h"
    "src/uqm/comm/zoqfot/zoqfotc.c"
)
source_group("Source Files\\uqm\\comm\\zoqfot.comm" FILES ${Source_Files__uqm__comm__zoqfot.comm})

set(Source_Files__uqm__lua_28uqm29
    "src/uqm/lua/luacomm.c"
    "src/uqm/lua/luacomm.h"
    "src/uqm/lua/luadebug.c"
    "src/uqm/lua/luadebug.h"
    "src/uqm/lua/luaevent.c"
    "src/uqm/lua/luaevent.h"
    "src/uqm/lua/luainit.c"
    "src/uqm/lua/luainit.h"
    "src/uqm/lua/luastate.c"
    "src/uqm/lua/luastate.h"
)
source_group("Source Files\\uqm\\lua %28uqm%29" FILES ${Source_Files__uqm__lua_28uqm29})

set(Source_Files__uqm__lua_28uqm29__luafuncs
    "src/uqm/lua/luafuncs/commfuncs.c"
    "src/uqm/lua/luafuncs/commfuncs.h"
    "src/uqm/lua/luafuncs/customfuncs.c"
    "src/uqm/lua/luafuncs/customfuncs.h"
    "src/uqm/lua/luafuncs/eventfuncs.c"
    "src/uqm/lua/luafuncs/eventfuncs.h"
    "src/uqm/lua/luafuncs/logfuncs.c"
    "src/uqm/lua/luafuncs/logfuncs.h"
    "src/uqm/lua/luafuncs/statefuncs.c"
    "src/uqm/lua/luafuncs/statefuncs.h"
)
source_group("Source Files\\uqm\\lua %28uqm%29\\luafuncs" FILES ${Source_Files__uqm__lua_28uqm29__luafuncs})

set(Source_Files__uqm__planets
    "src/uqm/planets/calc.c"
    "src/uqm/planets/cargo.c"
    "src/uqm/planets/devices.c"
    "src/uqm/planets/elemdata.h"
    "src/uqm/planets/generate.h"
    "src/uqm/planets/gentopo.c"
    "src/uqm/planets/lander.c"
    "src/uqm/planets/lander.h"
    "src/uqm/planets/lifeform.h"
    "src/uqm/planets/orbits.c"
    "src/uqm/planets/oval.c"
    "src/uqm/planets/pl_stuff.c"
    "src/uqm/planets/plandata.h"
    "src/uqm/planets/planets.c"
    "src/uqm/planets/planets.h"
    "src/uqm/planets/plangen.c"
    "src/uqm/planets/pstarmap.c"
    "src/uqm/planets/report.c"
    "src/uqm/planets/roster.c"
    "src/uqm/planets/scan.c"
    "src/uqm/planets/scan.h"
    "src/uqm/planets/solarsys.c"
    "src/uqm/planets/solarsys.h"
    "src/uqm/planets/sundata.h"
    "src/uqm/planets/surface.c"
)
source_group("Source Files\\uqm\\planets" FILES ${Source_Files__uqm__planets})

set(Source_Files__uqm__planets__generate
    "src/uqm/planets/generate/genall.h"
    "src/uqm/planets/generate/genand.c"
    "src/uqm/planets/generate/genburv.c"
    "src/uqm/planets/generate/genchmmr.c"
    "src/uqm/planets/generate/gencol.c"
    "src/uqm/planets/generate/gendefault.c"
    "src/uqm/planets/generate/gendefault.h"
    "src/uqm/planets/generate/gendru.c"
    "src/uqm/planets/generate/genilw.c"
    "src/uqm/planets/generate/genmel.c"
    "src/uqm/planets/generate/genmyc.c"
    "src/uqm/planets/generate/genorz.c"
    "src/uqm/planets/generate/genpet.c"
    "src/uqm/planets/generate/genpku.c"
    "src/uqm/planets/generate/genrain.c"
    "src/uqm/planets/generate/gensam.c"
    "src/uqm/planets/generate/genshof.c"
    "src/uqm/planets/generate/gensly.c"
    "src/uqm/planets/generate/gensol.c"
    "src/uqm/planets/generate/genspa.c"
    "src/uqm/planets/generate/gensup.c"
    "src/uqm/planets/generate/gensyr.c"
    "src/uqm/planets/generate/genthrad.c"
    "src/uqm/planets/generate/gentrap.c"
    "src/uqm/planets/generate/genutw.c"
    "src/uqm/planets/generate/genvault.c"
    "src/uqm/planets/generate/genvux.c"
    "src/uqm/planets/generate/genwreck.c"
    "src/uqm/planets/generate/genyeh.c"
    "src/uqm/planets/generate/genzfpscout.c"
    "src/uqm/planets/generate/genzoq.c"
)
source_group("Source Files\\uqm\\planets\\generate" FILES ${Source_Files__uqm__planets__generate})

set(Source_Files__uqm__ships
    "src/uqm/ships/ship.h"
)
source_group("Source Files\\uqm\\ships" FILES ${Source_Files__uqm__ships})

set(Source_Files__uqm__ships__androsyn
    "src/uqm/ships/androsyn/androsyn.c"
    "src/uqm/ships/androsyn/androsyn.h"
    "src/uqm/ships/androsyn/icode.h"
    "src/uqm/ships/androsyn/resinst.h"
)
source_group("Source Files\\uqm\\ships\\androsyn" FILES ${Source_Files__uqm__ships__androsyn})

set(Source_Files__uqm__ships__arilou
    "src/uqm/ships/arilou/arilou.c"
    "src/uqm/ships/arilou/arilou.h"
    "src/uqm/ships/arilou/icode.h"
    "src/uqm/ships/arilou/resinst.h"
)
source_group("Source Files\\uqm\\ships\\arilou" FILES ${Source_Files__uqm__ships__arilou})

set(Source_Files__uqm__ships__blackurq
    "src/uqm/ships/blackurq/blackurq.c"
    "src/uqm/ships/blackurq/blackurq.h"
    "src/uqm/ships/blackurq/icode.h"
    "src/uqm/ships/blackurq/resinst.h"
)
source_group("Source Files\\uqm\\ships\\blackurq" FILES ${Source_Files__uqm__ships__blackurq})

set(Source_Files__uqm__ships__chenjesu
    "src/uqm/ships/chenjesu/chenjesu.c"
    "src/uqm/ships/chenjesu/chenjesu.h"
    "src/uqm/ships/chenjesu/icode.h"
    "src/uqm/ships/chenjesu/resinst.h"
)
source_group("Source Files\\uqm\\ships\\chenjesu" FILES ${Source_Files__uqm__ships__chenjesu})

set(Source_Files__uqm__ships__chmmr
    "src/uqm/ships/chmmr/chmmr.c"
    "src/uqm/ships/chmmr/chmmr.h"
    "src/uqm/ships/chmmr/icode.h"
    "src/uqm/ships/chmmr/resinst.h"
)
source_group("Source Files\\uqm\\ships\\chmmr" FILES ${Source_Files__uqm__ships__chmmr})

set(Source_Files__uqm__ships__druuge
    "src/uqm/ships/druuge/druuge.c"
    "src/uqm/ships/druuge/druuge.h"
    "src/uqm/ships/druuge/icode.h"
    "src/uqm/ships/druuge/resinst.h"
)
source_group("Source Files\\uqm\\ships\\druuge" FILES ${Source_Files__uqm__ships__druuge})

set(Source_Files__uqm__ships__human
    "src/uqm/ships/human/human.c"
    "src/uqm/ships/human/human.h"
    "src/uqm/ships/human/icode.h"
    "src/uqm/ships/human/resinst.h"
)
source_group("Source Files\\uqm\\ships\\human" FILES ${Source_Files__uqm__ships__human})

set(Source_Files__uqm__ships__ilwrath
    "src/uqm/ships/ilwrath/icode.h"
    "src/uqm/ships/ilwrath/ilwrath.c"
    "src/uqm/ships/ilwrath/ilwrath.h"
    "src/uqm/ships/ilwrath/resinst.h"
)
source_group("Source Files\\uqm\\ships\\ilwrath" FILES ${Source_Files__uqm__ships__ilwrath})

set(Source_Files__uqm__ships__lastbat
    "src/uqm/ships/lastbat/icode.h"
    "src/uqm/ships/lastbat/lastbat.c"
    "src/uqm/ships/lastbat/lastbat.h"
    "src/uqm/ships/lastbat/resinst.h"
)
source_group("Source Files\\uqm\\ships\\lastbat" FILES ${Source_Files__uqm__ships__lastbat})

set(Source_Files__uqm__ships__melnorme
    "src/uqm/ships/melnorme/icode.h"
    "src/uqm/ships/melnorme/melnorme.c"
    "src/uqm/ships/melnorme/melnorme.h"
    "src/uqm/ships/melnorme/resinst.h"
)
source_group("Source Files\\uqm\\ships\\melnorme" FILES ${Source_Files__uqm__ships__melnorme})

set(Source_Files__uqm__ships__mmrnmhrm
    "src/uqm/ships/mmrnmhrm/icode.h"
    "src/uqm/ships/mmrnmhrm/mmrnmhrm.c"
    "src/uqm/ships/mmrnmhrm/mmrnmhrm.h"
    "src/uqm/ships/mmrnmhrm/resinst.h"
)
source_group("Source Files\\uqm\\ships\\mmrnmhrm" FILES ${Source_Files__uqm__ships__mmrnmhrm})

set(Source_Files__uqm__ships__mycon
    "src/uqm/ships/mycon/icode.h"
    "src/uqm/ships/mycon/mycon.c"
    "src/uqm/ships/mycon/mycon.h"
    "src/uqm/ships/mycon/resinst.h"
)
source_group("Source Files\\uqm\\ships\\mycon" FILES ${Source_Files__uqm__ships__mycon})

set(Source_Files__uqm__ships__orz
    "src/uqm/ships/orz/icode.h"
    "src/uqm/ships/orz/orz.c"
    "src/uqm/ships/orz/orz.h"
    "src/uqm/ships/orz/resinst.h"
)
source_group("Source Files\\uqm\\ships\\orz" FILES ${Source_Files__uqm__ships__orz})

set(Source_Files__uqm__ships__pkunk
    "src/uqm/ships/pkunk/icode.h"
    "src/uqm/ships/pkunk/pkunk.c"
    "src/uqm/ships/pkunk/pkunk.h"
    "src/uqm/ships/pkunk/resinst.h"
)
source_group("Source Files\\uqm\\ships\\pkunk" FILES ${Source_Files__uqm__ships__pkunk})

set(Source_Files__uqm__ships__probe
    "src/uqm/ships/probe/icode.h"
    "src/uqm/ships/probe/probe.c"
    "src/uqm/ships/probe/probe.h"
    "src/uqm/ships/probe/resinst.h"
)
source_group("Source Files\\uqm\\ships\\probe" FILES ${Source_Files__uqm__ships__probe})

set(Source_Files__uqm__ships__shofixti
    "src/uqm/ships/shofixti/icode.h"
    "src/uqm/ships/shofixti/resinst.h"
    "src/uqm/ships/shofixti/shofixti.c"
    "src/uqm/ships/shofixti/shofixti.h"
)
source_group("Source Files\\uqm\\ships\\shofixti" FILES ${Source_Files__uqm__ships__shofixti})

set(Source_Files__uqm__ships__sis_ship
    "src/uqm/ships/sis_ship/icode.h"
    "src/uqm/ships/sis_ship/resinst.h"
    "src/uqm/ships/sis_ship/sis_ship.c"
    "src/uqm/ships/sis_ship/sis_ship.h"
)
source_group("Source Files\\uqm\\ships\\sis_ship" FILES ${Source_Files__uqm__ships__sis_ship})

set(Source_Files__uqm__ships__slylandr
    "src/uqm/ships/slylandr/icode.h"
    "src/uqm/ships/slylandr/resinst.h"
    "src/uqm/ships/slylandr/slylandr.c"
    "src/uqm/ships/slylandr/slylandr.h"
)
source_group("Source Files\\uqm\\ships\\slylandr" FILES ${Source_Files__uqm__ships__slylandr})

set(Source_Files__uqm__ships__spathi
    "src/uqm/ships/spathi/icode.h"
    "src/uqm/ships/spathi/resinst.h"
    "src/uqm/ships/spathi/spathi.c"
    "src/uqm/ships/spathi/spathi.h"
)
source_group("Source Files\\uqm\\ships\\spathi" FILES ${Source_Files__uqm__ships__spathi})

set(Source_Files__uqm__ships__supox
    "src/uqm/ships/supox/icode.h"
    "src/uqm/ships/supox/resinst.h"
    "src/uqm/ships/supox/supox.c"
    "src/uqm/ships/supox/supox.h"
)
source_group("Source Files\\uqm\\ships\\supox" FILES ${Source_Files__uqm__ships__supox})

set(Source_Files__uqm__ships__syreen
    "src/uqm/ships/syreen/icode.h"
    "src/uqm/ships/syreen/resinst.h"
    "src/uqm/ships/syreen/syreen.c"
    "src/uqm/ships/syreen/syreen.h"
)
source_group("Source Files\\uqm\\ships\\syreen" FILES ${Source_Files__uqm__ships__syreen})

set(Source_Files__uqm__ships__thradd
    "src/uqm/ships/thradd/icode.h"
    "src/uqm/ships/thradd/resinst.h"
    "src/uqm/ships/thradd/thradd.c"
    "src/uqm/ships/thradd/thradd.h"
)
source_group("Source Files\\uqm\\ships\\thradd" FILES ${Source_Files__uqm__ships__thradd})

set(Source_Files__uqm__ships__umgah
    "src/uqm/ships/umgah/icode.h"
    "src/uqm/ships/umgah/resinst.h"
    "src/uqm/ships/umgah/umgah.c"
    "src/uqm/ships/umgah/umgah.h"
)
source_group("Source Files\\uqm\\ships\\umgah" FILES ${Source_Files__uqm__ships__umgah})

set(Source_Files__uqm__ships__urquan
    "src/uqm/ships/urquan/icode.h"
    "src/uqm/ships/urquan/resinst.h"
    "src/uqm/ships/urquan/urquan.c"
    "src/uqm/ships/urquan/urquan.h"
)
source_group("Source Files\\uqm\\ships\\urquan" FILES ${Source_Files__uqm__ships__urquan})

set(Source_Files__uqm__ships__utwig
    "src/uqm/ships/utwig/icode.h"
    "src/uqm/ships/utwig/resinst.h"
    "src/uqm/ships/utwig/utwig.c"
    "src/uqm/ships/utwig/utwig.h"
)
source_group("Source Files\\uqm\\ships\\utwig" FILES ${Source_Files__uqm__ships__utwig})

set(Source_Files__uqm__ships__vux
    "src/uqm/ships/vux/icode.h"
    "src/uqm/ships/vux/resinst.h"
    "src/uqm/ships/vux/vux.c"
    "src/uqm/ships/vux/vux.h"
)
source_group("Source Files\\uqm\\ships\\vux" FILES ${Source_Files__uqm__ships__vux})

set(Source_Files__uqm__ships__yehat
    "src/uqm/ships/yehat/icode.h"
    "src/uqm/ships/yehat/resinst.h"
    "src/uqm/ships/yehat/yehat.c"
    "src/uqm/ships/yehat/yehat.h"
)
source_group("Source Files\\uqm\\ships\\yehat" FILES ${Source_Files__uqm__ships__yehat})

set(Source_Files__uqm__ships__zoqfot
    "src/uqm/ships/zoqfot/icode.h"
    "src/uqm/ships/zoqfot/resinst.h"
    "src/uqm/ships/zoqfot/zoqfot.c"
    "src/uqm/ships/zoqfot/zoqfot.h"
)
source_group("Source Files\\uqm\\ships\\zoqfot" FILES ${Source_Files__uqm__ships__zoqfot})

set(Source_Files__uqm__supermelee
    "src/uqm/supermelee/buildpick.c"
    "src/uqm/supermelee/buildpick.h"
    "src/uqm/supermelee/loadmele.c"
    "src/uqm/supermelee/loadmele.h"
    "src/uqm/supermelee/melee.c"
    "src/uqm/supermelee/melee.h"
    "src/uqm/supermelee/meleesetup.c"
    "src/uqm/supermelee/meleesetup.h"
    "src/uqm/supermelee/meleeship.h"
    "src/uqm/supermelee/pickmele.c"
    "src/uqm/supermelee/pickmele.h"
)
source_group("Source Files\\uqm\\supermelee" FILES ${Source_Files__uqm__supermelee})

set(Source_Files__uqm__supermelee__netplay
    "src/uqm/supermelee/netplay/checkbuf.c"
    "src/uqm/supermelee/netplay/checkbuf.h"
    "src/uqm/supermelee/netplay/checksum.c"
    "src/uqm/supermelee/netplay/checksum.h"
    "src/uqm/supermelee/netplay/crc.c"
    "src/uqm/supermelee/netplay/crc.h"
    "src/uqm/supermelee/netplay/nc_connect.ci"
    "src/uqm/supermelee/netplay/netconnection.c"
    "src/uqm/supermelee/netplay/netconnection.h"
    "src/uqm/supermelee/netplay/netinput.c"
    "src/uqm/supermelee/netplay/netinput.h"
    "src/uqm/supermelee/netplay/netmelee.c"
    "src/uqm/supermelee/netplay/netmelee.h"
    "src/uqm/supermelee/netplay/netmisc.c"
    "src/uqm/supermelee/netplay/netmisc.h"
    "src/uqm/supermelee/netplay/netoptions.c"
    "src/uqm/supermelee/netplay/netoptions.h"
    "src/uqm/supermelee/netplay/netplay.h"
    "src/uqm/supermelee/netplay/netrcv.c"
    "src/uqm/supermelee/netplay/netrcv.h"
    "src/uqm/supermelee/netplay/netsend.c"
    "src/uqm/supermelee/netplay/netsend.h"
    "src/uqm/supermelee/netplay/netstate.c"
    "src/uqm/supermelee/netplay/netstate.h"
    "src/uqm/supermelee/netplay/notify.c"
    "src/uqm/supermelee/netplay/notify.h"
    "src/uqm/supermelee/netplay/notifyall.c"
    "src/uqm/supermelee/netplay/notifyall.h"
    "src/uqm/supermelee/netplay/packet.c"
    "src/uqm/supermelee/netplay/packet.h"
    "src/uqm/supermelee/netplay/packethandlers.c"
    "src/uqm/supermelee/netplay/packethandlers.h"
    "src/uqm/supermelee/netplay/packetq.c"
    "src/uqm/supermelee/netplay/packetq.h"
    "src/uqm/supermelee/netplay/packetsenders.c"
    "src/uqm/supermelee/netplay/packetsenders.h"
)
source_group("Source Files\\uqm\\supermelee\\netplay" FILES ${Source_Files__uqm__supermelee__netplay})

set(Source_Files__uqm__supermelee__netplay__proto
    "src/uqm/supermelee/netplay/proto/npconfirm.c"
    "src/uqm/supermelee/netplay/proto/npconfirm.h"
    "src/uqm/supermelee/netplay/proto/ready.c"
    "src/uqm/supermelee/netplay/proto/ready.h"
    "src/uqm/supermelee/netplay/proto/reset.c"
    "src/uqm/supermelee/netplay/proto/reset.h"
)
source_group("Source Files\\uqm\\supermelee\\netplay\\proto" FILES ${Source_Files__uqm__supermelee__netplay__proto})

set(ALL_FILES
    ${Doc}
    ${Doc__Devel}
    ${Doc__Users}
    ${Resources}
    ${Source_Files}
    ${Source_Files___core__memory}
    ${Source_Files__getopt}
    ${Source_Files__libs}
    ${Source_Files__libs__callback}
    ${Source_Files__libs__decomp}
    ${Source_Files__libs__file}
    ${Source_Files__libs__graphics}
    ${Source_Files__libs__graphics__sdl}
    ${Source_Files__libs__heap}
    ${Source_Files__libs__input}
    ${Source_Files__libs__input__sdl_No._1}
    ${Source_Files__libs__list}
    ${Source_Files__libs__log}
    ${Source_Files__libs__lua}
    ${Source_Files__libs__luauqm}
    ${Source_Files__libs__math}
    ${Source_Files__libs__md5}
    ${Source_Files__libs__memory}
    ${Source_Files__libs__mikmod}
    ${Source_Files__libs__network}
    ${Source_Files__libs__network__connect}
    ${Source_Files__libs__network__netmanager}
    ${Source_Files__libs__network__socket}
    ${Source_Files__libs__resource}
    ${Source_Files__libs__sound}
    ${Source_Files__libs__sound__decoders}
    ${Source_Files__libs__sound__mixer}
    ${Source_Files__libs__sound__mixer__mixsdl}
    ${Source_Files__libs__sound__mixer__nosound}
    ${Source_Files__libs__sound__openal}
    ${Source_Files__libs__strings}
    ${Source_Files__libs__task}
    ${Source_Files__libs__threads}
    ${Source_Files__libs__threads__sdl_No._3}
    ${Source_Files__libs__time}
    ${Source_Files__libs__time__sdl_No._4}
    ${Source_Files__libs__uio}
    ${Source_Files__libs__uio__stdio}
    ${Source_Files__libs__uio__zip}
    ${Source_Files__libs__video}
    ${Source_Files__uqm}
    ${Source_Files__uqm__comm}
    ${Source_Files__uqm__comm__arilou.comm}
    ${Source_Files__uqm__comm__blackur.comm}
    ${Source_Files__uqm__comm__chmmr.comm}
    ${Source_Files__uqm__comm__comandr.comm}
    ${Source_Files__uqm__comm__druuge.comm}
    ${Source_Files__uqm__comm__ilwrath.comm}
    ${Source_Files__uqm__comm__melnorm.comm}
    ${Source_Files__uqm__comm__mycon.comm}
    ${Source_Files__uqm__comm__orz.comm}
    ${Source_Files__uqm__comm__pkunk.comm}
    ${Source_Files__uqm__comm__rebel.comm}
    ${Source_Files__uqm__comm__shofixt.comm}
    ${Source_Files__uqm__comm__slyhome.comm}
    ${Source_Files__uqm__comm__slyland.comm}
    ${Source_Files__uqm__comm__spahome.comm}
    ${Source_Files__uqm__comm__spathi.comm}
    ${Source_Files__uqm__comm__starbas.comm}
    ${Source_Files__uqm__comm__supox.comm}
    ${Source_Files__uqm__comm__syreen.comm}
    ${Source_Files__uqm__comm__talkpet.comm}
    ${Source_Files__uqm__comm__thradd.comm}
    ${Source_Files__uqm__comm__umgah.comm}
    ${Source_Files__uqm__comm__urquan.comm}
    ${Source_Files__uqm__comm__utwig.comm}
    ${Source_Files__uqm__comm__vux.comm}
    ${Source_Files__uqm__comm__yehat.comm}
    ${Source_Files__uqm__comm__zoqfot.comm}
    ${Source_Files__uqm__lua_28uqm29}
    ${Source_Files__uqm__lua_28uqm29__luafuncs}
    ${Source_Files__uqm__planets}
    ${Source_Files__uqm__planets__generate}
    ${Source_Files__uqm__ships}
    ${Source_Files__uqm__ships__androsyn}
    ${Source_Files__uqm__ships__arilou}
    ${Source_Files__uqm__ships__blackurq}
    ${Source_Files__uqm__ships__chenjesu}
    ${Source_Files__uqm__ships__chmmr}
    ${Source_Files__uqm__ships__druuge}
    ${Source_Files__uqm__ships__human}
    ${Source_Files__uqm__ships__ilwrath}
    ${Source_Files__uqm__ships__lastbat}
    ${Source_Files__uqm__ships__melnorme}
    ${Source_Files__uqm__ships__mmrnmhrm}
    ${Source_Files__uqm__ships__mycon}
    ${Source_Files__uqm__ships__orz}
    ${Source_Files__uqm__ships__pkunk}
    ${Source_Files__uqm__ships__probe}
    ${Source_Files__uqm__ships__shofixti}
    ${Source_Files__uqm__ships__sis_ship}
    ${Source_Files__uqm__ships__slylandr}
    ${Source_Files__uqm__ships__spathi}
    ${Source_Files__uqm__ships__supox}
    ${Source_Files__uqm__ships__syreen}
    ${Source_Files__uqm__ships__thradd}
    ${Source_Files__uqm__ships__umgah}
    ${Source_Files__uqm__ships__urquan}
    ${Source_Files__uqm__ships__utwig}
    ${Source_Files__uqm__ships__vux}
    ${Source_Files__uqm__ships__yehat}
    ${Source_Files__uqm__ships__zoqfot}
    ${Source_Files__uqm__supermelee}
    ${Source_Files__uqm__supermelee__netplay}
    ${Source_Files__uqm__supermelee__netplay__proto}
)

################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME} ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
################################################################################
# Target name
################################################################################
set_target_properties(${PROJECT_NAME} PROPERTIES
    TARGET_NAME_DEBUG       "${OUTPUT_DIRECTORY}$<TARGET_FILE_NAME:${PROJECT_NAME}>"
    TARGET_NAME_RELEASE     "${OUTPUT_DIRECTORY}$<TARGET_FILE_NAME:${PROJECT_NAME}>"
)
################################################################################
# Output directory
################################################################################
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_DIRECTORY_DEBUG       "${CMAKE_CURRENT_SOURCE_DIR}/"
    OUTPUT_DIRECTORY_RELEASE     "${CMAKE_CURRENT_SOURCE_DIR}/"
)
################################################################################
# Linker Language
################################################################################
set_target_properties(${PROJECT_NAME} PROPERTIES
    LINKER_LANGUAGE CXX
    LINKER_LANGUAGE CXX
)
################################################################################
# MSVC runtime library
################################################################################
get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)
string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
    $<$<CONFIG:Debug>:
        MultiThreadedDebugDLL
    >
    $<$<CONFIG:Release>:
        MultiThreadedDLL
    >
    $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
)
set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

################################################################################
# Include directories
################################################################################
target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/regex"
)

################################################################################
# Compile definitions
################################################################################
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "$<$<CONFIG:Debug>:"
        "DEBUG;"
        "_DEBUG;"
        "DEBUG_TRACK_SEM;"
        "DEBUG_DCQ_THREADS"
    ">"
    "$<$<CONFIG:Release>:"
        "NDEBUG"
    ">"
    "WIN32;"
    "_CONSOLE;"
    "_VW=320;"
    "_VH=240;"
    "GFXMODULE_SDL;"
    "THREADLIB_SDL;"
    "HAVE_OPENAL;"
    "HAVE_ZIP;"
    "HAVE_JOYSTICK;"
    "NETPLAY;"
    "ZLIB_DLL;"
    "USE_INTERNAL_MIKMOD;"
    "USE_INTERNAL_LUA;"
    "USE_PLATFORM_ACCEL;"
    "_CRT_SECURE_NO_WARNINGS;"
    "_MBCS"
)

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /Od;
            /RTC1;
            /Gy-;
            /W3;
            /fp:except-;
            /GR
        >
        $<$<CONFIG:Release>:
            /O2;
            /Oi;
            /GF;
            /Gy;
            /W2
        >
        /MP;
        /sdl-;
        ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
        /TP;
        ${DEFAULT_CXX_EXCEPTION_HANDLING}
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /NODEFAULTLIB:msvcrt.lib;
            /DEBUG;
            /SUBSYSTEM:CONSOLE;
            /NXCOMPAT:NO;
            /DYNAMICBASE:NO;
            /INCREMENTAL
        >
        $<$<CONFIG:Release>:
            /NODEFAULTLIB:msvcrtd.lib;
            /DEBUG;
            /SUBSYSTEM:WINDOWS;
            /INCREMENTAL:NO
        >
        /NOLOGO
    )
endif()

################################################################################
# Post build events
################################################################################
add_custom_command_if(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMANDS
    COMMAND       $<CONFIG:Debug> xcopy /Y "$<SHELL_PATH:$<TARGET_FILE:${PROJECT_NAME}>>" "$ENV{LocalDebuggerWorkingDirectory}"
    COMMAND     $<CONFIG:Release> xcopy /Y "$<SHELL_PATH:$<TARGET_FILE:${PROJECT_NAME}>>" "$ENV{LocalDebuggerWorkingDirectory}"
    COMMAND     $<CONFIG:Release> editbin /REBASE:BASE=0x400000 /DYNAMICBASE:NO "$ENV{LocalDebuggerWorkingDirectory}$<TARGET_FILE_NAME:${PROJECT_NAME}>"
    COMMENT "Stripping debug info..."
)

################################################################################
# Dependencies
################################################################################
set(ADDITIONAL_LIBRARY_DEPENDENCIES
    "$<$<CONFIG:Debug>:"
        "SDL2;"
        "SDL2main"
    ">"
    "$<$<CONFIG:Release>:"
        "SDL2;"
        "SDL2main;"
        "user32"
    ">"
    "libpng16;"
    "zdll;"
    "ws2_32"
)
target_link_libraries(${PROJECT_NAME} PRIVATE "${ADDITIONAL_LIBRARY_DEPENDENCIES}")

