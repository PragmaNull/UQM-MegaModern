############################################################


set(uqm_src_libs_graphics_master_files "")

############################################################
# Sub-directories
include("src/libs/graphics/sdl/sdl.cmake")
list(APPEND uqm_src_libs_graphics_master_files ${uqm_src_libs_graphics_sdl_master_files})

############################################################
# Local files
set(uqm_src_libs_graphics_files
	"src/libs/graphics/bbox.cpp"
	"src/libs/graphics/bbox.h"
	"src/libs/graphics/boxint.cpp"
	"src/libs/graphics/clipline.cpp"
	"src/libs/graphics/cmap.cpp"
	"src/libs/graphics/cmap.h"
	"src/libs/graphics/context.cpp"
	"src/libs/graphics/context.h"
	"src/libs/graphics/dcqueue.cpp"
	"src/libs/graphics/dcqueue.h"
	"src/libs/graphics/drawable.cpp"
	"src/libs/graphics/drawable.h"
	"src/libs/graphics/drawcmd.h"
	"src/libs/graphics/filegfx.cpp"
	"src/libs/graphics/font.cpp"
	"src/libs/graphics/font.h"
	"src/libs/graphics/frame.cpp"
	"src/libs/graphics/gfx_common.cpp"
	"src/libs/graphics/gfx_common.h"
	"src/libs/graphics/gfx_defs.h"
	"src/libs/graphics/gfxintrn.h"
	"src/libs/graphics/gfxload.cpp"
	"src/libs/graphics/graphics.cmake"
	"src/libs/graphics/intersec.cpp"
	"src/libs/graphics/loaddisp.cpp"
	"src/libs/graphics/pixmap.cpp"
	"src/libs/graphics/prim.h"
	"src/libs/graphics/resgfx.cpp"
	"src/libs/graphics/tfb_draw.cpp"
	"src/libs/graphics/tfb_draw.h"
	"src/libs/graphics/tfb_prim.cpp"
	"src/libs/graphics/tfb_prim.h"
	"src/libs/graphics/widgets.cpp"
	"src/libs/graphics/widgets.h"
)

source_group("src\\libs\\graphics" FILES ${uqm_src_libs_graphics_files})
list(APPEND uqm_src_libs_graphics_master_files ${uqm_src_libs_graphics_files})
