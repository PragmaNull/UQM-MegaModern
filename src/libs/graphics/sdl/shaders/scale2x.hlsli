// Shared code of the shaders that scale a texture to a target of twice its size.

Texture2D u_texture : register(t0, space2);
SamplerState u_sampler : register(s0, space2);

// The semantics match the outputs of the vertex shader of the SDL GPU renderer.
struct PSInput
{
	float4 v_color : TEXCOORD0;
	float2 v_uv : TEXCOORD1;
};

// The source pixel of an output pixel, and the place of the output pixel in the 2x2 block of that source pixel.
struct Scale2x
{
	int2 pos;
	int2 size;
	int2 quad;
};

Scale2x scale2xInput(PSInput input)
{
	uint width;
	uint height;
	u_texture.GetDimensions(width, height);
	Scale2x s;
	s.size = int2(width, height);
	const int2 outPos = int2(floor(input.v_uv * float2(s.size * 2)));
	s.pos = outPos >> 1;
	s.quad = outPos & 1;
	return s;
}

// The 8-bit RGB value of the source pixel at offset d. An offset past the edge reads the edge pixel.
// The texture must use nearest sampling.
int3 pixel(Scale2x s, int2 d)
{
	const int2 p = clamp(s.pos + d, int2(0, 0), s.size - 1);
	const float2 uv = (float2(p) + 0.5) / float2(s.size);
	return int3(round(u_texture.SampleLevel(u_sampler, uv, 0).rgb * 255.0));
}

bool same(int3 a, int3 b)
{
	return all(a == b);
}

float4 toColor(float3 rgb)
{
	return float4(rgb / 255.0, 1.0);
}

float4 color(int3 c)
{
	return toColor(float3(c));
}

// The blends drop the low bits of each color first, as the CPU scalers do.
float4 blend11(int3 a, int3 b)
{
	return color((a >> 1) + (b >> 1));
}

float4 blend1111(int3 a, int3 b, int3 c, int3 d)
{
	return color((a >> 2) + (b >> 2) + (c >> 2) + (d >> 2));
}

// Rows of the YUV matrix of the CPU scalers, scaled by 2^14, with Y doubled.
static const int3 YUV_Y = int3(9794, 19224, 3749);
static const int3 YUV_U = int3(-2764, -5428, 8192);
static const int3 YUV_V = int3(8192, -6853, -1339);

int sumShifted(int3 m, int3 c)
{
	const int3 t = (m * c) >> 14;
	return t.x + t.y + t.z;
}

// The Y value that the CPU scalers use to compare brightness.
int lumaY(int3 c)
{
	return sumShifted(YUV_Y, c);
}

// True if the YUV distance of the colors is at most the tolerance, as Scale_CmpYUV.
bool cmpYUV(int3 a, int3 b, int tolerance)
{
	const int3 d = a - b;
	return abs(sumShifted(YUV_Y, d)) + abs(sumShifted(YUV_U, d)) + abs(sumShifted(YUV_V, d)) <= tolerance;
}

// The squared RGB distance of the colors, as Scale_GetRGBDelta.
int rgbDelta(int3 a, int3 b)
{
	const int3 d = a - b;
	return dot(d, d);
}
