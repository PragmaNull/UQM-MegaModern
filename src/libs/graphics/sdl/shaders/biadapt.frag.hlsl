// The biadapt 2x scaler, as Scale_BiAdaptFilter.

#include "scale2x.hlsli"

float4 main(PSInput input) : SV_Target
{
	const Scale2x s = scale2xInput(input);
	const bool lastCol = s.pos.x + 1 >= s.size.x;
	const bool lastRow = s.pos.y + 1 >= s.size.y;
	const int3 tl = pixel(s, int2(0, 0));

	if (s.quad.x == 0 && s.quad.y == 0)
	{
		return color(tl);
	}

	const int3 bl = pixel(s, int2(0, 1));
	if (s.quad.x == 0)
	{
		return same(tl, bl) ? color(tl) : blend11(tl, bl);
	}

	if (s.quad.y == 0)
	{
		if (lastCol)
		{
			return color(tl);
		}
		const int3 tr = pixel(s, int2(1, 0));
		return same(tl, tr) ? color(tr) : blend11(tl, tr);
	}

	if (lastCol)
	{
		return same(tl, bl) ? color(tl) : blend11(tl, bl);
	}
	if (lastRow)
	{
		return color(tl);
	}

	const int3 tr = pixel(s, int2(1, 0));
	const int3 br = pixel(s, int2(1, 1));
	if (same(tl, br) && same(tr, bl))
	{
		if (same(tl, tr))
		{
			return color(tl);
		}

		// Count the surrounding pixels of each color. The color with fewer is the line.
		int cl = 1;
		int cr = 1;
		const int2 around[8] = {int2(-1, 0), int2(-1, 1), int2(0, -1), int2(1, -1), int2(2, 0), int2(2, 1), int2(0, 2), int2(1, 2)};
		const bool inside[8] = {
			s.pos.x > 0, s.pos.x > 0,
			s.pos.y > 0, s.pos.y > 0,
			s.pos.x + 2 < s.size.x, s.pos.x + 2 < s.size.x,
			s.pos.y + 2 < s.size.y, s.pos.y + 2 < s.size.y};
		for (int i = 0; i < 8; ++i)
		{
			if (inside[i])
			{
				const int3 c = pixel(s, around[i]);
				if (same(c, tl))
				{
					++cl;
				}
				else if (same(c, tr))
				{
					++cr;
				}
			}
		}

		if (cl > cr)
		{
			return color(tr);
		}
		if (cr > cl)
		{
			return color(tl);
		}
		return blend11(tl, tr);
	}
	if (same(tl, br))
	{
		return color(tl);
	}
	if (same(tr, bl))
	{
		return color(tr);
	}
	return blend1111(tl, bl, tr, br);
}
