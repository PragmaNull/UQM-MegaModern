//Copyright Paul Reiche, Fred Ford. 1992-2002

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>

#include "core/string/StringUtils.h"

#include "cons_res.h"
#include "resinst.h"
#include "nameref.h"
#include "setup.h"
#include "units.h"
// for NumViews
#include "planets/planets.h"

namespace uqm
{
// for NUMBER_OF_PLANET_TYPES, PLANET_SHIELDED

const static char* planet_types[] = {
	"oolite", "yttric", "quasidegenerate", "lanthanide", "treasure",
	"urea", "metal", "radioactive", "opalescent", "cyanic",
	"acid", "alkali", "halide", "green", "copper",
	"carbide", "ultramarine", "noble", "azure", "chondrite",
	"purple", "superdense", "pellucid", "dust", "crimson",
	"cimmerian", "infrared", "selenic", "auric", "fluorescent",
	"ultraviolet", "plutonic", "rainbow", "shattered", "sapphire",
	"organic", "xenolithic", "redux", "primordial", "emerald",
	"chlorine", "magnetic", "water", "telluric", "hydrocarbon",
	"iodine", "vinylogous", "ruby", "magma", "maroon",
	"bluegas", "cyangas", "greengas", "greygas", "orangegas",
	"purplegas", "redgas", "violetgas", "yellowgas"};

const static char* planet_sizes[] = {
	"large", "medium", "small"};

FRAME planet[NumViews];
static char buffer[80] {};

void load_gravity_well(uint8_t selector)
{
	uint16_t i {};

	if (selector == PlanetType::SaMatra)
	{
		if (IsExtended() && uqm::activityKind(uqm::currentActivity()) != ActivityKind::InLastBattle)
		{
			planet[0] = CaptureDrawable(
				LoadGraphic(SceneryMaskPmapAnimRes));
		}
		else
		{
			planet[0] = CaptureDrawable(
				LoadGraphic(SamatraBigMaskPmapAnimRes));
		}
		planet[1] = planet[2] = 0;
	}
	else
	{
		const char* ptype {};
		if (selector & *PlanetType::PlanetShielded)
		{
			ptype = "slaveshield";
		}
		else
		{
			ptype = planet_types[selector];
		}

		for (i = 0; i < NumViews; ++i)
		{
			fmt::format_to_sz_n(buffer, "planet.{}.{}", ptype, planet_sizes[i]);
			planet[i] = CaptureDrawable(LoadGraphic(buffer));
		}
	}
}


void free_gravity_well(void)
{
	for (uint16_t i {0}; i < NumViews; ++i)
	{
		DestroyDrawable(ReleaseDrawable(planet[i]));
		planet[i] = 0;
	}
}

FRAME load_life_form(uint8_t selector)
{
	fmt::format_to_sz_n(buffer, "graphics.life.{}", selector);
	return CaptureDrawable(LoadGraphic(buffer));
}

MUSIC_REF load_orbit_theme(uint8_t selector)
{
	fmt::format_to_sz_n(buffer, "music.orbit{}", selector + 1);
	return LoadMusicInstance(buffer);
}

MUSIC_REF loadMainMenuMusic(uint8_t selector)
{
	fmt::format_to_sz_n(buffer, "music.mainmenu{}", selector + 1);
	return LoadMusicInstance(buffer);
}

int num_orbit_themes(void)
{
	if (VolasPackPresent)
	{
		return NumOrbitThemes;
	}
	else
	{
		return NumOrbitThemes - 1;
	}
}

} // namespace uqm