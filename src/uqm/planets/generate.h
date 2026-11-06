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

#ifndef GENERATE_H
#define GENERATE_H

typedef struct GenerateFunctions GenerateFunctions;

#include <cstdint>
#include "planets.h"
#include "core/misc/bittricks.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

/*
 * To do (for further cleanups):
 * - split off generateOrbital in a calculation and an activation
 *   (graphics and music) part.
 * - make generateOrbital return a meaningful value, specifically, whether
 *   or not the player is going into orbit
 * - for GenerateNameFunction, set the name in an argument, instead
 *   of in GLOBAL_SYS(PlanetName)
 * - make generateName work for moons
 * - add parameters to initNcs, reinitNpcs, and uninitNpcs, so that
 *   globals don't have to be used.
 * - Add a reference from each world to the solar system, so that most
 *   of these functions can do with one less argument.
 * - (maybe) don't directly call the generate functions via
 *   solarSys->genFuncs->..., but use a function for this, which first
 *   checks for solar system dependent handlers, and if this does not exist,
 *   or returns false, calls the default function.
 */

// Any of these functions returning true means that the action has been
// handled, and that the default function should not be called.
typedef bool (*InitNpcsFunction)(SolarSystemState* solarSys);
typedef bool (*ReinitNpcsFunction)(SolarSystemState* solarSys);
typedef bool (*UninitNpcsFunction)(SolarSystemState* solarSys);
typedef bool (*GeneratePlanetsFunction)(SolarSystemState* solarSys);
typedef bool (*GenerateMoonsFunction)(SolarSystemState* solarSys,
									  SystemPlanet* planet);
typedef bool (*GenerateOrbitalFunction)(SolarSystemState* solarSys,
										SystemPlanet* world);
typedef bool (*GenerateNameFunction)(const SolarSystemState*,
									 const SystemPlanet* world);
// The following functions return the number of objects being generated
// (or the index of the current object in some cases)
typedef uint16_t (*GenerateMineralsFunction)(const SolarSystemState*,
											 const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
typedef uint16_t (*GenerateEnergyFunction)(const SolarSystemState*,
										   const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
typedef uint16_t (*GenerateLifeFunction)(const SolarSystemState*,
										 const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
// The following functions return true if the node should be removed
// from the surface, i.e. picked up.
typedef bool (*PickupMineralsFunction)(SolarSystemState* solarSys,
									   SystemPlanet* world, uint16_t whichNode);
typedef bool (*PickupEnergyFunction)(SolarSystemState* solarSys,
									 SystemPlanet* world, uint16_t whichNode);
typedef bool (*PickupLifeFunction)(SolarSystemState* solarSys,
								   SystemPlanet* world, uint16_t whichNode);


struct GenerateFunctions
{
	InitNpcsFunction initNpcs;
	// Ships in the solar system, the first time it is accessed.
	ReinitNpcsFunction reinitNpcs;
	// Ships in the solar system, every next time it is accessed.
	UninitNpcsFunction uninitNpcs;
	// When leaving the solar system.
	GeneratePlanetsFunction generatePlanets;
	// Layout of planets within a solar system.
	GenerateMoonsFunction generateMoons;
	// Layout of moons around a planet.
	GenerateNameFunction generateName;
	// Name of a planet.
	GenerateOrbitalFunction generateOrbital;
	// Characteristics of words (planets and moons).
	GenerateMineralsFunction generateMinerals;
	// Minerals on the planet surface.
	GenerateEnergyFunction generateEnergy;
	// Energy sources on the planet surface.
	GenerateLifeFunction generateLife;
	// Bio on the planet surface.
	PickupMineralsFunction pickupMinerals;
	PickupEnergyFunction pickupEnergy;
	PickupLifeFunction pickupLife;
};

// JSD - GenerateFunctions moved from gendef.h due to dependency issues caused by needing
// to #include planets/generate.h only for this one declaration.  Anyway, it belongs here.
const GenerateFunctions* getGenerateFunctions(uint8_t Index);
#if 0 //defined(__cplusplus)
}
#endif

#endif /* GENERATE_H */
