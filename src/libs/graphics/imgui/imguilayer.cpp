// Dear ImGui overlay that draws on top of the game screen with the SDL renderer.

#include "imguilayer.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include "core/log/log.h"
#include "toolwindows.h"

namespace uqm
{

namespace
{

constexpr SDL_Keycode ToggleKey {SDLK_F12};

[[nodiscard]] bool isKeyboardEvent(const SDL_Event& event)
{
	switch (event.type)
	{
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_TEXT_INPUT:
		case SDL_EVENT_TEXT_EDITING:
			return true;
		default:
			return false;
	}
}

[[nodiscard]] bool isMouseEvent(const SDL_Event& event)
{
	switch (event.type)
	{
		case SDL_EVENT_MOUSE_MOTION:
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_WHEEL:
			return true;
		default:
			return false;
	}
}

} // namespace

uqstl::unique_ptr<ImGuiLayer> ImGuiLayer::create(SDL_Window* window, SDL_Renderer* renderer)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io {ImGui::GetIO()};
	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();

	if (!ImGui_ImplSDL3_InitForSDLRenderer(window, renderer))
	{
		log::warn("Could not start the ImGui platform backend.");
		ImGui::DestroyContext();
		return nullptr;
	}
	if (!ImGui_ImplSDLRenderer3_Init(renderer))
	{
		log::warn("Could not start the ImGui renderer backend.");
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();
		return nullptr;
	}
	log::info("ImGui {} ready.", ImGui::GetVersion());
	return uqstl::unique_ptr<ImGuiLayer> {new ImGuiLayer {renderer}};
}

ImGuiLayer::ImGuiLayer(SDL_Renderer* renderer)
	: m_renderer {renderer}
{
}

ImGuiLayer::~ImGuiLayer()
{
	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
}

bool ImGuiLayer::processEvent(const SDL_Event& event)
{
	if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == ToggleKey)
	{
		if (!event.key.repeat)
		{
			m_visible = !m_visible;
		}
		return true;
	}

	if (!m_visible)
	{
		return false;
	}
	ImGui_ImplSDL3_ProcessEvent(&event);
	const ImGuiIO& io {ImGui::GetIO()};
	return (io.WantCaptureKeyboard && isKeyboardEvent(event)) || (io.WantCaptureMouse && isMouseEvent(event));
}

void ImGuiLayer::drawWindows()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Windows"))
		{
			for (ToolWindow& window : toolWindows())
			{
				ImGui::MenuItem(window.name.c_str(), nullptr, &window.open);
			}
			ImGui::Separator();
			ImGui::MenuItem("ImGui Demo", nullptr, &m_showDemo);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	for (ToolWindow& window : toolWindows())
	{
		if (window.open)
		{
			window.draw(window.open);
		}
	}
	if (m_showDemo)
	{
		ImGui::ShowDemoWindow(&m_showDemo);
	}
}

bool ImGuiLayer::visible() const
{
	return m_visible;
}

void ImGuiLayer::render()
{
	if (!m_visible)
	{
		return;
	}

	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
	drawWindows();
	ImGui::Render();

	// ImGui draws in window pixels, so the letterbox of the game must be off.
	int width {};
	int height {};
	SDL_RendererLogicalPresentation mode {};
	SDL_GetRenderLogicalPresentation(m_renderer, &width, &height, &mode);
	SDL_SetRenderLogicalPresentation(m_renderer, width, height, SDL_LOGICAL_PRESENTATION_DISABLED);
	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_renderer);
	SDL_SetRenderLogicalPresentation(m_renderer, width, height, mode);
}

} // namespace uqm
