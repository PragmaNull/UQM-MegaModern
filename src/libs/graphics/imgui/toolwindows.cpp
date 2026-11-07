// Windows that the ImGui overlay lists in its menu bar.

#include "toolwindows.h"

namespace uqm
{

namespace
{

uqstl::vector<ToolWindow>& windows()
{
	static uqstl::vector<ToolWindow> s_windows {};
	return s_windows;
}

} // namespace

void addToolWindow(uqstl::string name, uqstl::function<void(bool& open)> draw, bool open)
{
	windows().push_back({uqstl::move(name), uqstl::move(draw), open});
}

uqstl::span<ToolWindow> toolWindows()
{
	return windows();
}

} // namespace uqm
