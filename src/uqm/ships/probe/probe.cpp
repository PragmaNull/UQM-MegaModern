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

#include "../ship.h"
#include "probe.h"
#include "resinst.h"

static constexpr auto MaxCrew {1};
static constexpr auto MaxEnergy {1};
static constexpr auto EnergyRegeneration {0};
static constexpr auto WeaponEnergyCost {0};
static constexpr auto SpecialEnergyCost {0};
static constexpr auto EnergyWait {0};
static constexpr auto MaxThrust {0};
static constexpr auto ThrustIncrement {0};
static constexpr auto TurnWait {0};
static constexpr auto ThrustWait {0};
static constexpr auto WeaponWait {0};
static constexpr auto SpecialWait {0};

static constexpr auto ShipMass {0};

static RACE_DESC probe_desc =
	{
		{/* SHIP_INFO */
		 "urquanprobe",
		 0,
		 0, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy, MaxEnergy,
		 0,
		 0,
		 UrquanDroneMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FLEET_STUFF */
			0, /* Initial sphere of influence radius */
			{
				/* Known location (center of SoI) */
				0,
				0,
			},
		 },
		{
			MaxThrust,
			ThrustIncrement,
			EnergyRegeneration,
			WeaponEnergyCost,
			SpecialEnergyCost,
			EnergyWait,
			TurnWait,
			ThrustWait,
			WeaponWait,
			SpecialWait,
			ShipMass,
		 },
		{{
			 NullResource,
			 NullResource,
			 NullResource,
		 },
		 {
			 NullResource,
			 NullResource,
			 NullResource,
		 },
		 {
			 NullResource,
			 NullResource,
			 NullResource,
		 },
		 {NullResource,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 NullResource,
		 NullResource,
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 nullptr,
		 nullptr},
		{
			0,
			0,
			nullptr,
		 },
		(UNINIT_FUNC*)nullptr,
		(PREPROCESS_FUNC*)nullptr,
		(POSTPROCESS_FUNC*)nullptr,
		(INIT_WEAPON_FUNC*)nullptr,
		0,
		0, /* CodeRef */
};

RACE_DESC* init_probe(void)
{
	RACE_DESC* RaceDescPtr {&probe_desc};

	return (RaceDescPtr);
}
