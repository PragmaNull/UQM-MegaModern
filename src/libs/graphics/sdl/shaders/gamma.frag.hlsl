// Applies gamma correction to the colors of the texture.

cbuffer Context : register(b0, space3)
{
	float inverseGamma;
};

Texture2D u_texture : register(t0, space2);
SamplerState u_sampler : register(s0, space2);

// The semantics match the outputs of the vertex shader of the SDL GPU renderer.
struct PSInput
{
	float4 v_color : TEXCOORD0;
	float2 v_uv : TEXCOORD1;
};

float4 main(PSInput input) : SV_Target
{
	const float4 color = u_texture.Sample(u_sampler, input.v_uv) * input.v_color;
	return float4(pow(color.rgb, inverseGamma), color.a);
}
