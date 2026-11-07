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
using IoStreamPtr = uqstl::unique_ptr<SDL_IOStream, SdlFree<&SDL_CloseIO>>;

// Memory that SDL allocated and SDL_free releases.
template <typename T>
using SdlArrayPtr = uqstl::unique_ptr<T[], SdlFree<&SDL_free>>;

// Locks the surface from construction to destruction.
class [[nodiscard]] SurfaceLock
{
public:
	explicit SurfaceLock(SDL_Surface* surface)
		: m_surface {surface}
	{
		SDL_LockSurface(m_surface);
	}

	~SurfaceLock()
	{
		SDL_UnlockSurface(m_surface);
	}

	SurfaceLock(const SurfaceLock&) = delete;
	SurfaceLock& operator=(const SurfaceLock&) = delete;

private:
	SDL_Surface* m_surface;
};

} // namespace uqm

#endif // UQM_LIBS_GRAPHICS_SDL_SDL_PTR_H
