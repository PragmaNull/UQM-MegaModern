// The hq2x scaler, as Scale_HqFilter.

#include "scale2x.hlsli"

// hq2x_rules.h: the rules of the 4 output pixels for each neighbor pattern.
cbuffer Rules : register(b0, space3)
{
	uint4 rules[256];
};

// The YUV value of a color as the RGB15_to_YUV table of the CPU scalers: 5 bits per channel.
int3 toYUV(int3 c)
{
	const int3 c5 = c >> 3;
	const int3 c8 = (c5 << 3) | (c5 >> 3);
	return int3(dot(YUV_Y, c8) >> 15, 64 + (dot(YUV_U, c8) >> 15), 64 + (dot(YUV_V, c8) >> 15));
}

// True if the YUV values differ by more than the thresholds, as Scale_DiffYUV.
bool diffYUV(int3 a, int3 b)
{
	const int3 d = abs(a - b);
	return d.x > 0x40 || d.y > 0x12 || d.z > 0x0c;
}

// The blends of the CPU scalers, which drop low bits and add a half-error constant.
int3 blendRule(uint rule, int3 pix[10])
{
	const int3 c = pix[5];
	const int3 b1 = pix[(rule >> 3) & 15];
	const int3 b2 = pix[(rule >> 7) & 15];
	switch (rule & 7)
	{
		case 1: // 3:1
			return (c >> 1) + (c >> 2) + (b1 >> 2);
		case 2: // 2:1:1
			return (c >> 1) + (b1 >> 2) + (b2 >> 2);
		case 3: // 5:2:1
			return (c >> 1) + (c >> 3) + (b1 >> 2) + (b2 >> 3) + 2;
		case 4: // 6:1:1
			return (c >> 1) + (c >> 2) + (b1 >> 3) + (b2 >> 3) + 2;
		case 5: // 2:3:3
			return (c >> 2) + (b1 >> 2) + (b1 >> 3) + (b2 >> 2) + (b2 >> 3) + 2;
		case 6: // 14:1:1
			return (c >> 1) + (c >> 2) + (c >> 3) + (b1 >> 4) + (b2 >> 4) + 3;
		default:
			return c;
	}
}

float4 main(PSInput input) : SV_Target
{
	const Scale2x s = scale2xInput(input);

	// Pixels 1-9 are the 3x3 block around pixel 5, by rows.
	int3 pix[10];
	int3 yuv[10];
	pix[0] = int3(0, 0, 0);
	yuv[0] = int3(0, 0, 0);
	for (int i = 1; i <= 9; ++i)
	{
		pix[i] = pixel(s, int2((i - 1) % 3 - 1, (i - 1) / 3 - 1));
		yuv[i] = toYUV(pix[i]);
	}

	uint pattern = 0;
	const int neighbors[8] = {1, 2, 3, 4, 6, 7, 8, 9};
	for (int n = 0; n < 8; ++n)
	{
		if (diffYUV(yuv[5], yuv[neighbors[n]]))
		{
			pattern |= 1u << n;
		}
	}

	const uint4 patternRules = rules[pattern];
	const int quad = s.quad.y * 2 + s.quad.x;
	const uint code = quad == 0 ? patternRules.x : quad == 1 ? patternRules.y : quad == 2 ? patternRules.z : patternRules.w;
	const bool differ = (code & 1) != 0 && diffYUV(yuv[(code >> 1) & 15], yuv[(code >> 5) & 15]);
	const uint rule = differ ? (code >> 9) & 0x7ff : (code >> 20) & 0x7ff;
	return color(blendRule(rule, pix));
}
