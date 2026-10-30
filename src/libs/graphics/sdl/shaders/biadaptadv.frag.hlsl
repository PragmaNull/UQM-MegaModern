// The advanced biadapt 2x scaler, as Scale_BiAdaptAdvFilter.

#include "scale2x.hlsli"

static const int RGB_HIGH = 8000;
static const int YUV_LOW = 30;
static const int YUV_MED = 70;
static const int YUV_HIGH = 130;

// The 4x4 source pixels around the current one. P(0, 0) is the current pixel.
static int3 g_pixels[4][4];
#define P(x, y) g_pixels[(x) + 1][(y) + 1]

bool closeRGB(int3 a, int3 b)
{
	return same(a, b) || rgbDelta(a, b) <= RGB_HIGH;
}

bool closeYUV(int3 a, int3 b, int tolerance)
{
	return same(a, b) || cmpYUV(a, b, tolerance);
}

// Detects a 2:1 line through the current pixel, for the pixel below it.
bool lineBelowThroughCurrent()
{
	const int3 c = P(0, 0);
	return (same(c, P(-1, 0)) && same(c, P(1, 1)) && same(c, P(2, 1))
			&& ((!closeRGB(c, P(-1, -1)) && !closeRGB(c, P(0, -1)) && !closeRGB(c, P(1, 0)) && !closeRGB(c, P(2, 0)))
				|| (!closeRGB(c, P(-1, 1)) && !closeRGB(c, P(1, 2)) && !closeRGB(c, P(2, 2)))))
		|| (same(c, P(1, 0)) && same(c, P(-1, 1)) && same(c, P(2, -1))
			&& ((!closeRGB(c, P(-1, 0)) && !closeRGB(c, P(0, -1)) && !closeRGB(c, P(1, -1)))
				|| (!closeRGB(c, P(-1, 2)) && !closeRGB(c, P(1, 1)) && !closeRGB(c, P(2, 0)))));
}

// Detects a 2:1 line through the pixel below the current one.
bool lineBelowThroughBelow()
{
	const int3 c = P(0, 1);
	return (same(c, P(-1, 0)) && same(c, P(1, 1)) && same(c, P(2, 2))
			&& ((!closeRGB(c, P(-1, -1)) && !closeRGB(c, P(1, 0)) && !closeRGB(c, P(2, 1)))
				|| (!closeRGB(c, P(-1, 1)) && !closeRGB(c, P(0, 2)) && !closeRGB(c, P(1, 2)))))
		|| (same(c, P(1, 0)) && same(c, P(-1, 1)) && same(c, P(2, 0))
			&& ((!closeRGB(c, P(-1, 0)) && !closeRGB(c, P(1, -1)) && !closeRGB(c, P(2, -1)))
				|| (!closeRGB(c, P(-1, 2)) && !closeRGB(c, P(0, 2)) && !closeRGB(c, P(1, 1)) && !closeRGB(c, P(2, 1)))));
}

// Detects a 1:2 line through the current pixel, for the pixel to the right of it.
bool lineRightThroughCurrent()
{
	const int3 c = P(0, 0);
	return (same(c, P(1, -1)) && same(c, P(0, 1)) && same(c, P(-1, 2))
			&& ((!closeRGB(c, P(0, -1)) && !closeRGB(c, P(-1, 0)) && !closeRGB(c, P(-1, 1)))
				|| (!closeRGB(c, P(2, -1)) && !closeRGB(c, P(1, 1)) && !closeRGB(c, P(0, 2)))))
		|| (same(c, P(0, -1)) && same(c, P(1, 1)) && same(c, P(1, 2))
			&& ((!closeRGB(c, P(-1, -1)) && !closeRGB(c, P(-1, 0)) && !closeRGB(c, P(0, 1)) && !closeRGB(c, P(0, 2)))
				|| (!closeRGB(c, P(1, -1)) && !closeRGB(c, P(2, 1)) && !closeRGB(c, P(2, 2)))));
}

// Detects a 1:2 line through the pixel to the right of the current one.
bool lineRightThroughRight()
{
	const int3 c = P(1, 0);
	return (same(c, P(1, -1)) && same(c, P(0, 1)) && same(c, P(0, 2))
			&& ((!closeRGB(c, P(0, -1)) && !closeRGB(c, P(-1, 1)) && !closeRGB(c, P(-1, 2)))
				|| (!closeRGB(c, P(2, -1)) && !closeRGB(c, P(2, 0)) && !closeRGB(c, P(1, 1)) && !closeRGB(c, P(1, 2)))))
		|| (same(c, P(0, -1)) && same(c, P(1, 1)) && same(c, P(2, 2))
			&& ((!closeRGB(c, P(-1, -1)) && !closeRGB(c, P(0, 1)) && !closeRGB(c, P(1, 2)))
				|| (!closeRGB(c, P(1, -1)) && !closeRGB(c, P(2, 0)) && !closeRGB(c, P(2, 1)))));
}

float4 bottomLeft()
{
	if (cmpYUV(P(0, 0), P(0, 1), YUV_LOW))
	{
		return blend11(P(0, 0), P(0, 1));
	}
	if (lineBelowThroughCurrent())
	{
		return color(P(0, 0));
	}
	if (lineBelowThroughBelow())
	{
		return color(P(0, 1));
	}
	return blend11(P(0, 0), P(0, 1));
}

float4 topRight()
{
	if (cmpYUV(P(0, 0), P(1, 0), YUV_LOW))
	{
		return blend11(P(0, 0), P(1, 0));
	}
	if (lineRightThroughCurrent())
	{
		return color(P(0, 0));
	}
	if (lineRightThroughRight())
	{
		return color(P(1, 0));
	}
	return blend11(P(0, 0), P(1, 0));
}

float4 bottomRight()
{
	const int3 p00 = P(0, 0);
	const int3 p10 = P(1, 0);
	const int3 p01 = P(0, 1);
	const int3 p11 = P(1, 1);

	if (same(p00, p11) && same(p10, p01))
	{
		// Count the surrounding pixels of each color. The color with fewer is the line.
		const int2 around[8] = {int2(0, -1), int2(1, -1), int2(2, 0), int2(2, 1), int2(1, 2), int2(0, 2), int2(-1, 1), int2(-1, 0)};
		int cl = 2;
		int cr = 2;
		for (int i = 0; i < 8; ++i)
		{
			const int3 c = P(around[i].x, around[i].y);
			if (closeYUV(c, p00, YUV_MED))
			{
				++cl;
			}
			else if (closeYUV(c, p10, YUV_MED))
			{
				++cr;
			}
		}
		if (cl > cr)
		{
			return color(p10);
		}
		if (cr > cl)
		{
			return color(p00);
		}
		return blend11(p00, p10);
	}

	const bool belowMatches = same(p00, p01) || cmpYUV(p00, p01, YUV_LOW);
	const bool rightMatches = same(p00, p10) || cmpYUV(p00, p10, YUV_LOW);
	if ((belowMatches && rightMatches) || (closeYUV(p10, p01, YUV_LOW) && closeYUV(p10, p11, YUV_LOW)))
	{
		return blend11(p01, p10);
	}
	if ((belowMatches || rightMatches) && closeYUV(p00, p11, YUV_LOW))
	{
		return blend11(p00, p11);
	}

	const bool mainDiagonal = closeYUV(p00, p11, YUV_HIGH);
	const bool otherDiagonal = closeYUV(p10, p01, YUV_HIGH);
	if (mainDiagonal && otherDiagonal)
	{
		return lumaY(p00) > lumaY(p10) ? blend11(p00, p11) : blend11(p10, p01);
	}
	if (mainDiagonal)
	{
		return blend11(p00, p11);
	}
	if (otherDiagonal)
	{
		return blend11(p10, p01);
	}
	return blend1111(p00, p01, p10, p11);
}

float4 main(PSInput input) : SV_Target
{
	const Scale2x s = scale2xInput(input);
	for (int x = -1; x <= 2; ++x)
	{
		for (int y = -1; y <= 2; ++y)
		{
			P(x, y) = pixel(s, int2(x, y));
		}
	}

	const int3 p00 = P(0, 0);
	if (s.quad.x == 0 && s.quad.y == 0)
	{
		return color(p00);
	}

	const bool belowEqual = same(p00, P(0, 1));
	const bool rightEqual = same(p00, P(1, 0));
	if (belowEqual && rightEqual && same(p00, P(1, 1)))
	{
		return color(p00);
	}

	if (s.quad.x == 0)
	{
		return belowEqual ? color(p00) : bottomLeft();
	}
	if (s.quad.y == 0)
	{
		return rightEqual ? color(p00) : topRight();
	}
	return bottomRight();
}
