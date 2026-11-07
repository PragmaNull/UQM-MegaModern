// Dear ImGui overlay that draws on top of the game screen with the SDL renderer.

#pragma once
#ifndef UQM_LIBS_GRAPHICS_IMGUI_IMGUILAYER_H
#define UQM_LIBS_GRAPHICS_IMGUI_IMGUILAYER_H

#include <SDL3/SDL.h>
#include "core/stl/memory.h"

namespace uqm
{

class ImGuiLayer
{
public:
	// Returns nullptr if ImGui cannot start with the window and the renderer.
	[[nodiscard]] static uqstl::unique_ptr<ImGuiLayer> create(SDL_Window* window, SDL_Renderer* renderer);

	~ImGuiLayer();
	ImGuiLayer(const ImGuiLayer&) = delete;
	ImGuiLayer& operator=(const ImGuiLayer&) = delete;

	// Gives the event to ImGui. Returns true if the game must not get the event.
	[[nodiscard]] bool processEvent(const SDL_Event& event);

	// True if the overlay shows, and the screen must update each frame.
	[[nodiscard]] bool visible() const;

	// Draws the overlay to the current render target. The target must be the window.
	void render();

private:
	explicit ImGuiLayer(SDL_Renderer* renderer);
	void drawWindows();

	SDL_Renderer* m_renderer;
	bool m_visible {};
	bool m_showDemo {};
};

} // namespace uqm

#endif // UQM_LIBS_GRAPHICS_IMGUI_IMGUILAYER_H
