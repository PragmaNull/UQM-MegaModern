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

#include <math.h>

#include "../../build.h"
#include "../../encount.h"
#include "../../gamestr.h"
#include "../../globdata.h"
#include "../../grpinfo.h"
#include "../../nameref.h"
#include "../../setup.h"
#include "../../sounds.h"
#include "../../state.h"
#include "../lander.h"
#include "../lifeform.h"
#include "../planets.h"
#include "genall.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
static bool GenerateSol_initNpcs(SolarSystemState* solarSys);
static bool GenerateSol_reinitNpcs(SolarSystemState* solarSys);
static bool GenerateSol_generatePlanets(SolarSystemState* solarSys);
static bool GenerateSol_generateMoons(SolarSystemState* solarSys,
									  SystemPlanet* planet);
static bool GenerateSol_generateName(const SolarSystemState*,
									 const SystemPlanet* world);
static bool GenerateSol_generateOrbital(SolarSystemState* solarSys,
										SystemPlanet* world);
static uint16_t GenerateSol_generateMinerals(const SolarSystemState* solarSys,
											 const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
static uint16_t GenerateSol_generateEnergy(const SolarSystemState*,
										   const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
static uint16_t GenerateSol_generateLife(const SolarSystemState*,
										 const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
static bool GenerateSol_pickupEnergy(SolarSystemState* solarSys,
									 SystemPlanet* world, uint16_t whichNode);

static int init_probe(void);
static void check_probe(void);


const GenerateFunctions generateSolFunctions = {
	/* .initNpcs         = */ GenerateSol_initNpcs,
	/* .reinitNpcs       = */ GenerateSol_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateSol_generatePlanets,
	/* .generateMoons    = */ GenerateSol_generateMoons,
	/* .generateName     = */ GenerateSol_generateName,
	/* .generateOrbital  = */ GenerateSol_generateOrbital,
	/* .generateMinerals = */ GenerateSol_generateMinerals,
	/* .generateEnergy   = */ GenerateSol_generateEnergy,
	/* .generateLife     = */ GenerateSol_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateSol_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool GenerateSol_initNpcs(SolarSystemState* solarSys)
{
	if (uqm::UQMOptions::read().headStart)
	{
		uqm::gameState().setFlag(uqm::GameFlag::PROBE_MESSAGE_DELIVERED, static_cast<uint32_t>(1));
	}
	uqm::gameState().game().BattleGroupRef = uqm::gameState().flag(uqm::GameFlag::URQUAN_PROBE_GRPOFFS);
	if (uqm::gameState().game().BattleGroupRef == 0 && !uqm::gameState().flag(uqm::GameFlag::PROBE_MESSAGE_DELIVERED))
	{
		CloneShipFragment(RaceId::UrquanDrone,
						  &uqm::gameState().game().npc_built_ship_q, 0);
		uqm::gameState().game().BattleGroupRef = PutGroupInfo(GroupsAddNew, 1);
		ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
		uqm::gameState().setFlag(uqm::GameFlag::URQUAN_PROBE_GRPOFFS, static_cast<uint32_t>(uqm::gameState().game().BattleGroupRef));
	}

	if (!init_probe())
	{
		GenerateDefault_initNpcs(solarSys);
	}
	else if (SpaceMusicOK)
	{
		findRaceSOI();
	}

	return true;
}

static bool GenerateSol_reinitNpcs(SolarSystemState* solarSys)
{
	if (uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) != 3)
	{
		GenerateDefault_reinitNpcs(solarSys);
		check_probe();
	}
	else
	{
		uqm::gameState().game().BattleGroupRef = 0;
		ReinitQueue(&uqm::gameState().game().ip_group_q);
		assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);
	}
	return true;
}

static constexpr auto SolSeed {334241042L};

static bool GenerateSol_generatePlanets(SolarSystemState* solarSys)
{
	SysGenRNG->reseed(SolSeed + (uqm::isStarSeed() ? uqm::currentSeed().value : 0));

	solarSys->SunDesc[0].NumPlanets = 9;
	for (uint16_t planetI {0}; planetI < 9; ++planetI)
	{
		SystemPlanet* pCurDesc = &solarSys->PlanetDesc[planetI];

		pCurDesc->rand_seed = SysGenRNG->next();
		const uint32_t rand_val {pCurDesc->rand_seed};
		const uint16_t word_val {uqm::lowWord(rand_val)};
		pCurDesc->angle = NormalizeAngle((uint16_t)uqm::highByte(word_val));

		switch (planetI)
		{
			case 0: /* MERCURY */
				pCurDesc->data_index = *PlanetType::Metal;
				pCurDesc->radius = EarthRadius * 39L / 100;
				pCurDesc->NumPlanets = 0;
				break;
			case 1: /* VENUS */
				pCurDesc->data_index = *PlanetType::Primordial;
				pCurDesc->radius = EarthRadius * 72L / 100;
				pCurDesc->NumPlanets = 0;
				pCurDesc->angle =
					NormalizeAngle(FullCircle - pCurDesc->angle);
				break;
			case 2: /* EARTH */
				pCurDesc->data_index = *PlanetType::Water | *PlanetType::PlanetShielded;
				pCurDesc->radius = EarthRadius;
				pCurDesc->NumPlanets = 2;
				break;
			case 3: /* MARS */
				pCurDesc->data_index = *PlanetType::Dust;
				pCurDesc->radius = EarthRadius * 152L / 100;
				pCurDesc->NumPlanets = 0;
				break;
			case 4: /* JUPITER */
				pCurDesc->data_index = *PlanetType::RedGasGiant;
				pCurDesc->radius = EarthRadius * 500L /* 520L */ / 100;
				pCurDesc->NumPlanets = 4;
				break;
			case 5: /* SATURN */
				pCurDesc->data_index = *PlanetType::OraGasGiant;
				pCurDesc->radius = EarthRadius * 750L /* 952L */ / 100;
				pCurDesc->NumPlanets = 1;
				break;
			case 6: /* URANUS */
				pCurDesc->data_index = *PlanetType::GrnGasGiant;
				pCurDesc->radius = EarthRadius * 1000L /* 1916L */ / 100;
				pCurDesc->NumPlanets = 0;
				break;
			case 7: /* NEPTUNE */
				pCurDesc->data_index = *PlanetType::BluGasGiant;
				pCurDesc->radius = EarthRadius * 1250L /* 2999L */ / 100;
				pCurDesc->NumPlanets = 1;
				break;
			case 8: /* PLUTO */
				pCurDesc->data_index = *PlanetType::Pellucid;
				pCurDesc->radius = EarthRadius * 1550L /* 3937L */ / 100;
				pCurDesc->NumPlanets = IsExtended();
				if (uqm::isOriginalSeed())
				{
					pCurDesc->angle = FullCircle - Octant;
				}
				break;
		}
		pCurDesc->orb_speed = FullCircle / (365.25 * pow((float)pCurDesc->radius / EarthRadius, 1.5));
		pCurDesc->location.x = Cosine(pCurDesc->angle, pCurDesc->radius);
		pCurDesc->location.y = Sine(pCurDesc->angle, pCurDesc->radius);
	}

	return true;
}

static bool GenerateSol_generateMoons(SolarSystemState* solarSys, SystemPlanet* planet)
{
	uint32_t rand_val {};

	GenerateDefault_generateMoons(solarSys, planet);

	const uint16_t planetNr {planetIndex(solarSys, planet)};
	switch (planetNr)
	{
		case 2: /* moons of EARTH */
			{
				uint16_t angle {};

				/* Starbase: */
				solarSys->MoonDesc[0].data_index = *PlanetType::HierarchyStarbase;
				solarSys->MoonDesc[0].radius = MinMoonRadius();
				angle = HalfCircle + Quadrant;
				solarSys->MoonDesc[0].location.x =
					Cosine(angle, solarSys->MoonDesc[0].radius);
				solarSys->MoonDesc[0].location.y =
					Sine(angle, solarSys->MoonDesc[0].radius);
				solarSys->MoonDesc[0].orb_speed = FullCircle / 11.46;

				/* Luna: */
				solarSys->MoonDesc[1].data_index = *PlanetType::Selenic;
				solarSys->MoonDesc[1].radius = MinMoonRadius()
											 + (4 - 1) * MoonDelta();
				rand_val = SysGenRNG->next();
				angle = NormalizeAngle(uqm::lowWord(rand_val));
				solarSys->MoonDesc[1].location.x =
					Cosine(angle, solarSys->MoonDesc[1].radius);
				solarSys->MoonDesc[1].location.y =
					Sine(angle, solarSys->MoonDesc[1].radius);
				solarSys->MoonDesc[1].orb_speed = FullCircle / 27.32;
				break;
			}
		case 4: /* moons of JUPITER */
			solarSys->MoonDesc[0].data_index = *PlanetType::Radioactive;
			solarSys->MoonDesc[0].orb_speed = FullCircle / 1.75;
			/* Io */
			solarSys->MoonDesc[1].data_index = *PlanetType::Halide;
			solarSys->MoonDesc[1].orb_speed = FullCircle / 3.5;
			/* Europa */
			solarSys->MoonDesc[2].data_index = *PlanetType::Cyanic;
			solarSys->MoonDesc[2].orb_speed = FullCircle / 7.16;
			/* Ganymede */
			solarSys->MoonDesc[3].data_index = *PlanetType::Pellucid;
			solarSys->MoonDesc[3].orb_speed = FullCircle / 16.7;
			/* Callisto */
			break;
		case 5: /* moons of SATURN */
			solarSys->MoonDesc[0].data_index = *PlanetType::Alkali;
			/*solarSys->MoonDesc[0].radius = MinMoonRadius()
					+ (MaxGenMoons - 1) * MoonDelta();*/
			solarSys->MoonDesc[0].orb_speed = FullCircle / 15.22;
			/* Titan */
			break;
		case 7: /* moons of NEPTUNE */
			solarSys->MoonDesc[0].data_index = *PlanetType::Vinylogous;
			solarSys->MoonDesc[0].orb_speed = FullCircle / -5.87;
			/* Triton */
			break;
		case 8: /* moons of Pluto */
			solarSys->MoonDesc[0].data_index = *PlanetType::Selenic;
			solarSys->MoonDesc[0].orb_speed = FullCircle / 6.38;
			/* Charon */
			break;
	}

	return true;
}

static bool GenerateSol_generateName(const SolarSystemState* solarSys,
									 const SystemPlanet* world)
{
	const uint16_t planetNr = planetIndex(solarSys, world);

	uqm::utf8StringCopy(uqm::gameState().sis().PlanetName,
						GameString(GameStringBase::PlanetNumber + planetNr));
	uqm::gameState().setFlag(uqm::GameFlag::BATTLE_PLANET, static_cast<uint32_t>(solarSys->PlanetDesc[planetNr].data_index));

	return true;
}

static bool GenerateSol_generateOrbital(SolarSystemState* solarSys, SystemPlanet* world)
{
	uint32_t rand_val {};
	uint16_t planetNr {};

	if (matchWorld(solarSys, world, 2, 0))
	{
		if (getNomadMode() == uqm::NomadMode::Off)
		{
			/* Starbase */
			PutGroupInfo(GroupsRandom, GroupSaveIp);
			ReinitQueue(&uqm::gameState().game().ip_group_q);
			assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

			EncounterGroup = 0;
			uqm::currentActivity() |= ActivityFlag::StartEncounter;
			uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>((uint8_t)~0));

			return true;
		}
		else
		{
			LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);

			solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(NomadBaseStrtabRes);
			solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;

			DoDiscoveryReport(MenuSounds);

			solarSys->SysInfo.PlanetInfo.DiscoveryStrings.reset();
			FreeLanderFont(&solarSys->SysInfo.PlanetInfo);

			return true;
		}
	}

	DoPlanetaryAnalysis(&solarSys->SysInfo, world);
	rand_val = SysGenRNG->seed();

	solarSys->SysInfo.PlanetInfo.ScanSeed[*ScanType::Mineral] = rand_val;
	GenerateMineralDeposits(&solarSys->SysInfo, GenerateAll, nullptr);
	rand_val = SysGenRNG->seed();

	planetNr = planetIndex(solarSys, world);
	if (worldIsPlanet(solarSys, world))
	{
		switch (planetNr)
		{
			case 0: /* MERCURY */
				solarSys->SysInfo.PlanetInfo.AtmoDensity = 0;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 98;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 38;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 3;
				solarSys->SysInfo.PlanetInfo.Weather = 0;
				solarSys->SysInfo.PlanetInfo.Tectonics = 2;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 59 * 240;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 165;
				break;
			case 1: /* VENUS */
				solarSys->SysInfo.PlanetInfo.AtmoDensity = 90 * EarthAtmosphere;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 95;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 95;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 177;
				solarSys->SysInfo.PlanetInfo.Weather = 7;
				solarSys->SysInfo.PlanetInfo.Tectonics = 1;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 243 * 240;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 457;
				break;
			case 2: /* EARTH */
				solarSys->SysInfo.PlanetInfo.AtmoDensity =
					EarthAtmosphere;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 100;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 100;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 23;
				solarSys->SysInfo.PlanetInfo.Weather = 1;
				solarSys->SysInfo.PlanetInfo.Tectonics = 1;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 240;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 22;
				break;
			case 3: /* MARS */
				// XXX: Mars atmo should actually be 1/2 in current units
				solarSys->SysInfo.PlanetInfo.AtmoDensity = 1;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 72;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 53;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 24;
				solarSys->SysInfo.PlanetInfo.Weather = 1;
				solarSys->SysInfo.PlanetInfo.Tectonics = 1;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 246;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -53;
				break;
			case 4: /* JUPITER */
				solarSys->SysInfo.PlanetInfo.AtmoDensity =
					GasGiantAtmosphere;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 24;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 1120;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 3;
				solarSys->SysInfo.PlanetInfo.Weather = 7;
				solarSys->SysInfo.PlanetInfo.Tectonics = 0;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 98;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -143;
				solarSys->SysInfo.PlanetInfo.PlanetToSunDist =
					EarthRadius * 520L / 100;
				break;
			case 5: /* SATURN */
				solarSys->SysInfo.PlanetInfo.AtmoDensity =
					GasGiantAtmosphere;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 13;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 945;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 27;
				solarSys->SysInfo.PlanetInfo.Weather = 7;
				solarSys->SysInfo.PlanetInfo.Tectonics = 0;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 102;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -197;
				solarSys->SysInfo.PlanetInfo.PlanetToSunDist =
					EarthRadius * 952L / 100;
				break;
			case 6: /* URANUS */
				solarSys->SysInfo.PlanetInfo.AtmoDensity =
					GasGiantAtmosphere;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 21;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 411;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 98;
				solarSys->SysInfo.PlanetInfo.Weather = 7;
				solarSys->SysInfo.PlanetInfo.Tectonics = 0;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 172;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -217;
				solarSys->SysInfo.PlanetInfo.PlanetToSunDist =
					EarthRadius * 1916L / 100;
				break;
			case 7: /* NEPTUNE */
				solarSys->SysInfo.PlanetInfo.AtmoDensity =
					GasGiantAtmosphere;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 28;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 396;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 30;
				solarSys->SysInfo.PlanetInfo.Weather = 7;
				solarSys->SysInfo.PlanetInfo.Tectonics = 0;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 182;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -229;
				solarSys->SysInfo.PlanetInfo.PlanetToSunDist =
					EarthRadius * 2999L / 100;
				break;
			case 8: /* PLUTO */
				if (!uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI))
				{
					LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
					solarSys->PlanetSideFrame[1] =
						CaptureDrawable(
							LoadGraphic(SpaplutoMaskPmapAnimRes));
					solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(SpaplutoStrtabRes);
					solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
				}

				solarSys->SysInfo.PlanetInfo.AtmoDensity = 0;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 33;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 18;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 119;
				solarSys->SysInfo.PlanetInfo.Weather = 0;
				solarSys->SysInfo.PlanetInfo.Tectonics = 0;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 1533;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -235;
				solarSys->SysInfo.PlanetInfo.PlanetToSunDist =
					EarthRadius * 3937L / 100;
				break;
		}

		solarSys->SysInfo.PlanetInfo.SurfaceGravity =
			CalcGravity(&solarSys->SysInfo.PlanetInfo);

		{ // For Sol Textures
			ResourceName maskAnim = nullptr;

			if (uqm::UQMOptions::read().sphereType)
			{
				switch (planetNr)
				{
					case 0: // MERCURY
						maskAnim = MercuryMaskAnimRes;
						break;
					case 1: // VENUS
						maskAnim = VenusMaskAnimRes;
						break;
					case 2: // EARTH
						maskAnim = EarthMaskAnimRes;
						break;
					case 3: // MARS
						maskAnim = MarsMaskAnimRes;
						break;
					case 4: // JUPITER
						maskAnim = JupiterMaskAnimRes;
						break;
					case 5: // SATURN
						maskAnim = SaturnMaskAnimRes;
						break;
					case 6: // URANUS
						maskAnim = UranusMaskAnimRes;
						break;
					case 7: // NEPTUNE
						maskAnim = NeptuneMaskAnimRes;
						break;
					case 8: // PLUTO
						maskAnim = PlutoMaskAnimRes;
						break;
				}
			}
			else if (planetNr == 2)
			{
				maskAnim = EarthMaskAnimRes;
			}

			LoadPlanet(CaptureDrawable(LoadGraphic(maskAnim)));
		}
	}
	else
	{
		// World is a moon.
		const uint16_t moonNr = moonIndex(solarSys, world);

		solarSys->SysInfo.PlanetInfo.AxialTilt = 0;
		solarSys->SysInfo.PlanetInfo.AtmoDensity = 0;
		solarSys->SysInfo.PlanetInfo.Weather = 0;
		switch (planetNr)
		{
			case 2: /* moons of EARTH */
				//   NOTE: Even though we save the seed here, it is
				//   irrelevant. The seed will be used to randomly place
				//   the tractors, but since they are mobile, they will be
				//   moved to different locations not governed by this seed.
				solarSys->SysInfo.PlanetInfo.ScanSeed[*ScanType::Biological] =
					rand_val;

				if (!uqm::gameState().flag(uqm::GameFlag::MOONBASE_DESTROYED))
				{
					LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
					solarSys->PlanetSideFrame[1] =
						CaptureDrawable(
							LoadGraphic(MoonbaseMaskPmapAnimRes));
					solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(MoonbaseStrtabRes);
					solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
				}

				solarSys->SysInfo.PlanetInfo.PlanetDensity = 60;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 25;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 0;
				solarSys->SysInfo.PlanetInfo.Tectonics = 0;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 240 * 29;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -18;
				break;

			case 4: /* moons of JUPITER */
				solarSys->SysInfo.PlanetInfo.PlanetToSunDist =
					EarthRadius * 520L / 100;
				switch (moonNr)
				{
					case 0: /* Io */
						solarSys->SysInfo.PlanetInfo.PlanetDensity = 69;
						solarSys->SysInfo.PlanetInfo.PlanetRadius = 25;
						solarSys->SysInfo.PlanetInfo.Tectonics = 3;
						solarSys->SysInfo.PlanetInfo.RotationPeriod = 390;
						solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -163;
						break;
					case 1: /* Europa */
						solarSys->SysInfo.PlanetInfo.PlanetDensity = 54;
						solarSys->SysInfo.PlanetInfo.PlanetRadius = 25;
						solarSys->SysInfo.PlanetInfo.Tectonics = 1;
						solarSys->SysInfo.PlanetInfo.RotationPeriod = 840;
						solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -161;
						break;
					case 2: /* Ganymede */
						solarSys->SysInfo.PlanetInfo.PlanetDensity = 35;
						solarSys->SysInfo.PlanetInfo.PlanetRadius = 41;
						solarSys->SysInfo.PlanetInfo.Tectonics = 0;
						solarSys->SysInfo.PlanetInfo.RotationPeriod = 1728;
						solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -164;
						break;
					case 3: /* Callisto */
						solarSys->SysInfo.PlanetInfo.PlanetDensity = 35;
						solarSys->SysInfo.PlanetInfo.PlanetRadius = 38;
						solarSys->SysInfo.PlanetInfo.Tectonics = 1;
						solarSys->SysInfo.PlanetInfo.RotationPeriod = 4008;
						solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -167;
						break;
				}
				break;

			case 5: /* moon of SATURN: Titan */
				solarSys->SysInfo.PlanetInfo.PlanetToSunDist =
					EarthRadius * 952L / 100;
				solarSys->SysInfo.PlanetInfo.AtmoDensity = 160;
				solarSys->SysInfo.PlanetInfo.Weather = 2;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 34;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 40;
				solarSys->SysInfo.PlanetInfo.Tectonics = 1;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 3816;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -178;
				break;

			case 7: /* moon of NEPTUNE: Triton */
				solarSys->SysInfo.PlanetInfo.PlanetToSunDist =
					EarthRadius * 2999L / 100;
				solarSys->SysInfo.PlanetInfo.AtmoDensity = 10;
				solarSys->SysInfo.PlanetInfo.Weather = 1;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 95;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 27;
				solarSys->SysInfo.PlanetInfo.Tectonics = 0;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 4300;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -216;
				break;

			case 8: /* moon of Pluto: Charon */
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 1;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 9;
				solarSys->SysInfo.PlanetInfo.AtmoDensity = 0;
				solarSys->SysInfo.PlanetInfo.Weather = 0;
				solarSys->SysInfo.PlanetInfo.Tectonics = 0;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 1533;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -220;
				solarSys->SysInfo.PlanetInfo.PlanetToSunDist =
					EarthRadius * 3937L / 100;
				break;
		}

		solarSys->SysInfo.PlanetInfo.SurfaceGravity =
			CalcGravity(&solarSys->SysInfo.PlanetInfo);

		{ // For Sol Textures
			ResourceName maskAnim = nullptr;

			if (uqm::UQMOptions::read().sphereType)
			{
				switch (planetNr)
				{
					case 2: // moon of EARTH: LUNA
						if (moonNr == 1)
						{
							maskAnim = LunaMaskAnimRes;
						}
						break;
					case 4: // moons of JUPITER
						switch (moonNr)
						{
							case 0: // IO
								maskAnim = IoMaskAnimRes;
								break;
							case 1: // EUROPA
								maskAnim = EuropaMaskAnimRes;
								break;
							case 2: // GANYMEDE
								maskAnim = GanymedeMaskAnimRes;
								break;
							case 3: // CALLISTO
								maskAnim = CallistoMaskAnimRes;
								break;
						}
						break;
					case 5: // moon of SATURN: TITAN
						maskAnim = TitanMaskAnimRes;
						break;
					case 7: // moon of NEPTUNE : TRITON
						maskAnim = TritonMaskAnimRes;
						break;
					case 8: // moon of PLUTO: CHARON
						maskAnim = CharonMaskAnimRes;
						break;
				}
			}
			LoadPlanet(CaptureDrawable(LoadGraphic(maskAnim)));
		}
	}

	return true;
}

static uint16_t GenerateSol_generateMinerals(const SolarSystemState* solarSys,
											 const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{
	if (IsExtended() && matchWorld(solarSys, world, 8, 0))
	{
		/* Pluto's Moon, Charon */
		return CustomMineralDeposits(&solarSys->SysInfo, whichNode, info,
									 5, *ChemicalElement::CharonDust, *DepositSize::Light);
	}
	else if (!uqm::isOriginalSeed() && matchWorld(solarSys, world, 0, MatchPlanet))
	{
		/* Mercury */
		return CustomMineralDeposits(&solarSys->SysInfo, whichNode, info,
									 5, *ChemicalElement::RadioactiveCompounds, *DepositSize::Light);
	}
	else
	{
		return GenerateMineralDeposits(&solarSys->SysInfo, whichNode, info);
	}
}


static uint16_t GenerateSol_generateEnergy(const SolarSystemState* solarSys,
										   const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{
	if (matchWorld(solarSys, world, 8, MatchPlanet))
	{
		/* Pluto */
		// This check is needed because the retrieval bit is not set for
		// this node to keep it on the surface while the lander is taking
		// off
		if (uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI))
		{ // already picked up
			return 0;
		}

		if (info)
		{
			info->loc_pt.x = ResScale(20);
			info->loc_pt.y = MapHeight() - ResScale(8);
		}

		return 1; // only matters when count is requested
	}

	if (matchWorld(solarSys, world, 2, 1))
	{
		/* Earth Moon */
		// This check is redundant since the retrieval bit will keep the
		// node from showing up again
		if (uqm::gameState().flag(uqm::GameFlag::MOONBASE_DESTROYED))
		{ // already picked up
			return 0;
		}

		if (info)
		{
			info->loc_pt.x = ScaledMapWidth() * 3 / 4;
			info->loc_pt.y = MapHeight() * 1 / 4;
		}

		return 1; // only matters when count is requested
	}

	(void)whichNode;
	return 0;
}

static constexpr auto FwiffoFrags {8};

static bool GenerateSol_pickupEnergy(SolarSystemState* solarSys, SystemPlanet* world,
									 uint16_t whichNode)
{
	if (matchWorld(solarSys, world, 8, MatchPlanet))
	{ // Pluto
		assert(!uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI) && whichNode == 0);

		// Ran into Fwiffo on Pluto
		if (!KillLanderCrewSeq(FwiffoFrags, c_gameSecond / 20))
		{
			return false; // lander probably died
		}

		uqm::gameState().setFlag(uqm::GameFlag::FOUND_PLUTO_SPATHI, static_cast<uint32_t>(1));

		GenerateDefault_landerReport(solarSys);
		SetLanderTakeoff();

		// Do not remove the node from the surface while the lander is
		// taking off. FOUND_PLUTO_SPATHI bit will keep the node from
		// showing up on subsequent visits.
		return false;
	}

	if (matchWorld(solarSys, world, 2, 1))
	{ // Earth Moon
		assert(!uqm::gameState().flag(uqm::GameFlag::MOONBASE_DESTROYED) && whichNode == 0);

		GenerateDefault_landerReport(solarSys);

		if (getNomadMode() == uqm::NomadMode::Off)
		{
			SetLanderTakeoff();

			uqm::gameState().setFlag(uqm::GameFlag::MOONBASE_DESTROYED, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::MOONBASE_ON_SHIP, static_cast<uint32_t>(1));

			return true; // picked up
		}
		else
		{
			return false;
		}
	}

	(void)whichNode;
	return false;
}

static uint16_t GenerateSol_generateLife(const SolarSystemState* solarSys,
										 const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{
	if (matchWorld(solarSys, world, 2, 1))
	{
		/* Earth Moon */
		return GenerateRandomNodes(&solarSys->SysInfo, *ScanType::Biological,
								   10, BrainboxBulldozer, whichNode, info);
	}

	return 0;
}


static int init_probe(void)
{
	IpGroupHandle hGroup {};

	if (!uqm::gameState().flag(uqm::GameFlag::PROBE_MESSAGE_DELIVERED)
		&& GetGroupInfo(uqm::gameState().game().BattleGroupRef, GroupInitIp)
		&& (hGroup = GetHeadLink(&uqm::gameState().game().ip_group_q)))
	{
		InterplanetaryGroup* GroupPtr {LockIpGroup(&uqm::gameState().game().ip_group_q, hGroup)};
		GroupPtr->task = *GroupTask::InOrbit;
		GroupPtr->sys_loc = 2 + 1;	/* orbitting earth */
		GroupPtr->dest_loc = 2 + 1; /* orbitting earth */
		GroupPtr->loc.x = 0;
		GroupPtr->loc.y = 0;
		GroupPtr->group_counter = 0;
		UnlockIpGroup(&uqm::gameState().game().ip_group_q, hGroup);

		return 1;
	}
	else
	{
		return 0;
	}
}

static void check_probe(void)
{
	if (!uqm::gameState().game().BattleGroupRef)
	{
		return; // nothing to check
	}

	IpGroupHandle hGroup {GetHeadLink(&uqm::gameState().game().ip_group_q)};
	if (!hGroup)
	{
		return; // still nothing to check
	}

	InterplanetaryGroup* GroupPtr {LockIpGroup(&uqm::gameState().game().ip_group_q, hGroup)};
	// REFORM_GROUP was set in ipdisp.c:ip_group_collision()
	// during a collision with the flagship.
	if (GroupPtr->race_id == RaceId::UrquanDrone
		&& (GroupPtr->task & GroupTaskFlag::ReformGroup))
	{
		// We just want the probe to take off as fast as possible,
		// so clear out REFORM_GROUP
		GroupPtr->task = *GroupTask::Flee | GroupTaskFlag::IgnoreFlagship;
		GroupPtr->dest_loc = 0;
	}
	UnlockIpGroup(&uqm::gameState().game().ip_group_q, hGroup);
}

} // namespace uqm
