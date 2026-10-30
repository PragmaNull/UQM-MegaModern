// GPU shaders for gamma correction and the screen scalers, for the SDL GPU renderer.

#include "gpushaders.h"
#include "core/log/log.h"
#include "shaders/gamma.frag.h"

namespace uqm
{

namespace
{

// Uniform data of the gamma shader. Uniform buffers are in blocks of 16 bytes.
struct GammaUniforms
{
	float inverseGamma;
	float padding[3];
};

} // namespace

uqstl::unique_ptr<GpuShaders> GpuShaders::create(SDL_Renderer* renderer)
{
	auto* const device {static_cast<SDL_GPUDevice*>(
		SDL_GetPointerProperty(SDL_GetRendererProperties(renderer), SDL_PROP_RENDERER_GPU_DEVICE_POINTER, nullptr))};
	if (!device)
	{
		return nullptr;
	}

	const SDL_GPUShaderFormat formats {SDL_GetGPUShaderFormats(device)};
	SDL_GPUShaderFormat format {SDL_GPU_SHADERFORMAT_INVALID};
	for (const SDL_GPUShaderFormat candidate : {SDL_GPU_SHADERFORMAT_SPIRV, SDL_GPU_SHADERFORMAT_DXIL, SDL_GPU_SHADERFORMAT_MSL})
	{
		if (formats & candidate)
		{
			format = candidate;
			break;
		}
	}
	if (format == SDL_GPU_SHADERFORMAT_INVALID)
	{
		log::warn("The GPU device takes no shader format that the game has.");
		return nullptr;
	}

	uqstl::unique_ptr<GpuShaders> shaders {
		new GpuShaders {renderer, device, format}
	 };
	if (!shaders->load(shaders->m_gamma, {gamma_frag_dxil, gamma_frag_spv, gamma_frag_msl}, 1))
	{
		return nullptr;
	}
	return shaders;
}

GpuShaders::GpuShaders(SDL_Renderer* renderer, SDL_GPUDevice* device, SDL_GPUShaderFormat format)
	: m_renderer {renderer}
	, m_device {device}
	, m_format {format}
{
}

GpuShaders::~GpuShaders()
{
	release(m_gamma);
}

const char* GpuShaders::formatName() const
{
	switch (m_format)
	{
		case SDL_GPU_SHADERFORMAT_SPIRV:
			return "SPIR-V";
		case SDL_GPU_SHADERFORMAT_DXIL:
			return "DXIL";
		case SDL_GPU_SHADERFORMAT_MSL:
			return "MSL";
		default:
			return "unknown";
	}
}

SDL_GPURenderState* GpuShaders::gamma(float gamma)
{
	if (gamma != m_gammaValue)
	{
		const GammaUniforms uniforms {1.0f / gamma, {}};
		if (!SDL_SetGPURenderStateFragmentUniforms(m_gamma.state, 0, &uniforms, sizeof(uniforms)))
		{
			log::warn("Could not set the gamma: {}", SDL_GetError());
		}
		m_gammaValue = gamma;
	}
	return m_gamma.state;
}

bool GpuShaders::load(Effect& effect, const ShaderCode& code, uint32_t numUniformBuffers)
{
	SDL_GPUShaderCreateInfo info {};
	uqstl::span<const uint8_t> bytes {};
	switch (m_format)
	{
		case SDL_GPU_SHADERFORMAT_SPIRV:
			bytes = code.spirv;
			info.entrypoint = "main";
			break;
		case SDL_GPU_SHADERFORMAT_DXIL:
			bytes = code.dxil;
			info.entrypoint = "main";
			break;
		default:
			bytes = code.msl;
			info.entrypoint = "main0";
			break;
	}
	info.code = bytes.data();
	info.code_size = bytes.size();
	info.format = m_format;
	info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	info.num_samplers = 1;
	info.num_uniform_buffers = numUniformBuffers;

	effect.shader = SDL_CreateGPUShader(m_device, &info);
	if (!effect.shader)
	{
		log::warn("Could not make a GPU shader: {}", SDL_GetError());
		return false;
	}

	SDL_GPURenderStateCreateInfo stateInfo {};
	stateInfo.fragment_shader = effect.shader;
	effect.state = SDL_CreateGPURenderState(m_renderer, &stateInfo);
	if (!effect.state)
	{
		log::warn("Could not make a GPU render state: {}", SDL_GetError());
		return false;
	}
	return true;
}

void GpuShaders::release(Effect& effect)
{
	if (effect.state)
	{
		SDL_DestroyGPURenderState(effect.state);
		effect.state = nullptr;
	}
	if (effect.shader)
	{
		SDL_ReleaseGPUShader(m_device, effect.shader);
		effect.shader = nullptr;
	}
}

} // namespace uqm
