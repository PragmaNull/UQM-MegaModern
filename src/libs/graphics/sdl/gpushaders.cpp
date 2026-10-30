// GPU shaders for gamma correction and the screen scalers, for the SDL GPU renderer.

#include "gpushaders.h"
#include "core/log/log.h"
#include "shaders/biadapt.frag.h"
#include "shaders/biadaptadv.frag.h"
#include "shaders/gamma.frag.h"
#include "shaders/hq2x.frag.h"
#include "shaders/hq2x_rules.h"
#include "shaders/triscan.frag.h"

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
	if (!shaders->load(shaders->m_gamma, {gamma_frag_dxil, gamma_frag_spv, gamma_frag_msl}, 1)
		|| !shaders->load(shaders->m_biadapt, {biadapt_frag_dxil, biadapt_frag_spv, biadapt_frag_msl}, 0)
		|| !shaders->load(shaders->m_biadaptAdv, {biadaptadv_frag_dxil, biadaptadv_frag_spv, biadaptadv_frag_msl}, 0)
		|| !shaders->load(shaders->m_triscan, {triscan_frag_dxil, triscan_frag_spv, triscan_frag_msl}, 0)
		|| !shaders->load(shaders->m_hq2x, {hq2x_frag_dxil, hq2x_frag_spv, hq2x_frag_msl}, 1))
	{
		return nullptr;
	}
	if (!SDL_SetGPURenderStateFragmentUniforms(shaders->m_hq2x.state, 0, hq2x_rules, sizeof(hq2x_rules)))
	{
		log::warn("Could not set the hq2x rules: {}", SDL_GetError());
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
	release(m_biadapt);
	release(m_biadaptAdv);
	release(m_triscan);
	release(m_hq2x);
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

SDL_GPURenderState* GpuShaders::scaler(GfxFlags flags) const
{
	if (testFlag(flags, GfxFlags::Scale_BiAdapt))
	{
		return m_biadapt.state;
	}
	if (testFlag(flags, GfxFlags::Scale_BiAdaptAdv))
	{
		return m_biadaptAdv.state;
	}
	if (testFlag(flags, GfxFlags::Scale_Triscan))
	{
		return m_triscan.state;
	}
	if (testFlag(flags, GfxFlags::Scale_HQXX))
	{
		return m_hq2x.state;
	}
	return nullptr;
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
