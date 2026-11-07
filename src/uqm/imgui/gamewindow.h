// ImGui window that shows and changes the game state: flags, clock, flagship and SIS.

#pragma once
#ifndef UQM_IMGUI_GAMEWINDOW_H
#define UQM_IMGUI_GAMEWINDOW_H

namespace uqm
{

// Adds the Game window to the ImGui overlay. Call on the main thread.
void addGameWindow();

} // namespace uqm

#endif // UQM_IMGUI_GAMEWINDOW_H
