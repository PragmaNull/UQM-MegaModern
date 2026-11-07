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
#ifndef UQM_PLANETS_GENERATE_GENDEFAULT_H
#define UQM_PLANETS_GENERATE_GENDEFAULT_H

#include "core/types/enum.h"
#include <cstdint>
#include "../planets.h"
#include "core/misc/bittricks.h"

namespace uqm
{
#if 0 //defined(__cplusplus)
extern "C" {
#endif

inline constexpr auto SmallRocky {1 << 0};
inline constexpr auto LargeRocky {1 << 1};
inline constexpr auto AllRocky {1 << 2};
inline constexpr auto OnlyLarge {1 << 3};
inline constexpr auto OnlyGas {1 << 4};


bool GenerateDefault_initNpcs(SolarSystemState* solarSys);
bool GenerateDefault_reinitNpcs(SolarSystemState* solarSys);
bool GenerateDefault_uninitNpcs(SolarSystemState* solarSys);
bool GenerateDefault_generatePlanets(SolarSystemState* solarSys);
bool GenerateDefault_generateMoons(SolarSystemState* solarSys,
								   SystemPlanet* planet);
bool GenerateDefault_generateName(const SolarSystemState*,
								  const SystemPlanet* world);
bool GenerateDefault_generateOrbital(SolarSystemState* solarSys,
									 SystemPlanet* world);
uint16_t GenerateDefault_generateMinerals(const SolarSystemState*,
										  const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
uint16_t GenerateDefault_generateEnergy(const SolarSystemState*,
										const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
uint16_t GenerateDefault_generateLife(const SolarSystemState*,
									  const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
bool GenerateDefault_pickupMinerals(SolarSystemState*, SystemPlanet* world,
									uint16_t whichNode);
bool GenerateDefault_pickupEnergy(SolarSystemState*, SystemPlanet* world,
								  uint16_t whichNode);
bool GenerateDefault_pickupLife(SolarSystemState*, SystemPlanet* world,
								uint16_t whichNode);

uint16_t GenerateDefault_generateArtifact(const SolarSystemState*,
										  uint16_t whichNode, SurfaceNode* info);
uint16_t GenerateDefault_generateRuins(const SolarSystemState*,
									   uint16_t whichNode, SurfaceNode* info);
bool GenerateDefault_landerReport(SolarSystemState*);
bool GenerateDefault_landerReportCycle(SolarSystemState*);
extern void GeneratePlanets(SolarSystemState* system);
extern bool CheckForHabitable(SolarSystemState* solarSys);
extern uint8_t GenerateWorlds(uint8_t whichType);
extern void GenerateGasGiantRanged(SolarSystemState* solarSys);
extern uint8_t GenerateCrystalWorld(void);
extern uint8_t GenerateDesolateWorld(void);
extern uint8_t GenerateHabitableWorld(void);
extern uint8_t GenerateGasGiantWorld(void);
extern uint8_t GenerateMinPlanets(uint8_t min);
extern uint8_t GenerateNumberOfPlanets(uint8_t minimum);
extern uint8_t PlanetByteGen(SystemPlanet* pPDesc);

const extern GenerateFunctions generateDefaultFunctions;

enum class HardModeEncounter
{
	Ilwrath,		 // 14/6 ships at procyon even if ilwrath left
	Probe,			 // 4/2 probes at dreadnought wreck
	Urquan,			 // 6 dreadnoughts at vault if the shuttle is on board
	Eggcase00,		 // 4 mycon
	Eggcase01,		 // 4 mycon
	Eggcase02,		 // 4 mycon
	Thraddash,		 // 12/6 thraddash minimum at all time
	NoHelpFromPkunk, // no help before final battle if pkunk are absorbed before the revolt
	ReadyToBargain	 // druuge sell rosy sphere on second deal
};
consteval bool isArithmeticEnum(HardModeEncounter) { return true; }

#if 0 //defined(__cplusplus)
}
#endif

} // namespace uqm

#endif // UQM_PLANETS_GENERATE_GENDEFAULT_H
