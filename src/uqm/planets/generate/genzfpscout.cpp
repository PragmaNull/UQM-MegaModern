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

#include "genall.h"
#include "../../build.h"
#include "../../globdata.h"
#include "../../grpinfo.h"
#include "../../state.h"


static bool GenerateZoqFotPikScout_initNpcs(SolarSystemState* solarSys);
static bool GenerateZoqFotPikScout_reinitNpcs(SolarSystemState* solarSys);


const GenerateFunctions generateZoqFotPikScoutFunctions = {
	/* .initNpcs         = */ GenerateZoqFotPikScout_initNpcs,
	/* .reinitNpcs       = */ GenerateZoqFotPikScout_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateDefault_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateDefault_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateDefault_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateDefault_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool GenerateZoqFotPikScout_initNpcs(SolarSystemState* solarSys)
{
	if (!uqm::gameState().flag(uqm::GameFlag::MET_ZOQFOT))
	{
		uqm::gameState().game().BattleGroupRef = uqm::gameState().flag(uqm::GameFlag::ZOQFOT_GRPOFFS);
		if (uqm::gameState().game().BattleGroupRef == 0)
		{
			CloneShipFragment(RaceId::Zoqfotpik,
							  &uqm::gameState().game().npc_built_ship_q, 0);
			uqm::gameState().game().BattleGroupRef = PutGroupInfo(GroupsAddNew, 1);
			ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
			uqm::gameState().setFlag(uqm::GameFlag::ZOQFOT_GRPOFFS, static_cast<uint32_t>(uqm::gameState().game().BattleGroupRef));
		}
	}

	GenerateDefault_initNpcs(solarSys);

	return true;
}

static bool GenerateZoqFotPikScout_reinitNpcs(SolarSystemState* solarSys)
{
	GenerateDefault_reinitNpcs(solarSys);

	if (!uqm::gameState().game().BattleGroupRef)
	{
		return true; // nothing to check
	}

	HIPGROUP hGroup {GetHeadLink(&uqm::gameState().game().ip_group_q)};
	if (!hGroup)
	{
		return true; // still nothing to check
	}

	InterplanetaryGroup* GroupPtr {LockIpGroup(&uqm::gameState().game().ip_group_q, hGroup)};
	// REFORM_GROUP was set in ipdisp.c:ip_group_collision()
	// during a collision with the flagship.
	if (GroupPtr->race_id == RaceId::Zoqfotpik
		&& (GroupPtr->task & GroupTaskFlag::ReformGroup))
	{
		GroupPtr->task = *GroupTask::Flee | GroupTaskFlag::IgnoreFlagship | GroupTaskFlag::ReformGroup;
		GroupPtr->dest_loc = 0;
	}
	UnlockIpGroup(&uqm::gameState().game().ip_group_q, hGroup);

	return true;
}
