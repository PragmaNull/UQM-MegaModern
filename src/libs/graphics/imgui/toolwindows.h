// Windows that the ImGui overlay lists in its menu bar.

#pragma once
#ifndef UQM_LIBS_GRAPHICS_IMGUI_TOOLWINDOWS_H
#define UQM_LIBS_GRAPHICS_IMGUI_TOOLWINDOWS_H

#include "core/stl/functional.h"
#include "core/stl/stl.h"
#include "core/stl/string.h"

namespace uqm
{

struct ToolWindow
{
	uqstl::string name {};
	// Draws the window with ImGui. Set 'open' to false to close the window.
	uqstl::function<void(bool& open)> draw {};
	bool open {};
};

// Use these functions on the main thread only.
void addToolWindow(uqstl::string name, uqstl::function<void(bool& open)> draw, bool open = false);
[[nodiscard]] uqstl::span<ToolWindow> toolWindows();

} // namespace uqm

#endif // UQM_LIBS_GRAPHICS_IMGUI_TOOLWINDOWS_H
