############################################################


set(uqm_src_libs_master_files "")

############################################################
# Sub-directories
include("src/libs/callback/callback.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_callback_master_files})
# include("src/libs/cdp/cdp.cmake")
# list(APPEND uqm_src_libs_master_files ${uqm_src_libs_cdp_master_files})
include("src/libs/decomp/decomp.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_decomp_master_files})
include("src/libs/file/file.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_file_master_files})
include("src/libs/graphics/graphics.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_graphics_master_files})
include("src/libs/heap/heap.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_heap_master_files})
include("src/libs/input/input.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_input_master_files})
include("src/libs/io/io.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_io_master_files})
include("src/libs/list/list.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_list_master_files})
include("src/libs/luauqm/luauqm.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_luauqm_master_files})
include("src/libs/math/math.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_math_master_files})
include("src/libs/md5/md5.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_md5_master_files})
include("src/libs/memory/memory.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_memory_master_files})
include("src/libs/network/network.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_network_master_files})
include("src/libs/resource/resource.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_resource_master_files})
include("src/libs/sound/sound.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_sound_master_files})
include("src/libs/strings/strings.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_strings_master_files})
include("src/libs/task/task.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_task_master_files})
include("src/libs/threads/threads.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_threads_master_files})
include("src/libs/time/time.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_time_master_files})
include("src/libs/uio/uio.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_uio_master_files})
include("src/libs/video/video.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_video_master_files})

############################################################
# Local files
set(uqm_src_libs_files
	"src/libs/async.h"
#	"src/libs/cdplib.h"
	"src/libs/compiler.h"
	"src/libs/declib.h"
	"src/libs/file.h"
	"src/libs/gfxlib.h"
	"src/libs/heap.h"
	"src/libs/inplib.h"
	"src/libs/libs.cmake"
	"src/libs/list.h"
	"src/libs/mathlib.h"
	"src/libs/md5.h"
	"src/libs/memlib.h"
	"src/libs/misc.h"
	"src/libs/net.h"
	"src/libs/platform.h"
	"src/libs/reslib.h"
#	"src/libs/scriptlib.h"
	"src/libs/sndlib.h"
	"src/libs/strlib.h"
	"src/libs/tasklib.h"
	"src/libs/threadlib.h"
	"src/libs/uio.h"
#	"src/libs/uioutils.h"
	"src/libs/unicode.h"
	"src/libs/vidlib.h"
)

source_group("src\\libs" FILES ${uqm_src_libs_files})
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_files})
