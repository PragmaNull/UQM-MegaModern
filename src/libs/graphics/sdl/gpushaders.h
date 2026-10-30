// GPU shaders for gamma correction and the screen scalers, for the SDL GPU renderer.

#ifndef LIBS_GRAPHICS_SDL_GPUSHADERS_H_
#define LIBS_GRAPHICS_SDL_GPUSHADERS_H_

#include <SDL3/SDL.h>
#include <cstdint>
#include <span>
#include "core/stl/memory.h"

namespace uqm
{

// The compiled forms of one fragment shader.
struct ShaderCode
{
	uqstl::span<const uint8_t> dxil;
	uqstl::span<const uint8_t> spirv;
	uqstl::span<const uint8_t> msl;
};

class GpuShaders
{
public:
	// Returns nullptr if the renderer has no GPU device, or if the shaders cannot be made.
	[[nodiscard]] static uqstl::unique_ptr<GpuShaders> create(SDL_Renderer* renderer);

	~GpuShaders();
	GpuShaders(const GpuShaders&) = delete;
	GpuShaders& operator=(const GpuShaders&) = delete;

	// The name of the shader format in use.
	[[nodiscard]] const char* formatName() const;

	// Render state that applies gamma correction.
	[[nodiscard]] SDL_GPURenderState* gamma(float gamma);

private:
	// A fragment shader and the render state that uses it.
	struct Effect
	{
		SDL_GPUShader* shader {};
		SDL_GPURenderState* state {};
	};

	GpuShaders(SDL_Renderer* renderer, SDL_GPUDevice* device, SDL_GPUShaderFormat format);
	[[nodiscard]] bool load(Effect& effect, const ShaderCode& code, uint32_t numUniformBuffers);
	void release(Effect& effect);

	SDL_Renderer* m_renderer;
	SDL_GPUDevice* m_device;
	SDL_GPUShaderFormat m_format;
	Effect m_gamma {};
	float m_gammaValue {0.0f};
};

} // namespace uqm

#endif /* LIBS_GRAPHICS_SDL_GPUSHADERS_H_ */
