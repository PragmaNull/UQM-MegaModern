############################################################


set(uqm_src_libs_graphics_sdl_master_files "")


############################################################
# Local files
set(uqm_src_libs_graphics_sdl_files
	"src/libs/graphics/sdl/2xscalers.cpp"
	"src/libs/graphics/sdl/2xscalers.h"
	"src/libs/graphics/sdl/biadv2x.cpp"
	"src/libs/graphics/sdl/bilinear2x.cpp"
	"src/libs/graphics/sdl/canvas.cpp"
	"src/libs/graphics/sdl/clipboard.cpp"
	"src/libs/graphics/sdl/gpushaders.cpp"
	"src/libs/graphics/sdl/gpushaders.h"
	"src/libs/graphics/sdl/hq2x.cpp"
	"src/libs/graphics/sdl/nearest2x.cpp"
	"src/libs/graphics/sdl/palette.cpp"
	"src/libs/graphics/sdl/palette.h"
	"src/libs/graphics/sdl/png2sdl.cpp"
	"src/libs/graphics/sdl/png2sdl.h"
	"src/libs/graphics/sdl/primitives.cpp"
	"src/libs/graphics/sdl/primitives.h"
	"src/libs/graphics/sdl/pure.h"
	"src/libs/graphics/sdl/rotozoom.cpp"
	"src/libs/graphics/sdl/rotozoom.h"
	"src/libs/graphics/sdl/scaleint.h"
	"src/libs/graphics/sdl/scalers.cpp"
	"src/libs/graphics/sdl/scalers.h"
	"src/libs/graphics/sdl/shaders/biadapt.frag.h"
	"src/libs/graphics/sdl/shaders/biadapt.frag.hlsl"
	"src/libs/graphics/sdl/shaders/biadaptadv.frag.h"
	"src/libs/graphics/sdl/shaders/biadaptadv.frag.hlsl"
	"src/libs/graphics/sdl/shaders/gamma.frag.h"
	"src/libs/graphics/sdl/shaders/gamma.frag.hlsl"
	"src/libs/graphics/sdl/shaders/hq2x.frag.h"
	"src/libs/graphics/sdl/shaders/hq2x.frag.hlsl"
	"src/libs/graphics/sdl/shaders/hq2x_rules.h"
	"src/libs/graphics/sdl/shaders/scale2x.hlsli"
	"src/libs/graphics/sdl/shaders/triscan.frag.h"
	"src/libs/graphics/sdl/shaders/triscan.frag.hlsl"
	"src/libs/graphics/sdl/sdl.cmake"
	"src/libs/graphics/sdl/sdl2_common.cpp"
	"src/libs/graphics/sdl/sdl2_pure.cpp"
	"src/libs/graphics/sdl/sdl_common.cpp"
	"src/libs/graphics/sdl/sdl_common.h"
	"src/libs/graphics/sdl/sdlimage.cpp"
	"src/libs/graphics/sdl/sdlimage.h"
	"src/libs/graphics/sdl/triscan2x.cpp"
)

source_group("src\\libs\\graphics\\sdl" FILES ${uqm_src_libs_graphics_sdl_files})
list(APPEND uqm_src_libs_graphics_sdl_master_files ${uqm_src_libs_graphics_sdl_files})
