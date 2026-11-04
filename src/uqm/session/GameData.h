// The prices, the battle group and the queues of a game, in the GAME_STATE struct.
#pragma once
#ifndef UQM_SESSION_GAMEDATA_H_
#define UQM_SESSION_GAMEDATA_H_

#include <cstdint>
#include "uqm/displist.h"
#include "uqm/sis.h"

typedef struct
{
	uint8_t CrewCost, FuelCost;
	uint8_t ModuleCost[NUM_MODULES];
	uint8_t ElementWorth[NUM_ELEMENT_CATEGORIES];

	uint32_t BattleGroupRef;
	uqm::LinkQueue avail_race_q;
	/* List of all the races in the game with information
			 * about their ships, and what player knows about their
			 * fleet, center of SoI, status, etc.
			 * queue element is FLEET_INFO */
	uqm::LinkQueue npc_built_ship_q;
	/* Non-player-character list of ships (during encounter)
			 * queue element is SHIP_FRAGMENT */
	uqm::LinkQueue ip_group_q;
	/* List of groups present in solarsys (during IP);
			 * queue element is IP_GROUP */
	uqm::LinkQueue encounter_q;
	/* List of HyperSpace encounters (black globes);
			 * queue element is ENCOUNTER */
	uqm::LinkQueue built_ship_q;
	/* List of SIS escort ships;
			 * queue element is SHIP_FRAGMENT */
	uqm::LinkQueue stowed_ship_q;
	/* List of escort ships available at starbase;
			 * queue element is SHIP_FRAGMENT */
} GAME_STATE;

#endif // UQM_SESSION_GAMEDATA_H_
