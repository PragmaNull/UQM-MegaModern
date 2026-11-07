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

#pragma once
#ifndef UQM_PLANETS_ELEMDATA_H
#define UQM_PLANETS_ELEMDATA_H

#include "core/types/enum.h"
#include "core/misc/bittricks.h"

namespace uqm
{
#if 0 //defined(__cplusplus)
extern "C" {
#endif

/*------------------------------ Type Defines ----------------------------- */
enum class MineralCategory
{
	Common = 0,
	Corrosive,
	BaseMetal,
	Noble,
	RareEarth,
	Precious,
	Radioactive,
	Exotic,

	NumElementCategories
};
consteval bool isArithmeticEnum(MineralCategory) { return true; }


/*------------------------------ Global Data ------------------------------ */

enum class ChemicalElement
{
	Hydrogen, // Unused - Common
	Helium,
	Lithium,
	Beryllium,
	Boron,
	Carbon,
	Nitrogen, // Unused - Common
	Oxygen,
	Fluorine,
	Neon,
	Sodium,
	Magnesium,
	Aluminum,
	Silicon,
	Phosphorus,
	Sulfur,
	Chlorine,
	Argon,
	Potassium,
	Calcium,
	Scandium,
	Titanium,
	Vanadium,
	Chromium,
	Manganese,
	Iron,
	Cobalt,
	Nickel,
	Copper,
	Zinc,
	Gallium,
	Germanium,
	Arsenic,
	Selenium,
	Bromine,
	Krypton,
	Rubidium,
	Strontium,
	Yttrium,
	Zirconium,
	Niobium,
	Molybdenum,
	Technetium,
	Ruthenium,
	Rhodium,
	Palladium,
	Silver,
	Cadmium,
	Indium,
	Tin,
	Antimony,  // Unused - Base Metal
	Tellurium, // Unused - Base Metal
	Iodine,
	Xenon,
	Cesium,
	Barium,
	Lanthanum,
	Cerium,
	Praseodymium,
	Neodymium,
	Promethium,
	Samarium,
	Europium, // Unused - Rare Earth
	Gadolinium,
	Terbium,
	Dyprosium, // Unused - Rare Earth
	Holmium,
	Erbium,
	Thulium,
	Ytterbium,
	Lutetium,
	Hafnium, // Unused - Base Metal
	Tantalum,
	Tungsten,
	Rhenium,
	Osmium,
	Iridium,
	Platinum,
	Gold,
	Mercury,
	Thallium,
	Lead,
	Bismuth,
	Polonium,
	Astatine,
	Radon,
	Francium,
	Radium,
	Actinium,
	Thorium,
	Protactinium,
	Uranium,
	Neptunium,
	Plutonium,
	NumberOfNormal,

	Ozone = NumberOfNormal,
	FreeRadicals,
	CarbonDioxide,
	CarbonMonoxide, // Unused - Common
	Ammonia,
	Methane,
	SulfuricAcid,
	HydrochloricAcid,
	HydrocyanicAcid,
	FormicAcid,
	PhosphoricAcid,
	Formaldehyde,
	Cyanoacetylene,
	Methanol,
	Ethanol,
	SiliconMonoxide,  // Unused - Common
	TitaniumOxide,	  // Unused - Common
	ZirconiumOxide,	  // Unused - Common
	Water,			  // Unused - Common
	SiliconCompounds, // Unused - Common
	MetalOxides,	  // Unused - Common
	QuantumBh,		  // Unused - Exotic
	Neutronium,
	MagneticMonopoles,
	DegenerateMatter,
	RtSuperFluid,
	AguutiNodules,
	IronCompounds,		  // Unused - Common
	AluminumCompounds,	  // Unused - Common
	NitrousOxide,		  // Unused - Common
	RadioactiveCompounds, // Unused - Radioactive
	Hydrocarbons,
	CarbonCompounds, // Unused - Common
	Antimatter,
	CharonDust, // Unused - Exotic
	ReisburgHelices,
	TzoCrystals,
	CalciumCompounds, // Unused - Common
	NitricAcid,

	NumberOfElements
};
consteval bool isArithmeticEnum(ChemicalElement) { return true; }


inline constexpr auto Few {1};
inline constexpr auto Moderate {4};
inline constexpr auto Numerous {8};

enum class DepositSize
{
	Light = 0,
	Medium,
	Heavy
};
consteval bool isArithmeticEnum(DepositSize) { return true; }
inline constexpr auto Nothing {(uint8_t)(~0)};

constexpr auto MineralDeposit(const auto& qn, const auto& ql) { return uqm::makeByte(qn, ql); }
constexpr auto DepositQuantity(const auto& md) { return uqm::lowNibble(md); }
constexpr auto DepositQuality(const auto& md) { return uqm::highNibble(md); }

inline constexpr auto MaxElementUnits {0xF};

const extern uint8_t* Elements;
inline auto ElementCategory(const auto& et) { return Elements[et] & 0x7; }

#if 0 //defined(__cplusplus)
}
#endif

} // namespace uqm

#endif // UQM_PLANETS_ELEMDATA_H
