// Runs work from the ImGui windows on the game thread.
#pragma once
#ifndef UQM_IMGUI_GAMETHREAD_H_
#define UQM_IMGUI_GAMETHREAD_H_

#include "core/stl/functional.h"

namespace uqm
{

// Runs the task on the game thread the next time that it processes callbacks.
void runOnGameThread(uqstl::function<void()> task);

} // namespace uqm

#endif // UQM_IMGUI_GAMETHREAD_H_
