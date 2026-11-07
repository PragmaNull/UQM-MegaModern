// The prices, the battle group and the queues of a game, in the GameData struct.
#pragma once
#ifndef UQM_SESSION_GAMEDATA_H_
#define UQM_SESSION_GAMEDATA_H_

#include <cstdint>
#include "uqm/displist.h"
#include "uqm/sis.h"

struct GameData
{
	uint8_t CrewCost, FuelCost;
	uint8_t ModuleCost[*ModuleType::NumModules];
	uint8_t ElementWorth[*MineralCategory::NumElementCategories];

	uint32_t BattleGroupRef;
	uqm::LinkQueue avail_race_q;
	/* List of all the races in the game with information
			 * about their ships, and what player knows about their
			 * fleet, center of SoI, status, etc.
			 * queue Element is FleetInfo */
	uqm::LinkQueue npc_built_ship_q;
	/* Non-player-character list of ships (during Encounter)
			 * queue Element is ShipFragment */
	uqm::LinkQueue ip_group_q;
	/* List of groups present in solarsys (during IP);
			 * queue Element is InterplanetaryGroup */
	uqm::LinkQueue encounter_q;
	/* List of HyperSpace encounters (black globes);
			 * queue Element is Encounter */
	uqm::LinkQueue built_ship_q;
	/* List of SIS escort ships;
			 * queue Element is ShipFragment */
	uqm::LinkQueue stowed_ship_q;
	/* List of escort ships available at starbase;
			 * queue Element is ShipFragment */
};

#endif // UQM_SESSION_GAMEDATA_H_
