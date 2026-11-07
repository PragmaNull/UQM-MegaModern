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

#include "gendef.h"
#include "planets/generate.h"
#include "planets/generate/genall.h"


namespace uqm
{
const GenerateFunctions* getGenerateFunctions(uint8_t Index)
{
	switch (static_cast<PlotId>(Index))
	{
		case PlotId::Sol:
			return &generateSolFunctions;
		case PlotId::Shofixti:
			return &generateShofixtiFunctions;
		case PlotId::StartColony:
			return &generateColonyFunctions;
		case PlotId::Algolites:
		case PlotId::SpathiMonument:
		case PlotId::Spathi:
			return &generateSpathiFunctions;
		case PlotId::Melnorme0:
		case PlotId::Melnorme1:
		case PlotId::Melnorme2:
		case PlotId::Melnorme3:
		case PlotId::Melnorme4:
		case PlotId::Melnorme5:
		case PlotId::Melnorme6:
		case PlotId::Melnorme7:
		case PlotId::Melnorme8:
			return &generateMelnormeFunctions;
		case PlotId::TalkingPet:
			return &generateTalkingPetFunctions;
		case PlotId::MotherArk:
		case PlotId::Chmmr:
			return &generateChmmrFunctions;
		case PlotId::Syreen:
			return &generateSyreenFunctions;
		case PlotId::MyconTrap:
			return &generateTrapFunctions;
		case PlotId::Burvixese:
			return &generateBurvixeseFunctions;
		case PlotId::Slylandro:
			return &generateSlylandroFunctions;
		case PlotId::Druuge:
			return &generateDruugeFunctions;
		case PlotId::Bomb:
		case PlotId::Utwig:
			return &generateUtwigFunctions;
		case PlotId::AquaHelix:
		case PlotId::Thradd:
			return &generateThraddashFunctions;
		case PlotId::SunDevice:
		case PlotId::Mycon:
		case PlotId::EggCase0:
		case PlotId::EggCase1:
		case PlotId::EggCase2:
			return &generateMyconFunctions;
		case PlotId::Androsynth:
		case PlotId::ExcavationSite:
			return &generateAndrosynthFunctions;
		case PlotId::TaaloProtector:
		case PlotId::Orz:
			return &generateOrzFunctions;
		case PlotId::ShipVault:
			return &generateVaultFunctions;
		case PlotId::UrquanWreck:
			return &generateWreckFunctions;
		case PlotId::Maidens:
		case PlotId::VuxBeast:
		case PlotId::Vux:
			return &generateVuxFunctions;
		case PlotId::Urquan0:
		case PlotId::Urquan1:
		case PlotId::Urquan2:
		case PlotId::Kohrah0:
		case PlotId::Kohrah1:
		case PlotId::Kohrah2:
		case PlotId::DestroyedStarbase:
		case PlotId::Samatra:
			return &generateSaMatraFunctions;
		case PlotId::Zoqfot:
		case PlotId::ZoqColony0:
		case PlotId::ZoqColony1:
		case PlotId::ZoqColony2:
		case PlotId::ZoqColony3:
			return &generateZoqFotPikFunctions;
		case PlotId::ZoqScout:
			return &generateZoqFotPikScoutFunctions;
		case PlotId::Yehat:
			return &generateYehatFunctions;
		case PlotId::Pkunk:
			return &generatePkunkFunctions;
		case PlotId::Supox:
			return &generateSupoxFunctions;
		case PlotId::Rainbow0:
		case PlotId::Rainbow1:
		case PlotId::Rainbow2:
		case PlotId::Rainbow3:
		case PlotId::Rainbow4:
		case PlotId::Rainbow5:
		case PlotId::Rainbow6:
		case PlotId::Rainbow7:
		case PlotId::Rainbow8:
		case PlotId::Rainbow9:
			return &generateRainbowWorldFunctions;
		case PlotId::Ilwrath:
			return &generateIlwrathFunctions;
		default:
			return &generateDefaultFunctions;
	}
}

} // namespace uqm
