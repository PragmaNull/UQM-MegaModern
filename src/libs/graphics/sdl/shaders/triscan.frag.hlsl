// The triscan 2x scaler, as Scale_TriScanFilter.

#include "scale2x.hlsli"

static const int TOLERANCE = 100;

bool similar(int3 a, int3 b)
{
	return same(a, b) || cmpYUV(a, b, TOLERANCE);
}

float4 main(PSInput input) : SV_Target
{
	const Scale2x s = scale2xInput(input);
	const int3 center = pixel(s, int2(0, 0));
	const int3 up = pixel(s, int2(0, -1));
	const int3 down = pixel(s, int2(0, 1));
	const int3 left = pixel(s, int2(-1, 0));
	const int3 right = pixel(s, int2(1, 0));

	if (similar(up, down) || similar(left, right))
	{
		return color(center);
	}

	const int3 side = s.quad.x == 0 ? left : right;
	const int3 vertical = s.quad.y == 0 ? up : down;
	return similar(side, vertical) ? blend11(side, vertical) : color(center);
}
