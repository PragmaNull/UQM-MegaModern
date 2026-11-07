// Owners for SDL handles.
#pragma once
#ifndef UQM_LIBS_GRAPHICS_SDL_SDL_PTR_H
#define UQM_LIBS_GRAPHICS_SDL_SDL_PTR_H

#include <SDL3/SDL.h>

#include "core/stl/memory.h"

namespace uqm
{

// Calls 'Free' on the handle.
template <auto Free>
struct SdlFree
{
	template <typename T>
	void operator()(T* handle) const
	{
		Free(handle);
	}
};

using SurfacePtr = uqstl::unique_ptr<SDL_Surface, SdlFree<&SDL_DestroySurface>>;

} // namespace uqm

#endif // UQM_LIBS_GRAPHICS_SDL_SDL_PTR_H
