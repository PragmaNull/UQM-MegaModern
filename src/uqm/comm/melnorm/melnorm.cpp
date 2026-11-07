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

#include "../commall.h"
#include "core/log/log.h"
#include "core/types/enum.h"
#include "libs/inplib.h"
#include "libs/mathlib.h"
#include "resinst.h"
#include "strings.h"
#include "uqm/gameev.h"
#include "uqm/hyper.h"
#include "uqm/lua/UQMLua.h"
#include "uqm/shipcont.h"
// for SOL_X/SOL_Y
#include "uqm/planets/planets.h"
// for xxx_DISASTER
#include "../../gendef.h"
#include "../../starmap.h"
#include "uqm/controls.h"
#include "uqm/oscill.h"
#include "uqm/races.h"
#include "uqm/random/UQMRandom.h"
#include "uqm/sis.h"

namespace uqm
{
static uqstl::array<uint16_t, 10> melnorme_digit_names {
	*MelnormePhrase::EnumerateZero,
	*MelnormePhrase::EnumerateOne,
	*MelnormePhrase::EnumerateTwo,
	*MelnormePhrase::EnumerateThree,
	*MelnormePhrase::EnumerateFour,
	*MelnormePhrase::EnumerateFive,
	*MelnormePhrase::EnumerateSix,
	*MelnormePhrase::EnumerateSeven,
	*MelnormePhrase::EnumerateEight,
	*MelnormePhrase::EnumerateNine};

static uqstl::array<uint16_t, 10> melnorme_teen_names {
	*MelnormePhrase::EnumerateTen,
	*MelnormePhrase::EnumerateEleven,
	*MelnormePhrase::EnumerateTwelve,
	*MelnormePhrase::EnumerateThirteen,
	*MelnormePhrase::EnumerateFourteen,
	*MelnormePhrase::EnumerateFifteen,
	*MelnormePhrase::EnumerateSixteen,
	*MelnormePhrase::EnumerateSeventeen,
	*MelnormePhrase::EnumerateEighteen,
	*MelnormePhrase::EnumerateNineteen};

static uqstl::array<uint16_t, 10> melnorme_tens_names {
	0, /* invalid */
	0, /* skip digit */
	*MelnormePhrase::EnumerateTwenty,
	*MelnormePhrase::EnumerateThirty,
	*MelnormePhrase::EnumerateFourty,
	*MelnormePhrase::EnumerateFifty,
	*MelnormePhrase::EnumerateSixty,
	*MelnormePhrase::EnumerateSeventy,
	*MelnormePhrase::EnumerateEighty,
	*MelnormePhrase::EnumerateNinety};

const static NumberSpeechDesc melnorme_numbers_english =
	{
		5, /* NumDigits */
		{{{
			  /* 1000-999999 */
			  1000,								 /* Divider */
			  0,								 /* Subtrahend */
			  nullptr,							 /* StrDigits - recurse */
			  nullptr,							 /* Names - not used */
			  *MelnormePhrase::EnumerateThousand /* CommonIndex */
		  },
		  {
			  /* 100-999 */
			  100,								/* Divider */
			  0,								/* Subtrahend */
			  melnorme_digit_names.data(),		/* StrDigits */
			  nullptr,							/* Names - not used */
			  *MelnormePhrase::EnumerateHundred /* CommonIndex */
		  },
		  {
			  /* 20-99 */
			  10,						  /* Divider */
			  0,						  /* Subtrahend */
			  melnorme_tens_names.data(), /* StrDigits */
			  nullptr,					  /* Names - not used */
			  0							  /* CommonIndex - not used */
		  },
		  {
			  /* 10-19 */
			  1,						  /* Divider */
			  10,						  /* Subtrahend */
			  melnorme_teen_names.data(), /* StrDigits */
			  nullptr,					  /* Names - not used */
			  0							  /* CommonIndex - not used */
		  },
		  {
			  /* 0-9 */
			  1,						   /* Divider */
			  0,						   /* Subtrahend */
			  melnorme_digit_names.data(), /* StrDigits */
			  nullptr,					   /* Names - not used */
			  0							   /* CommonIndex - not used */
		  }}}};
static ConversationData melnorme_desc =
	{
		Conversation::Melnorme, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		MelnormePmapAnim, /* AlienFrame */
		MelnormeFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		MelnormeColorMap, /* AlienColorMap */
		MelnormeMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		MelnormeConversationPhrases, /* PlayerPhrases */
		5, /* NumAnimations */
		{{
			/* AlienAmbientArray (ambient animations) */
			{
				6,					  /* StartIndex */
				5,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 12,
				0, /* FrameRate */
				GameTicksPerSecond * 4,
				GameTicksPerSecond * 4, /* RestartRate */
				(1 << 1),				/* BlockMask */
			},
			{
				11,					  /* StartIndex */
				9,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond * 4,
				GameTicksPerSecond * 4, /* RestartRate */
				(1 << 0),				/* BlockMask */
			},
			{
				20,					  /* StartIndex */
				2,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				22,					  /* StartIndex */
				2,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				/* Flagship picture */
				24,													 /* StartIndex */
				1,													 /* NumFrames */
				AnimationFlags::Circular | AnimationFlags::Disabled, /* AnimFlags */
				0,
				0, /* FrameRate */
				0,
				0, /* RestartRate */
				0, /* BlockMask */
			},
		}},
		{
								/* AlienTransitionDesc */
			0,					  /* StartIndex */
			0,					  /* NumFrames */
			AnimationFlags::None, /* AnimFlags */
			0,
								0, /* FrameRate */
			0,
								0, /* RestartRate */
			0, /* BlockMask */
		},
		{
								/* AlienTalkDesc */
			1,					  /* StartIndex */
			5,					  /* NumFrames */
			AnimationFlags::None, /* AnimFlags */
			GameTicksPerSecond / 15,
								0, /* FrameRate */
			GameTicksPerSecond / 12,
								0, /* RestartRate */
			0, /* BlockMask */
		},
		&melnorme_numbers_english, /* AlienNumberSpeech - default */
		/* Filler for loaded resources */
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
};


//////////////Technology System///////////////////////
// This section deals with enabling and checking for
// various technologies.  It should probably be
// migrated to its own file.

// Identifiers for the various technologies
enum class TechId
{
	ModuleBlaster,
	LanderSpeed,
	ModuleAntimissile,
	LanderShieldBio,
	LanderCargo,
	ModuleBigfueltank,
	LanderRapidfire,
	LanderShieldQuake,
	ModuleTracking,
	LanderShieldLightning,
	LanderShieldHeat,
	ModuleCannon,
	ModuleFurnace,
};
consteval bool isArithmeticEnum(TechId) { return true; }

// Group the technologies into three subtypes
enum class TechType
{
	Module,		  // Flagship modules
				  //  subtype = moduleId, info = cost
				  //   Cost will be scaled by ModuleCostScale.
	LanderShield, // Lander shield enhancements
				  //  subtype = disaster type, info = unused
	State		  // Other game state changes
				  //  subtype = stateId, info = state value
};


// Define the information specifying a particular technology
struct TechData
{
	TechId id;	   // ID of the technology
	TechType type; // Type of the technology
	int subtype;   // Subtype of the technology
	int info;	   // Supplemental information
};


// A table of the available technologies.
// This should really be an associative map of TechIds to tech data records,
// but implementing that would be excessive.
const static uqstl::array<TechData, 13> tech_data_table {
	{
		// Tech ID                      Tech Type,               Supplemental info
		{TechId::ModuleBlaster, TechType::Module, *ModuleType::BlasterWeapon, 4000},
		{TechId::LanderSpeed, TechType::State, static_cast<int>(uqm::GameFlag::IMPROVED_LANDER_SPEED), 1},
		{TechId::ModuleAntimissile, TechType::Module, *ModuleType::AntimissileDefense, 4000},
		{TechId::LanderShieldBio, TechType::LanderShield, *LanderEvent::BiologicalDisaster, -1},
		{TechId::LanderCargo, TechType::State, static_cast<int>(uqm::GameFlag::IMPROVED_LANDER_CARGO), 1},
		{TechId::ModuleBigfueltank, TechType::Module, *ModuleType::HigheffFuelsys, 1000},
		{TechId::LanderRapidfire, TechType::State, static_cast<int>(uqm::GameFlag::IMPROVED_LANDER_SHOT), 1},
		{TechId::LanderShieldQuake, TechType::LanderShield, *LanderEvent::EarthquakeDisaster, -1},
		{TechId::ModuleTracking, TechType::Module, *ModuleType::TrackingSystem, 5000},
		{TechId::LanderShieldLightning, TechType::LanderShield, *LanderEvent::LightningDisaster, -1},
		{TechId::LanderShieldHeat, TechType::LanderShield, *LanderEvent::LavaspotDisaster, -1},
		{TechId::ModuleCannon, TechType::Module, *ModuleType::CannonWeapon, 6000},
		{TechId::ModuleFurnace, TechType::Module, *ModuleType::ShivaFurnace, 4000},
	 }
};
const size_t NUM_TECHNOLOGIES = std::size(tech_data_table);

// Lookup function to get the data for a particular tech
const static TechData* GetTechData(TechId techId)
{
	size_t i = 0;
	for (i = 0; i < NUM_TECHNOLOGIES; ++i)
	{
		if (tech_data_table[i].id == techId)
		{
			return &tech_data_table[i];
		}
	}
	return nullptr;
}


// 'stateId' is a uqm::GameFlag.
static bool HasStateTech(int stateId)
{
	return uqm::gameState().flag(static_cast<uqm::GameFlag>(stateId)) != 0;
}

static void GrantStateTech(int stateId, uint8_t value)
{
	uqm::gameState().setFlag(static_cast<uqm::GameFlag>(stateId), value);
}

static bool HasTech(TechId techId)
{
	const TechData* techData = GetTechData(techId);
	if (!techData)
	{
		return false;
	}

	switch (techData->type)
	{
		case TechType::Module:
			return uqm::gameState().game().ModuleCost[techData->subtype] != 0;
		case TechType::LanderShield:
			return (uqm::gameState().flag(uqm::GameFlag::LANDER_SHIELDS) & (1 << techData->subtype)) != 0;
		case TechType::State:
			return HasStateTech(techData->subtype);
	}
	return false;
}

static void GrantTech(TechId techId)
{
	const TechData* techData = GetTechData(techId);
	if (!techData)
	{
		return;
	}

	switch (techData->type)
	{
		case TechType::Module:
			uqm::gameState().game().ModuleCost[techData->subtype] = techData->info / ModuleCostScale;
			return;
		case TechType::LanderShield:
			{
				const uint16_t state = uqm::gameState().flag(uqm::GameFlag::LANDER_SHIELDS) | (1 << techData->subtype);
				uqm::gameState().setFlag(uqm::GameFlag::LANDER_SHIELDS, static_cast<uint32_t>(state));
				return;
			}
		case TechType::State:
			GrantStateTech(techData->subtype, techData->info);
			return;
	}
}

static int countTech(void)
{
	uint8_t numTech = 0;
	uint8_t i = 0;

	for (i = 0; i <= NUM_TECHNOLOGIES; ++i)
	{
		numTech += HasTech((TechId)i);
	}

	return numTech;
}


////////////Melnorme Sales System///////////
// This section contains code related to Melnorme sales

// Many of the conversation lines in strings.h fall into groups
// of sequential responses.  These structures allow those
// responses to be interated through.
const static uqstl::array<MelnormePhrase, 8> ok_buy_event_lines {
	MelnormePhrase::OkBuyEvent1, MelnormePhrase::OkBuyEvent2, MelnormePhrase::OkBuyEvent3, MelnormePhrase::OkBuyEvent4,
	MelnormePhrase::OkBuyEvent5, MelnormePhrase::OkBuyEvent6, MelnormePhrase::OkBuyEvent7, MelnormePhrase::OkBuyEvent8};
const size_t NUM_EVENT_ITEMS = std::size(ok_buy_event_lines);

const static uqstl::array<MelnormePhrase, 16> ok_buy_alien_race_lines {
	MelnormePhrase::OkBuyAlienRace1, MelnormePhrase::OkBuyAlienRace2, MelnormePhrase::OkBuyAlienRace3,
	MelnormePhrase::OkBuyAlienRace4, MelnormePhrase::OkBuyAlienRace5, MelnormePhrase::OkBuyAlienRace6,
	MelnormePhrase::OkBuyAlienRace7, MelnormePhrase::OkBuyAlienRace8, MelnormePhrase::OkBuyAlienRace9,
	MelnormePhrase::OkBuyAlienRace10, MelnormePhrase::OkBuyAlienRace11, MelnormePhrase::OkBuyAlienRace12,
	MelnormePhrase::OkBuyAlienRace13, MelnormePhrase::OkBuyAlienRace14, MelnormePhrase::OkBuyAlienRace15,
	MelnormePhrase::OkBuyAlienRace16};
const size_t NUM_ALIEN_RACE_ITEMS = std::size(ok_buy_alien_race_lines);

const static uqstl::array<MelnormePhrase, 9> ok_buy_history_lines {
	MelnormePhrase::OkBuyHistory1, MelnormePhrase::OkBuyHistory2, MelnormePhrase::OkBuyHistory3,
	MelnormePhrase::OkBuyHistory4, MelnormePhrase::OkBuyHistory5, MelnormePhrase::OkBuyHistory6,
	MelnormePhrase::OkBuyHistory7, MelnormePhrase::OkBuyHistory8, MelnormePhrase::OkBuyHistory9};
const size_t NUM_HISTORY_ITEMS = std::size(ok_buy_history_lines);

const static uqstl::array<MelnormePhrase, 10> hello_and_down_to_business_lines {
	MelnormePhrase::HelloAndDownToBusiness1, MelnormePhrase::HelloAndDownToBusiness2,
	MelnormePhrase::HelloAndDownToBusiness3, MelnormePhrase::HelloAndDownToBusiness4,
	MelnormePhrase::HelloAndDownToBusiness5, MelnormePhrase::HelloAndDownToBusiness6,
	MelnormePhrase::HelloAndDownToBusiness7, MelnormePhrase::HelloAndDownToBusiness8,
	MelnormePhrase::HelloAndDownToBusiness9, MelnormePhrase::HelloAndDownToBusiness10};
const size_t NUM_HELLO_LINES = std::size(hello_and_down_to_business_lines);

const static uqstl::array<MelnormePhrase, 6> rescue_lines {
	MelnormePhrase::RescueExplanation, MelnormePhrase::RescueAgain1, MelnormePhrase::RescueAgain2,
	MelnormePhrase::RescueAgain3, MelnormePhrase::RescueAgain4, MelnormePhrase::RescueAgain5};
const size_t NUM_RESCUE_LINES = std::size(rescue_lines);

// How many lines are available in the given array?
static size_t
GetNumLines(const MelnormePhrase array[])
{
	if (array == ok_buy_event_lines.data())
	{
		return NUM_EVENT_ITEMS;
	}
	else if (array == ok_buy_alien_race_lines.data())
	{
		return NUM_ALIEN_RACE_ITEMS;
	}
	else if (array == ok_buy_history_lines.data())
	{
		return NUM_HISTORY_ITEMS;
	}
	else if (array == hello_and_down_to_business_lines.data())
	{
		return NUM_HELLO_LINES;
	}
	else if (array == rescue_lines.data())
	{
		return NUM_RESCUE_LINES;
	}
	return 0;
}

// Get the line, with range checking.
// Returns the last line if the desired one is out of range.
static MelnormePhrase GetLineSafe(const MelnormePhrase array[], size_t linenum)
{
	const size_t array_size = GetNumLines(array);
	assert(array_size > 0);
	if (linenum >= array_size)
	{
		linenum = array_size - 1;
	}
	return array[linenum];
}

// Data structure to hold the Melnorme's info on a technology
struct TechSaleData
{
	TechId techId;			  // ID of technology
	int price;				  // Melnorme's price to sell
	MelnormePhrase sale_line; // Sales pitch line ID
	MelnormePhrase sold_line; // Post-sale line ID
};

// Right now, all techs have the same price.
// Unless Hard Mode is enabled.
static constexpr auto Techprice {75 * BioCreditValueDefault};

uqstl::array<TechSaleData, 13> tech_sale_catalog {
	{
		{TechId::ModuleBlaster, Techprice, MelnormePhrase::NewTech1, MelnormePhrase::OkBuyNewTech1},
		{TechId::LanderSpeed, Techprice, MelnormePhrase::NewTech2, MelnormePhrase::OkBuyNewTech2},
		{TechId::ModuleAntimissile, Techprice, MelnormePhrase::NewTech3, MelnormePhrase::OkBuyNewTech3},
		{TechId::LanderShieldBio, Techprice, MelnormePhrase::NewTech4, MelnormePhrase::OkBuyNewTech4},
		{TechId::LanderCargo, Techprice, MelnormePhrase::NewTech5, MelnormePhrase::OkBuyNewTech5},
		{TechId::ModuleBigfueltank, Techprice, MelnormePhrase::NewTech6, MelnormePhrase::OkBuyNewTech6},
		{TechId::LanderRapidfire, Techprice, MelnormePhrase::NewTech7, MelnormePhrase::OkBuyNewTech7},
		{TechId::LanderShieldQuake, Techprice, MelnormePhrase::NewTech8, MelnormePhrase::OkBuyNewTech8},
		{TechId::ModuleTracking, Techprice, MelnormePhrase::NewTech9, MelnormePhrase::OkBuyNewTech9},
		{TechId::LanderShieldLightning, Techprice, MelnormePhrase::NewTech10, MelnormePhrase::OkBuyNewTech10},
		{TechId::LanderShieldHeat, Techprice, MelnormePhrase::NewTech11, MelnormePhrase::OkBuyNewTech11},
		{TechId::ModuleCannon, Techprice, MelnormePhrase::NewTech12, MelnormePhrase::OkBuyNewTech12},
		{TechId::ModuleFurnace, Techprice, MelnormePhrase::NewTech13, MelnormePhrase::OkBuyNewTech13},
	 }
};
const size_t NUM_TECH_ITEMS = std::size(tech_sale_catalog);

// Return the next tech for sale that the player doesn't already have.
// Returns nullptr if the player has all the techs.
static TechSaleData* GetNextTechForSale(void)
{
	uint8_t i = 0;
	uint8_t j = 0;

	if (isDifficulty(uqm::Difficulty::Hard) && CurStarDescPtr())
	{
		switch (static_cast<PlotId>(CurStarDescPtr()->Index))
		{
			case PlotId::Melnorme0:
				i = *TechId::ModuleCannon;
				j = i + 1;
				break;
			case PlotId::Melnorme1:
				i = *TechId::ModuleBlaster;
				j = i + 1;
				break;
			case PlotId::Melnorme2:
				i = *TechId::LanderShieldBio;
				j = i + 2; // TECH_LANDER_CARGO
				break;
			case PlotId::Melnorme3:
				i = *TechId::LanderRapidfire;
				j = i + 2; // TECH_LANDER_SHIELD_QUAKE
				break;
			case PlotId::Melnorme4:
				i = *TechId::ModuleBigfueltank;
				j = i + 1;
				break;
			case PlotId::Melnorme5:
				i = *TechId::LanderSpeed;
				j = i + 2; // TECH_MODULE_ANTIMISSILE
				break;
			case PlotId::Melnorme6:
				i = *TechId::ModuleTracking;
				j = i + 1;
				break;
			case PlotId::Melnorme7:
				i = *TechId::LanderShieldLightning;
				j = i + 2; // TECH_LANDER_SHIELD_HEAT
				break;
			case PlotId::Melnorme8:
				i = *TechId::ModuleFurnace;
				j = i + 1;
				break;
			default:
				i = 0;
				j = i;
		}

		for (i = i; i < j; ++i)
		{
			if (!HasTech(tech_sale_catalog[i].techId))
			{
				return &tech_sale_catalog[i];
			}
		}
	}
	else if (isDifficulty(uqm::Difficulty::Hard) && !CurStarDescPtr())
	{
		return nullptr;
	}
	else
	{
		for (i = 0; i < NUM_TECH_ITEMS; ++i)
		{
			if (!HasTech(tech_sale_catalog[i].techId))
			{
				return &tech_sale_catalog[i];
			}
		}
	}
	return nullptr;
}

///////////End Melnorme Sales Section//////////////////

static StatMsgMode prevMsgMode;

static void DoFirstMeeting(ResponseRef R);

static uint16_t ShipWorth(void)
{
	uint8_t i {};
	int8_t crew_pods {};
	uint16_t worth {};

	worth = uqm::gameState().sis().NumLanders
		  * uqm::gameState().game().ModuleCost[*ModuleType::PlanetLander];
	for (i = 0; i < NumDriveSlots; ++i)
	{
		if (uqm::gameState().sis().DriveSlots[i] < EmptySlot)
		{
			worth += uqm::gameState().game().ModuleCost[*ModuleType::FusionThruster];
		}
	}
	for (i = 0; i < NumJetSlots; ++i)
	{
		if (uqm::gameState().sis().JetSlots[i] < EmptySlot)
		{
			worth += uqm::gameState().game().ModuleCost[*ModuleType::TurningJets];
		}
	}

	crew_pods = -(int8_t)((uqm::gameState().sis().CrewEnlisted + CrewPodCapacity - 1)
						  / CrewPodCapacity);
	for (i = 0; i < NumModuleSlots; ++i)
	{
		const uint8_t which_module {*uqm::gameState().sis().ModuleSlots[i]};
		if (which_module < ModuleType::BombModule0
			&& (which_module != ModuleType::CrewPod || ++crew_pods > 0))
		{
			worth += uqm::gameState().game().ModuleCost[which_module];
		}
	}

	return (worth);
}

static uint16_t rescue_fuel;
static FlagshipData SIS_copy;

// Extract method to return the response string index
// for stripping a given module.
static MelnormePhrase GetStripModuleRef(int moduleID)
{
	switch (static_cast<ModuleType>(moduleID))
	{
		case ModuleType::PlanetLander:
			return MelnormePhrase::Landers;
		case ModuleType::FusionThruster:
			return MelnormePhrase::Thrusters;
		case ModuleType::TurningJets:
			return MelnormePhrase::Jets;
		case ModuleType::CrewPod:
			return MelnormePhrase::Pods;
		case ModuleType::StorageBay:
			return MelnormePhrase::Bays;
		case ModuleType::DynamoUnit:
			return MelnormePhrase::Dynamos;
		case ModuleType::ShivaFurnace:
			return MelnormePhrase::Furnaces;
		case ModuleType::GunWeapon:
			return MelnormePhrase::Guns;
		case ModuleType::BlasterWeapon:
			return MelnormePhrase::Blasters;
		case ModuleType::CannonWeapon:
			return MelnormePhrase::Cannons;
		case ModuleType::TrackingSystem:
			return MelnormePhrase::Trackers;
		case ModuleType::AntimissileDefense:
			return MelnormePhrase::Defenses;
		// If a modder has added new modules, should it really
		// be a fatal error if the Melnorme don't know about
		// them?
		default:
			assert(0 && "Unknown module");
	}
	return MelnormePhrase::NullPhrase;
}

static uint32_t getStripRandomSeed(void)
{
	uint32_t x {};
	uint32_t y {};
	// We truncate the location because encounters move the ship slightly in
	// HSpace, and throw some other relatively immutable values in the mix to
	// vary the deal when stuck at the same general location again.
	// It is still possible but unlikely for encounters to move the ship into
	// another truncation sector so the player could choose from 2 deals.
	x = LogxToUniverse(uqm::gameState().sis().log_x) / 100;
	y = LogyToUniverse(uqm::gameState().sis().log_y) / 100;
	// prime numbers help randomness
	return y * 1013 + x + uqm::gameState().sis().NumLanders
		 + *uqm::gameState().sis().ModuleSlots[1] + *uqm::gameState().sis().ModuleSlots[4]
		 + *uqm::gameState().sis().ModuleSlots[7] + *uqm::gameState().sis().ModuleSlots[10];
}

static bool StripShip(uint16_t fuel_required)
{
	uint8_t i {};
	uint8_t which_module {};
	int8_t crew_pods {};

	uqm::gameState().setFlag(uqm::GameFlag::MELNORME_RESCUE_REFUSED, static_cast<uint32_t>(0));

	crew_pods = -(int8_t)((uqm::gameState().sis().CrewEnlisted + CrewPodCapacity - 1)
						  / CrewPodCapacity);
	if (fuel_required == 0)
	{
		uqm::gameState().sis() = SIS_copy;
		DeltaSISGauges(UndefinedDelta, rescue_fuel, UndefinedDelta);
	}
	else if (fuel_required == (uint16_t)~0)
	{
		uqm::gameState().sis().NumLanders = 0;
		for (i = 0; i < NumDriveSlots; ++i)
		{
			uqm::gameState().sis().DriveSlots[i] = ModuleType::EmptyDriveSlot;
		}
		for (i = 0; i < NumJetSlots; ++i)
		{
			uqm::gameState().sis().JetSlots[i] = ModuleType::EmptyJetSlot;
		}
		if (uqm::gameState().sis().FuelOnBoard > FuelReserve)
		{
			uqm::gameState().sis().FuelOnBoard = FuelReserve;
		}
		uqm::gameState().sis().TotalBioMass = 0;
		uqm::gameState().sis().TotalElementMass = 0;
		for (i = 0; i < MineralCategory::NumElementCategories; ++i)
		{
			uqm::gameState().sis().ElementAmounts[i] = 0;
		}
		for (i = 0; i < NumModuleSlots; ++i)
		{
			which_module = *uqm::gameState().sis().ModuleSlots[i];
			if (which_module < ModuleType::BombModule0
				&& (which_module != ModuleType::CrewPod
					|| ++crew_pods > 0))
			{
				uqm::gameState().sis().ModuleSlots[i] = ModuleType::EmptyModuleSlot;
			}
		}

		DeltaSISGauges(UndefinedDelta, UndefinedDelta, UndefinedDelta);
	}
	else if (fuel_required)
	{
		int8_t bays {};
		uint8_t beg_mod {};
		uint8_t end_mod {};
		uint16_t worth {};
		uqstl::array<uint8_t, *ModuleType::BombModule0> module_count;

		// Bug #567
		// In order to offer the same deal each time if it is refused, we seed
		// the random number generator with our location, thus making the deal
		// a repeatable pseudo-random function of where we got stuck and what,
		// exactly, is on our ship.
		uqm::UQMRandom rc {};
		rc.reseed(getStripRandomSeed());

		SIS_copy = uqm::gameState().sis();
		for (i = *ModuleType::PlanetLander; i < ModuleType::BombModule0; ++i)
		{
			module_count[i] = 0;
		}

		uint32_t capacity {FuelReserve};
		uint8_t slot {NumModuleSlots - 1};
		do
		{
			if (SIS_copy.ModuleSlots[slot] == ModuleType::FuelTank
				|| SIS_copy.ModuleSlots[slot] == ModuleType::HigheffFuelsys)
			{
				uint16_t volume {};

				volume = SIS_copy.ModuleSlots[slot] == ModuleType::FuelTank ? FuelTankCapacity : HefuelTankCapacity;
				capacity += volume;
			}
		} while (slot--);
		if (fuel_required > capacity)
		{
			fuel_required = capacity;
		}

		bays = -(int8_t)((SIS_copy.TotalElementMass + StorageBayCapacity - 1)
						 / StorageBayCapacity);
		for (i = 0; i < NumModuleSlots; ++i)
		{
			which_module = *SIS_copy.ModuleSlots[i];
			if (which_module == ModuleType::CrewPod)
			{
				++crew_pods;
			}
			else if (which_module == ModuleType::StorageBay)
			{
				++bays;
			}
		}

		worth = fuel_required / FuelTankScale;
		uint16_t total {0};
		uint8_t num_searches {0};
		beg_mod = end_mod = (uint8_t)~0;
		while (total < worth && ShipWorth() && ++num_searches)
		{
			const uint32_t rand_val {rc.next()};
			switch (static_cast<ModuleType>(which_module = uqm::lowByte(uqm::lowWord(rand_val)) % (*ModuleType::CrewPod + 1)))
			{
				case ModuleType::PlanetLander:
					if (SIS_copy.NumLanders == 0)
					{
						continue;
					}
					--SIS_copy.NumLanders;
					break;
				case ModuleType::FusionThruster:
					for (i = 0; i < NumDriveSlots; ++i)
					{
						if (SIS_copy.DriveSlots[i] < EmptySlot)
						{
							break;
						}
					}
					if (i == NumDriveSlots)
					{
						continue;
					}
					SIS_copy.DriveSlots[i] = ModuleType::EmptyDriveSlot;
					break;
				case ModuleType::TurningJets:
					for (i = 0; i < NumJetSlots; ++i)
					{
						if (SIS_copy.JetSlots[i] < EmptySlot)
						{
							break;
						}
					}
					if (i == NumJetSlots)
					{
						continue;
					}
					SIS_copy.JetSlots[i] = ModuleType::EmptyJetSlot;
					break;
				case ModuleType::CrewPod:
					i = uqm::highByte(uqm::lowWord(rand_val)) % NumModuleSlots;
					which_module = *SIS_copy.ModuleSlots[i];
					if (which_module >= ModuleType::BombModule0
						|| which_module == ModuleType::FuelTank
						|| which_module == ModuleType::HigheffFuelsys
						|| (which_module == ModuleType::StorageBay
							&& module_count[*ModuleType::StorageBay] >= bays)
						|| (which_module == ModuleType::CrewPod
							&& module_count[*ModuleType::CrewPod] >= crew_pods))
					{
						continue;
					}
					SIS_copy.ModuleSlots[i] = ModuleType::EmptyModuleSlot;
					break;
				default:
					break;
			}

			if (beg_mod == (uint8_t)~0)
			{
				beg_mod = end_mod = which_module;
			}
			else if (which_module > end_mod)
			{
				end_mod = which_module;
			}
			++module_count[which_module];
			total += uqm::gameState().game().ModuleCost[which_module];
		}

		if (total == 0)
		{
			NPCPhrase(MelnormePhrase::Charity);
			DeltaSISGauges(0, fuel_required, 0);
			return (false);
		}
		else
		{
			NPCPhrase(MelnormePhrase::RescueOffer);
			rescue_fuel = fuel_required;
			if (rescue_fuel == capacity)
			{
				NPCPhrase(MelnormePhrase::RescueTanks);
			}
			else
			{
				NPCPhrase(MelnormePhrase::RescueHome);
			}
			for (i = *ModuleType::PlanetLander; i < ModuleType::BombModule0; ++i)
			{
				if (module_count[i])
				{
					if (i == end_mod && i != beg_mod)
					{
						NPCPhrase(MelnormePhrase::EndListWithAnd);
					}
					NPCPhrase(MelnormePhrase::EnumerateOne + (module_count[i] - 1));
					NPCPhrase(GetStripModuleRef(i));
				}
			}
		}
	}

	return (true);
}

static void ExitConversation(ResponseRef R)
{
	if (PlayerSaid(R, MelnormePhrase::NoTradeNow))
	{
		NPCPhrase(MelnormePhrase::OkNoTradeNowBye);
	}
	else if (PlayerSaid(R, MelnormePhrase::YoureOn))
	{
		NPCPhrase(MelnormePhrase::YouGiveUsNoChoice);

		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_ANGER, static_cast<uint32_t>(1));
		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(R, MelnormePhrase::SoWeCanAttack))
	{
		NPCPhrase(MelnormePhrase::DeceitfulHuman);

		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_ANGER, static_cast<uint32_t>(2));
		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(R, MelnormePhrase::ByeMelnormeSlightlyAngry))
	{
		NPCPhrase(MelnormePhrase::MelnormeSlightlyAngryGoodbye);
	}
	else if (PlayerSaid(R, MelnormePhrase::OkStripMe))
	{
		if (ShipWorth() < 4000 / ModuleCostScale)
		{
			/* is ship worth stripping */
			NPCPhrase(MelnormePhrase::NotWorthStripping);
		}
		else
		{
			uqm::gameState().setFlag(uqm::GameFlag::MELNORME_ANGER, static_cast<uint32_t>(0));

			StripShip((uint16_t)~0);
			NPCPhrase(MelnormePhrase::FairJustice);
		}
	}
	else if (PlayerSaid(R, MelnormePhrase::FightSomeMore))
	{
		NPCPhrase(MelnormePhrase::OkFightSomeMore);

		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_ANGER, static_cast<uint32_t>(3));
		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(R, MelnormePhrase::ByeMelnormePissedOff))
	{
		NPCPhrase(MelnormePhrase::MelnormePissedOffGoodbye);
	}
	else if (PlayerSaid(R, MelnormePhrase::WellIfThatsTheWayYouFeel))
	{
		NPCPhrase(MelnormePhrase::WeFightAgain);

		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(R, MelnormePhrase::YouHateUsSoWeGoAway))
	{
		NPCPhrase(MelnormePhrase::HateYouGoodbye);
	}
	else if (PlayerSaid(R, MelnormePhrase::TakeIt))
	{
		StripShip(0);
		NPCPhrase(MelnormePhrase::HappyToHaveRescued);
	}
	else if (PlayerSaid(R, MelnormePhrase::LeaveIt))
	{
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_RESCUE_REFUSED, static_cast<uint32_t>(1));
		NPCPhrase(MelnormePhrase::MaybeSeeYouLater);
	}
	else if (PlayerSaid(R, MelnormePhrase::NoHelp))
	{
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_RESCUE_REFUSED, static_cast<uint32_t>(1));
		NPCPhrase(MelnormePhrase::GoodbyeAndGoodluck);
	}
	else if (PlayerSaid(R, MelnormePhrase::NoChangedMind))
	{
		NPCPhrase(MelnormePhrase::GoodbyeAndGoodluckAgain);
	}
	else if (PlayerSaid(R, MelnormePhrase::BeLeavingNow)
			 || PlayerSaid(R, MelnormePhrase::Goodbye))
	{
		NPCPhrase(MelnormePhrase::FriendlyGoodbye);
	}
}

static void DoRescue(ResponseRef R)
{
	int16_t dx {};
	int16_t dy {};
	uint16_t fuel_required {};

	(void)R; // ignored
	// JSD Replace old method for locating SOL with plot method
	//dx = LogxToUniverse (GLOBAL_SIS (log_x))
	//		- SOL_X;
	//dy = LogyToUniverse (GLOBAL_SIS (log_y))
	//		- SOL_Y;
	dx = LogxToUniverse(uqm::gameState().sis().log_x)
	   - plot_map[*PlotId::Sol].star_pt.x;
	dy = LogyToUniverse(uqm::gameState().sis().log_y)
	   - plot_map[*PlotId::Sol].star_pt.y;
	fuel_required = square_root(
						(uint32_t)((long)dx * dx + (long)dy * dy))
				  + (2 * FuelTankScale);

	if (StripShip(fuel_required))
	{
		Response(MelnormePhrase::TakeIt, ExitConversation);
		Response(MelnormePhrase::LeaveIt, ExitConversation);
	}
}

// Extract method for getting the player's current credits.
static uint16_t GetAvailableCredits(void)
{
	return uqm::makeWord(uqm::gameState().flag(uqm::GameFlag::MELNORME_CREDIT0),
						 uqm::gameState().flag(uqm::GameFlag::MELNORME_CREDIT1));
}

// Extract method for setting the player's current credits.
static void SetAvailableCredits(uint16_t credits)
{
	uqm::gameState().setFlag(uqm::GameFlag::MELNORME_CREDIT0, static_cast<uint32_t>(uqm::lowByte(credits)));
	uqm::gameState().setFlag(uqm::GameFlag::MELNORME_CREDIT1, static_cast<uint32_t>(uqm::highByte(credits)));
}

// Now returns whether the purchase succeeded instead of the remaining
// credit balance.  Use GetAvailableCredits() to get the latter.
static bool DeltaCredit(int16_t delta_credit)
{
	uint16_t Credit = GetAvailableCredits();

	// Can they afford it?
	if ((int)delta_credit >= 0 || ((int)(-delta_credit) <= (int)(Credit)))
	{
		Credit += delta_credit;
		SetAvailableCredits(Credit);
		DrawStatusMessage(nullptr);
		return true;
	}

	// Fail
	NPCPhrase(MelnormePhrase::NeedMoreCredit0);
	NPCNumber(-delta_credit - Credit, nullptr);
	NPCPhrase(MelnormePhrase::NeedMoreCredit1);

	return false;
}


// Extract methods to process the giving of various bits of information to the
// player.  Ideally, we'd want to merge these three into a single parameterized
// function, but the nature of the XXX_GAME_STATE() code makes that tricky.
static void CurrentEvents(void)
{
	const uint8_t stack = uqm::gameState().flag(uqm::GameFlag::MELNORME_EVENTS_INFO_STACK);
	const MelnormePhrase phraseId {GetLineSafe(ok_buy_event_lines.data(), stack)};
	switch (phraseId)
	{
		case MelnormePhrase::OkBuyEvent1:
			SetHomeworldKnown(*HomeId::Shofixti);
			break;
		case MelnormePhrase::OkBuyEvent6:
			SetHomeworldKnown(*HomeId::Slylandro);
			break;
		default:
			break;
	}
	NPCPhrase(phraseId);
	uqm::gameState().setFlag(uqm::GameFlag::MELNORME_EVENTS_INFO_STACK, static_cast<uint32_t>(stack + 1));
}

static void AlienRaces(void)
{
	const uint8_t stack = uqm::gameState().flag(uqm::GameFlag::MELNORME_ALIEN_INFO_STACK);
	const MelnormePhrase phraseId {GetLineSafe(ok_buy_alien_race_lines.data(), stack)};
	// Two pieces of alien knowledge trigger state changes.
	switch (phraseId)
	{
		case MelnormePhrase::OkBuyAlienRace5:
			SetHomeworldKnown(*HomeId::Druuge);
			break;
		case MelnormePhrase::OkBuyAlienRace7:
			SetHomeworldKnown(*HomeId::Thraddash);
			break;
		case MelnormePhrase::OkBuyAlienRace8:
			SetHomeworldKnown(*HomeId::Chmmr);
			break;
		case MelnormePhrase::OkBuyAlienRace12:
			SetHomeworldKnown(*HomeId::Shofixti);
			break;
		case MelnormePhrase::OkBuyAlienRace13:
			SetHomeworldKnown(*HomeId::Slylandro);
			break;
		case MelnormePhrase::OkBuyAlienRace14:
			if (!uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI))
			{
				uqm::gameState().setFlag(uqm::GameFlag::KNOW_SPATHI_PASSWORD, static_cast<uint32_t>(1));
				uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(7));
			}
			SetHomeworldKnown(*HomeId::Spathi);
			break;
		case MelnormePhrase::OkBuyAlienRace15:
			if (uqm::gameState().flag(uqm::GameFlag::KNOW_ABOUT_SHATTERED) < 2)
			{
				uqm::gameState().setFlag(uqm::GameFlag::KNOW_ABOUT_SHATTERED, static_cast<uint32_t>(2));
			}
			uqm::gameState().setFlag(uqm::GameFlag::KNOW_SYREEN_WORLD_SHATTERED, static_cast<uint32_t>(1));
			SetHomeworldKnown(*HomeId::Syreen);
			break;
		case MelnormePhrase::OkBuyAlienRace16:
			SetHomeworldKnown(*HomeId::Yehat);
			break;
		default:
			break;
	}
	NPCPhrase(phraseId);
	uqm::gameState().setFlag(uqm::GameFlag::MELNORME_ALIEN_INFO_STACK, static_cast<uint32_t>(stack + 1));
}

static void History(void)
{
	const uint8_t stack = uqm::gameState().flag(uqm::GameFlag::MELNORME_HISTORY_INFO_STACK);
	const MelnormePhrase phraseId {GetLineSafe(ok_buy_history_lines.data(), stack)};
	NPCPhrase(phraseId);
	uqm::gameState().setFlag(uqm::GameFlag::MELNORME_HISTORY_INFO_STACK, static_cast<uint32_t>(stack + 1));
}

// extract method to tell if we have any information left to sell to the player.
static bool AnyInfoLeftToSell(void)
{
	return uqm::gameState().flag(uqm::GameFlag::MELNORME_EVENTS_INFO_STACK) < NUM_EVENT_ITEMS
		|| uqm::gameState().flag(uqm::GameFlag::MELNORME_ALIEN_INFO_STACK) < NUM_ALIEN_RACE_ITEMS
		|| uqm::gameState().flag(uqm::GameFlag::MELNORME_HISTORY_INFO_STACK) < NUM_HISTORY_ITEMS;
}

static void NatureOfConversation(ResponseRef R);

static uint8_t AskedToBuy;


static constexpr auto InfoCost {75};

static void DoBuy(ResponseRef R)
{
	int16_t needed_credit {};
	const uint8_t FuelCost = BioCreditValueDefault / 2;

	const uint16_t credit {GetAvailableCredits()};

	uint32_t capacity {FuelReserve};
	uint8_t slot {NumModuleSlots - 1};
	do
	{
		if (uqm::gameState().sis().ModuleSlots[slot] == ModuleType::FuelTank
			|| uqm::gameState().sis().ModuleSlots[slot] == ModuleType::HigheffFuelsys)
		{
			uint16_t volume {};

			volume = uqm::gameState().sis().ModuleSlots[slot] == ModuleType::FuelTank ? FuelTankCapacity : HefuelTankCapacity;
			capacity += volume;
		}
	} while (slot--);

	// If they're out of credits, educate them on how commerce works.
	if (credit == 0 && !uqm::UQMOptions::read().infiniteCredits)
	{
		AskedToBuy = true;
		NPCPhrase(MelnormePhrase::NeedCredit);

		NatureOfConversation(R);
	}
	else if (PlayerSaid(R, MelnormePhrase::BuyFuel)
			 || PlayerSaid(R, MelnormePhrase::Buy1Fuel)
			 || PlayerSaid(R, MelnormePhrase::Buy5Fuel)
			 || PlayerSaid(R, MelnormePhrase::Buy10Fuel)
			 || PlayerSaid(R, MelnormePhrase::Buy25Fuel)
			 || PlayerSaid(R, MelnormePhrase::FillMeUp))
	{
		needed_credit = 0;
		if (PlayerSaid(R, MelnormePhrase::Buy1Fuel))
		{
			needed_credit = 1;
		}
		else if (PlayerSaid(R, MelnormePhrase::Buy5Fuel))
		{
			needed_credit = 5;
		}
		else if (PlayerSaid(R, MelnormePhrase::Buy10Fuel))
		{
			needed_credit = 10;
		}
		else if (PlayerSaid(R, MelnormePhrase::Buy25Fuel))
		{
			needed_credit = 25;
		}
		else if (PlayerSaid(R, MelnormePhrase::FillMeUp))
		{
			const int16_t remainingCapacity = (capacity - uqm::gameState().sis().FuelOnBoard
											   + FuelTankScale - 1)
											/ FuelTankScale;

			if (credit < remainingCapacity && !uqm::UQMOptions::read().infiniteCredits)
			{
				needed_credit = credit;
			}
			else
			{
				needed_credit = remainingCapacity;
			}
		}

		if (needed_credit == 0)
		{
			if (!uqm::gameState().flag(uqm::GameFlag::MELNORME_FUEL_PROCEDURE))
			{
				NPCPhrase(MelnormePhrase::BuyFuelIntro);
				uqm::gameState().setFlag(uqm::GameFlag::MELNORME_FUEL_PROCEDURE, static_cast<uint32_t>(1));
			}
		}
		else
		{
			if (uqm::gameState().sis().FuelOnBoard / FuelTankScale
					+ needed_credit
				> capacity / FuelTankScale)
			{
				NPCPhrase(MelnormePhrase::NoRoomForFuel);
				goto TryFuelAgain;
			}

			if (((int)(needed_credit * FuelCost) <= (int)credit)
				|| uqm::UQMOptions::read().infiniteCredits)
			{
				if (IsExtended() && PlayerSaid(R, MelnormePhrase::FillMeUp))
				{
					NPCPhrase(MelnormePhrase::OkFillYouUp);
				}

				NPCPhrase(MelnormePhrase::GotFuel);

				uint32_t f {(uint32_t)needed_credit * FuelTankScale};
				while (f > 0x3FFFL)
				{
					DeltaSISGauges(0, 0x3FFF, 0);
					f -= 0x3FFF;
				}
				DeltaSISGauges(0, f, 0);
			}
			needed_credit *= FuelCost;
		}
		if (needed_credit)
		{
			if (!uqm::UQMOptions::read().infiniteCredits)
			{
				DeltaCredit(-needed_credit);
			}
			if (uqm::gameState().sis().FuelOnBoard >= capacity)
			{
				goto BuyBuyBuy;
			}
		}
TryFuelAgain:
		NPCPhrase(MelnormePhrase::HowMuchFuel);

		Response(MelnormePhrase::Buy1Fuel, DoBuy);
		Response(MelnormePhrase::Buy5Fuel, DoBuy);
		Response(MelnormePhrase::Buy10Fuel, DoBuy);
		Response(MelnormePhrase::Buy25Fuel, DoBuy);
		Response(MelnormePhrase::FillMeUp, DoBuy);
		Response(MelnormePhrase::DoneBuyingFuel, DoBuy);
	}
	else if (PlayerSaid(R, MelnormePhrase::BuyTechnology)
			 || PlayerSaid(R, MelnormePhrase::BuyNewTech))
	{
		// Note that this code no longer uses the MELNORME_TECH_STACK state
		// buts, as they're not needed; we can tell what technologies the
		// player has by using the technology API above.  This opens the
		// possibility of the player acquiring tech from someplace other than
		// the Melnorme.
		TechSaleData* nextTech {};

		// If it's our first time, give an introduction.
		if (!uqm::gameState().flag(uqm::GameFlag::MELNORME_TECH_PROCEDURE))
		{
			NPCPhrase(MelnormePhrase::BuyNewTechIntro);
			uqm::gameState().setFlag(uqm::GameFlag::MELNORME_TECH_PROCEDURE, static_cast<uint32_t>(1));
		}

		// Did the player just attempt to buy a tech?
		if (PlayerSaid(R, MelnormePhrase::BuyNewTech))
		{
			nextTech = GetNextTechForSale();
			if (!nextTech)
			{
				goto BuyBuyBuy; // No tech left to buy
			}

			if (!uqm::UQMOptions::read().infiniteCredits && !DeltaCredit(-nextTech->price))
			{
				goto BuyBuyBuy; // Can't afford it
			}

			// Make the sale
			GrantTech(nextTech->techId);
			NPCPhrase(nextTech->sold_line);
		}

		nextTech = GetNextTechForSale();
		if (!nextTech)
		{
			NPCPhrase(MelnormePhrase::NewTechAllGone);
			goto BuyBuyBuy; // No tech left to buy
		}

		NPCPhrase(nextTech->sale_line);

		if (isDifficulty(uqm::Difficulty::Hard) && countTech())
		{
			nextTech->price = Techprice + countTech() * 50;
			NPCPhrase(MelnormePhrase::NeedMoreCredit0);
			NPCNumber(nextTech->price - Techprice, nullptr);
			NPCPhrase(MelnormePhrase::NeedMoreCredit1);
		}

		Response(MelnormePhrase::BuyNewTech, DoBuy);
		Response(MelnormePhrase::NoBuyNewTech, DoBuy);
	}
	else if (PlayerSaid(R, MelnormePhrase::BuyInfo)
			 || PlayerSaid(R, MelnormePhrase::BuyCurrentEvents)
			 || PlayerSaid(R, MelnormePhrase::BuyAlienRaces)
			 || PlayerSaid(R, MelnormePhrase::BuyHistory))
	{
		if (!uqm::gameState().flag(uqm::GameFlag::MELNORME_INFO_PROCEDURE))
		{
			NPCPhrase(MelnormePhrase::BuyInfoIntro);
			uqm::gameState().setFlag(uqm::GameFlag::MELNORME_INFO_PROCEDURE, static_cast<uint32_t>(1));
		}
		else if (PlayerSaid(R, MelnormePhrase::BuyInfo))
		{
			NPCPhrase(MelnormePhrase::OkBuyInfo);
		}
		else
		{
			if (!uqm::UQMOptions::read().infiniteCredits && !DeltaCredit(-InfoCost))
			{
				goto BuyBuyBuy;
			}

			if (PlayerSaid(R, MelnormePhrase::BuyCurrentEvents))
			{
				CurrentEvents();
			}
			else if (PlayerSaid(R, MelnormePhrase::BuyAlienRaces))
			{
				AlienRaces();
			}
			else if (PlayerSaid(R, MelnormePhrase::BuyHistory))
			{
				History();
			}
		}

		if (!AnyInfoLeftToSell())
		{
			NPCPhrase(MelnormePhrase::InfoAllGone);
			goto BuyBuyBuy;
		}

		if (uqm::gameState().flag(uqm::GameFlag::MELNORME_EVENTS_INFO_STACK) < NUM_EVENT_ITEMS)
		{
			Response(MelnormePhrase::BuyCurrentEvents, DoBuy);
		}
		if (uqm::gameState().flag(uqm::GameFlag::MELNORME_ALIEN_INFO_STACK) < NUM_ALIEN_RACE_ITEMS)
		{
			Response(MelnormePhrase::BuyAlienRaces, DoBuy);
		}
		if (uqm::gameState().flag(uqm::GameFlag::MELNORME_HISTORY_INFO_STACK) < NUM_HISTORY_ITEMS)
		{
			Response(MelnormePhrase::BuyHistory, DoBuy);
		}
		Response(MelnormePhrase::DoneBuyingInfo, DoBuy);
	}
	else
	{
		if (PlayerSaid(R, MelnormePhrase::DoneBuyingFuel))
		{
			NPCPhrase(MelnormePhrase::OkDoneBuyingFuel);
		}
		else if (PlayerSaid(R, MelnormePhrase::NoBuyNewTech))
		{
			NPCPhrase(MelnormePhrase::OkNoBuyNewTech);
		}
		else if (PlayerSaid(R, MelnormePhrase::DoneBuyingInfo))
		{
			NPCPhrase(MelnormePhrase::OkDoneBuyingInfo);
		}
		else
		{
			NPCPhrase(MelnormePhrase::WhatToBuy);
		}

BuyBuyBuy:
		if (uqm::gameState().sis().FuelOnBoard < capacity)
		{
			Response(MelnormePhrase::BuyFuel, DoBuy);
		}
		if (GetNextTechForSale())
		{
			Response(MelnormePhrase::BuyTechnology, DoBuy);
		}
		if (AnyInfoLeftToSell())
		{
			Response(MelnormePhrase::BuyInfo, DoBuy);
		}

		Response(MelnormePhrase::DoneBuying, NatureOfConversation);
		Response(MelnormePhrase::BeLeavingNow, ExitConversation);
	}
}

static void DoSell(ResponseRef R)
{
	uint8_t num_new_rainbows {};
	uint16_t rainbow_mask {};
	int16_t added_credit {};
	int what_to_sell_queued = 0;
	const uint8_t BioCreditValue = ifHardDifficulty(1, BioCreditValueDefault);

	rainbow_mask = uqm::makeWord(
		uqm::gameState().flag(uqm::GameFlag::RAINBOW_WORLD0),
		uqm::gameState().flag(uqm::GameFlag::RAINBOW_WORLD1));
	num_new_rainbows = -(int8_t)(uqm::gameState().flag(uqm::GameFlag::MELNORME_RAINBOW_COUNT));
	while (rainbow_mask)
	{
		if (rainbow_mask & 1)
		{
			++num_new_rainbows;
		}

		rainbow_mask >>= 1;
	}

	if (!PlayerSaid(R, MelnormePhrase::Sell))
	{
		if (PlayerSaid(R, MelnormePhrase::SellLifeData))
		{
			TimeCount TimeIn;
			const int16_t beast_value = 0;
			bool Sleepy = true;

			if (IsExtended() && uqm::gameState().flag(uqm::GameFlag::VUX_BEAST_ON_SHIP) == 2
				&& uqm::gameState().flag(uqm::GameFlag::VUX_BEAST) == 2)
			{
				uqm::gameState().setFlag(uqm::GameFlag::VUX_BEAST_ON_SHIP, static_cast<uint32_t>(0));
			}

			added_credit = uqm::gameState().sis().TotalBioMass * BioCreditValue + beast_value;

			NPCPhrase(MelnormePhrase::SoldLifeData1);
			NPCNumber(uqm::gameState().sis().TotalBioMass, nullptr);
			NPCPhrase(MelnormePhrase::SoldLifeData2);
			NPCNumber(added_credit, nullptr);
			NPCPhrase(MelnormePhrase::SoldLifeData3);
			// queue MelnormePhrase::WhatToSell before talk-segue
			if (num_new_rainbows)
			{
				NPCPhrase(MelnormePhrase::WhatToSell);
				what_to_sell_queued = 1;
			}

			AlienTalkSegue(1);

			FlushInput();

			DrawCargoStrings((uint8_t)~0, (uint8_t)~0);
			TimeIn = GetTimeCounter() + c_gameSecond / 2;
			while (GetTimeCounter() < TimeIn && Sleepy)
			{
				UpdateDuty(true);
				Sleepy = !(AnyButtonPress(true) || uqm::currentActivity() & ActivityFlag::CheckAbort);
			}
			DrawCargoStrings(
				(uint8_t)MineralCategory::NumElementCategories,
				(uint8_t)MineralCategory::NumElementCategories);

			while (uqm::gameState().sis().TotalBioMass && Sleepy)
			{
				Sleepy = !(AnyButtonPress(true)
						   || uqm::currentActivity() & ActivityFlag::CheckAbort);

				UpdateDuty(true);

				--uqm::gameState().sis().TotalBioMass;
				TaskSwitch();
				DeltaCredit(BioCreditValue);
				DrawCargoStrings(
					(uint8_t)MineralCategory::NumElementCategories,
					(uint8_t)MineralCategory::NumElementCategories);
			}

			if (!Sleepy && uqm::gameState().sis().TotalBioMass)
			{
				DeltaCredit(
					uqm::gameState().sis().TotalBioMass
					* BioCreditValue);
				uqm::gameState().sis().TotalBioMass = 0;
			}
			else
			{
				TimeIn = GetTimeCounter() + c_gameSecond / 2;
				while (GetTimeCounter() < TimeIn && Sleepy)
				{
					UpdateDuty(true);
					Sleepy = !(AnyButtonPress(true) || uqm::currentActivity() & ActivityFlag::CheckAbort);
				}
			}
			ClearSISRect(ClearSisFlags::DrawDisplay);
		}
		else /* if (R == MelnormePhrase::SellRainbowLocations) */
		{
			TimeCount TimeIn;
			bool Sleepy = true;
			const int diffCase = difficultyCase(500, 200, 125);

			added_credit = num_new_rainbows * (diffCase * BioCreditValue);

			NPCPhrase(MelnormePhrase::SoldRainbowLocations1);
			NPCNumber(num_new_rainbows, nullptr);
			NPCPhrase(MelnormePhrase::SoldRainbowLocations2);
			NPCNumber(added_credit, nullptr);
			NPCPhrase(MelnormePhrase::SoldRainbowLocations3);

			uint8_t planets {num_new_rainbows};
			num_new_rainbows += uqm::gameState().flag(uqm::GameFlag::MELNORME_RAINBOW_COUNT);
			uqm::gameState().setFlag(uqm::GameFlag::MELNORME_RAINBOW_COUNT, static_cast<uint32_t>(num_new_rainbows));
			num_new_rainbows = 0;

			if (!IsExtended())
			{
				DeltaCredit(added_credit);
			}
			else
			{
				// queue MelnormePhrase::WhatToSell before talk-segue
				if (uqm::gameState().sis().TotalBioMass)
				{
					NPCPhrase(MelnormePhrase::WhatToSell);
					what_to_sell_queued = 1;
				}

				AlienTalkSegue(1);

				FlushInput();

				DrawRainbowPlanet(planets);
				TimeIn = GetTimeCounter() + c_gameSecond / 2;
				while (GetTimeCounter() < TimeIn && Sleepy)
				{
					UpdateDuty(true);
					Sleepy = !(AnyButtonPress(true) || uqm::currentActivity() & ActivityFlag::CheckAbort);
				}

				DrawRainbowPlanet(planets);

				TimeIn = GetTimeCounter() + c_gameSecond / 10;

				while (planets != 0 && Sleepy)
				{
					Sleepy = !(AnyButtonPress(true)
							   || uqm::currentActivity() & ActivityFlag::CheckAbort);

					UpdateDuty(true);

					if (GetTimeCounter() > TimeIn)
					{
						--planets;
						DeltaCredit(diffCase * BioCreditValue);
						DrawRainbowPlanet(planets);
						TimeIn = GetTimeCounter() + c_gameSecond / 6;
					}
				}

				if (!Sleepy && planets != 0)
				{
					DeltaCredit(planets * diffCase * BioCreditValue);
					planets = 0;
				}
				else
				{
					TimeIn = GetTimeCounter() + c_gameSecond / 2;
					while (GetTimeCounter() < TimeIn && Sleepy)
					{
						UpdateDuty(true);
						Sleepy = !(AnyButtonPress(true) || uqm::currentActivity() & ActivityFlag::CheckAbort);
					}
				}
				ClearSISRect(ClearSisFlags::DrawDisplay);
			}
		}

		AskedToBuy = false;
	}

	if (uqm::gameState().sis().TotalBioMass || num_new_rainbows)
	{
		if (!what_to_sell_queued)
		{
			NPCPhrase(MelnormePhrase::WhatToSell);
		}

		if (uqm::gameState().sis().TotalBioMass)
		{
			Response(MelnormePhrase::SellLifeData, DoSell);
		}
		if (num_new_rainbows)
		{
			Response(MelnormePhrase::SellRainbowLocations, DoSell);
		}
		Response(MelnormePhrase::DoneSelling, NatureOfConversation);
	}
	else
	{
		if (PlayerSaid(R, MelnormePhrase::Sell))
		{
			NPCPhrase(MelnormePhrase::NothingToSell);
		}
		DisablePhrase(MelnormePhrase::Sell);

		NatureOfConversation(R);
	}
}


static void NatureOfConversation(ResponseRef R)
{
	uint8_t num_new_rainbows {};
	uint16_t rainbow_mask {};

	if (PlayerSaid(R, MelnormePhrase::GetOnWithBusiness))
	{
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_YACK_STACK2, static_cast<uint32_t>(5));
		R = 0;
	}

	// Draw credits display
	DeltaCredit(0);
	const uint16_t Credit {GetAvailableCredits()};
	if (R == 0)
	{
		uint8_t stack = uqm::gameState().flag(uqm::GameFlag::MELNORME_YACK_STACK2) - 5;
		NPCPhrase(GetLineSafe(hello_and_down_to_business_lines.data(), stack));
		if (stack < (NUM_HELLO_LINES - 1))
		{
			++stack;
		}
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_YACK_STACK2, static_cast<uint32_t>(stack + 5));
	}

	rainbow_mask = uqm::makeWord(
		uqm::gameState().flag(uqm::GameFlag::RAINBOW_WORLD0),
		uqm::gameState().flag(uqm::GameFlag::RAINBOW_WORLD1));
	num_new_rainbows = -(int8_t)(uqm::gameState().flag(uqm::GameFlag::MELNORME_RAINBOW_COUNT));
	while (rainbow_mask)
	{
		if (rainbow_mask & 1)
		{
			++num_new_rainbows;
		}

		rainbow_mask >>= 1;
	}

	if (uqm::gameState().sis().FuelOnBoard > 0
		|| uqm::gameState().sis().TotalBioMass
		|| Credit
		|| num_new_rainbows
		|| uqm::UQMOptions::read().infiniteCredits)
	{
		if (!uqm::gameState().flag(uqm::GameFlag::TRADED_WITH_MELNORME))
		{
			uqm::gameState().setFlag(uqm::GameFlag::TRADED_WITH_MELNORME, static_cast<uint32_t>(1));

			NPCPhrase(MelnormePhrase::TradingInfo);
		}

		if (R == 0)
		{
			/* Melnorme reports any news and turns purple */
			NPCPhrase(MelnormePhrase::BuyOrSell);
			AlienTalkSegue(1);

			XFormColorMap(GetColorMapAddress(CommData.AlienColorMap, 1),
						  c_gameSecond / 2);

			AlienTalkSegue((uint16_t)~0);
		}
		else if (PlayerSaid(R, MelnormePhrase::WhyTurnedPurple))
		{
			uqm::gameState().setFlag(uqm::GameFlag::WHY_MELNORME_PURPLE, static_cast<uint32_t>(1));

			NPCPhrase(MelnormePhrase::TurnedPurpleBecause);
		}
		else if (PlayerSaid(R, MelnormePhrase::DoneSelling))
		{
			NPCPhrase(MelnormePhrase::OkDoneSelling);
		}
		else if (PlayerSaid(R, MelnormePhrase::DoneBuying))
		{
			NPCPhrase(MelnormePhrase::OkDoneBuying);
		}

		if (!uqm::gameState().flag(uqm::GameFlag::WHY_MELNORME_PURPLE) && !uqm::UQMOptions::read().infiniteCredits)
		{
			Response(MelnormePhrase::WhyTurnedPurple, NatureOfConversation);
		}
		if (!AskedToBuy)
		{
			Response(MelnormePhrase::Buy, DoBuy);
		}
		if (PhraseEnabled(MelnormePhrase::Sell))
		{
			Response(MelnormePhrase::Sell, DoSell);
		}
		Response(MelnormePhrase::Goodbye, ExitConversation);
	}
	else /* needs to be rescued */
	{
		if (uqm::gameState().flag(uqm::GameFlag::MELNORME_RESCUE_REFUSED))
		{
			NPCPhrase(MelnormePhrase::ChangedMind);

			Response(MelnormePhrase::YesChangedMind, DoRescue);
			Response(MelnormePhrase::NoChangedMind, ExitConversation);
		}
		else
		{
			uint8_t num_rescues = uqm::gameState().flag(uqm::GameFlag::MELNORME_RESCUE_COUNT);
			NPCPhrase(GetLineSafe(rescue_lines.data(), num_rescues));

			if (num_rescues < NUM_RESCUE_LINES - 1)
			{
				++num_rescues;
				uqm::gameState().setFlag(uqm::GameFlag::MELNORME_RESCUE_COUNT, static_cast<uint32_t>(num_rescues));
			}

			NPCPhrase(MelnormePhrase::ShouldWeHelpYou);

			Response(MelnormePhrase::YesHelp, DoRescue);
			Response(MelnormePhrase::NoHelp, ExitConversation);
		}
	}
}

static uint8_t local_stack0, local_stack1;

static void DoBluster(ResponseRef R)
{
	if (PlayerSaid(R, MelnormePhrase::TradeIsForTheWeak))
	{
		NPCPhrase(MelnormePhrase::WereNotAfraid);
		AlienTalkSegue((uint16_t)~0);

		XFormColorMap(GetColorMapAddress(CommData.AlienColorMap, 2),
					  c_gameSecond / 2);

		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_YACK_STACK2, static_cast<uint32_t>(4));
	}
	else if (PlayerSaid(R, MelnormePhrase::WhyBlueLight))
	{
		uqm::gameState().setFlag(uqm::GameFlag::WHY_MELNORME_BLUE, static_cast<uint32_t>(1));

		NPCPhrase(MelnormePhrase::BlueIsMad);
	}
	else if (PlayerSaid(R, MelnormePhrase::WeStrong1))
	{
		local_stack0 = 1;
		NPCPhrase(MelnormePhrase::YouNotStrong1);
	}
	else if (PlayerSaid(R, MelnormePhrase::WeStrong2))
	{
		local_stack0 = 2;
		NPCPhrase(MelnormePhrase::YouNotStrong2);
	}
	else if (PlayerSaid(R, MelnormePhrase::WeStrong3))
	{
		local_stack0 = 3;
		NPCPhrase(MelnormePhrase::YouNotStrong3);
	}
	else if (PlayerSaid(R, MelnormePhrase::JustTesting))
	{
		local_stack1 = 1;
		NPCPhrase(MelnormePhrase::ReallyTesting);
	}

	if (!uqm::gameState().flag(uqm::GameFlag::WHY_MELNORME_BLUE))
	{
		Response(MelnormePhrase::WhyBlueLight, DoBluster);
	}
	switch (local_stack0)
	{
		case 0:
			Response(MelnormePhrase::WeStrong1, DoBluster);
			break;
		case 1:
			Response(MelnormePhrase::WeStrong2, DoBluster);
			break;
		case 2:
			Response(MelnormePhrase::WeStrong3, DoBluster);
			break;
	}
	switch (local_stack1)
	{
		case 0:
			Response(MelnormePhrase::JustTesting, DoBluster);
			break;
		case 1:
			{
				Response(MelnormePhrase::YesReallyTesting, DoFirstMeeting);
				break;
			}
	}
	Response(MelnormePhrase::YoureOn, ExitConversation);
}

static void yack0_respond(void)
{

	switch (uqm::gameState().flag(uqm::GameFlag::MELNORME_YACK_STACK0))
	{
		case 0:
			Response(MelnormePhrase::WeAreFromAlliance, DoFirstMeeting);
			break;
		case 1:
			Response(MelnormePhrase::HowKnow, DoFirstMeeting);
			break;
	}
}

static void yack1_respond(void)
{
	switch (uqm::gameState().flag(uqm::GameFlag::MELNORME_YACK_STACK1))
	{
		case 0:
			Response(MelnormePhrase::WhatAboutYourselves, DoFirstMeeting);
			break;
		case 1:
			Response(MelnormePhrase::WhatFactors, DoFirstMeeting);
			FALLTHROUGH;
		case 2:
			Response(MelnormePhrase::GetOnWithBusiness, NatureOfConversation);
			break;
	}
}

static void yack2_respond(void)
{
	switch (uqm::gameState().flag(uqm::GameFlag::MELNORME_YACK_STACK2))
	{
		case 0:
			Response(MelnormePhrase::WhatAboutUniverse, DoFirstMeeting);
			break;
		case 1:
			Response(MelnormePhrase::GivingIsGood1, DoFirstMeeting);
			break;
		case 2:
			Response(MelnormePhrase::GivingIsGood2, DoFirstMeeting);
			break;
		case 3:
			Response(MelnormePhrase::TradeIsForTheWeak, DoBluster);
			break;
	}
}

static void DoFirstMeeting(ResponseRef R)
{
	uint8_t last_stack = 0;
	using VoidFunc = void (*)(void);
	VoidFunc temp_func {}, stack_func[] =
							   {
								   yack0_respond,
								   yack1_respond,
								   yack2_respond,
							   };

	if (R == 0)
	{
		uint8_t business_count {};

		business_count = uqm::gameState().flag(uqm::GameFlag::MELNORME_BUSINESS_COUNT);
		switch (business_count++)
		{
			case 0:
				NPCPhrase(MelnormePhrase::HelloNowDownToBusiness1);
				break;
			case 1:
				NPCPhrase(MelnormePhrase::HelloNowDownToBusiness2);
				break;
			case 2:
				NPCPhrase(MelnormePhrase::HelloNowDownToBusiness3);
				--business_count;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_BUSINESS_COUNT, static_cast<uint32_t>(business_count));
	}
	else if (PlayerSaid(R, MelnormePhrase::WeAreFromAlliance))
	{
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_YACK_STACK0, static_cast<uint32_t>(1));
		NPCPhrase(MelnormePhrase::KnowOfYou);
	}
	else if (PlayerSaid(R, MelnormePhrase::HowKnow))
	{
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_YACK_STACK0, static_cast<uint32_t>(2));
		NPCPhrase(MelnormePhrase::KnowBecause);
	}
	else if (PlayerSaid(R, MelnormePhrase::WhatAboutYourselves))
	{
		last_stack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_YACK_STACK1, static_cast<uint32_t>(1));
		NPCPhrase(MelnormePhrase::NoTalkAboutOurselves);
	}
	else if (PlayerSaid(R, MelnormePhrase::WhatFactors))
	{
		last_stack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_YACK_STACK1, static_cast<uint32_t>(2));
		NPCPhrase(MelnormePhrase::FactorsAre);
	}
	else if (PlayerSaid(R, MelnormePhrase::WhatAboutUniverse))
	{
		last_stack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_YACK_STACK2, static_cast<uint32_t>(1));
		NPCPhrase(MelnormePhrase::NoFreeLunch);
	}
	else if (PlayerSaid(R, MelnormePhrase::GivingIsGood1))
	{
		last_stack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_YACK_STACK2, static_cast<uint32_t>(2));
		NPCPhrase(MelnormePhrase::GivingIsBad1);
	}
	else if (PlayerSaid(R, MelnormePhrase::GivingIsGood2))
	{
		last_stack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_YACK_STACK2, static_cast<uint32_t>(3));
		NPCPhrase(MelnormePhrase::GivingIsBad2);
	}
	else if (PlayerSaid(R, MelnormePhrase::YesReallyTesting))
	{
		NPCPhrase(MelnormePhrase::TestResults);
		AlienTalkSegue((uint16_t)~0);

		XFormColorMap(GetColorMapAddress(CommData.AlienColorMap, 0),
					  c_gameSecond / 2);
	}
	else if (PlayerSaid(R, MelnormePhrase::WeApologize))
	{
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_ANGER, static_cast<uint32_t>(0));
		NPCPhrase(MelnormePhrase::ApologyAccepted);
		AlienTalkSegue((uint16_t)~0);

		XFormColorMap(GetColorMapAddress(CommData.AlienColorMap, 0),
					  c_gameSecond / 2);
	}

	temp_func = stack_func[0];
	stack_func[0] = stack_func[last_stack];
	stack_func[last_stack] = temp_func;
	(*stack_func[0])();
	(*stack_func[1])();
	(*stack_func[2])();
	Response(MelnormePhrase::NoTradeNow, ExitConversation);
}

static void DoMelnormeMiffed(ResponseRef R)
{
	if (R == 0)
	{
		uint8_t miffed_count {};

		miffed_count = uqm::gameState().flag(uqm::GameFlag::MELNORME_MIFFED_COUNT);
		switch (miffed_count++)
		{
			case 0:
				NPCPhrase(MelnormePhrase::HelloSlightlyAngry1);
				break;
			case 1:
				NPCPhrase(MelnormePhrase::HelloSlightlyAngry2);
				break;
			default:
				--miffed_count;
				NPCPhrase(MelnormePhrase::HelloSlightlyAngry3);
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_MIFFED_COUNT, static_cast<uint32_t>(miffed_count));

		AlienTalkSegue((uint16_t)~0);

		XFormColorMap(GetColorMapAddress(CommData.AlienColorMap, 2),
					  c_gameSecond / 2);
	}
	else if (PlayerSaid(R, MelnormePhrase::ExploreRelationship))
	{
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_YACK_STACK3, static_cast<uint32_t>(1));

		NPCPhrase(MelnormePhrase::ExampleOfRelationship);
	}
	else if (PlayerSaid(R, MelnormePhrase::Excuse1))
	{
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_YACK_STACK3, static_cast<uint32_t>(2));

		NPCPhrase(MelnormePhrase::NoExcuse1);
	}
	else if (PlayerSaid(R, MelnormePhrase::Excuse2))
	{
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_YACK_STACK3, static_cast<uint32_t>(3));

		NPCPhrase(MelnormePhrase::NoExcuse2);
	}
	else if (PlayerSaid(R, MelnormePhrase::Excuse3))
	{
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_YACK_STACK3, static_cast<uint32_t>(4));

		NPCPhrase(MelnormePhrase::NoExcuse3);
	}

	switch (uqm::gameState().flag(uqm::GameFlag::MELNORME_YACK_STACK3))
	{
		case 0:
			Response(MelnormePhrase::ExploreRelationship, DoMelnormeMiffed);
			break;
		case 1:
			Response(MelnormePhrase::Excuse1, DoMelnormeMiffed);
			break;
		case 2:
			Response(MelnormePhrase::Excuse2, DoMelnormeMiffed);
			break;
		case 3:
			Response(MelnormePhrase::Excuse3, DoMelnormeMiffed);
			break;
	}
	Response(MelnormePhrase::WeApologize, DoFirstMeeting);
	Response(MelnormePhrase::SoWeCanAttack, ExitConversation);
	Response(MelnormePhrase::ByeMelnormeSlightlyAngry, ExitConversation);
}

static void DoMelnormePissed(ResponseRef R)
{
	if (R == 0)
	{
		uint8_t pissed_count {};

		pissed_count = uqm::gameState().flag(uqm::GameFlag::MELNORME_PISSED_COUNT);
		switch (pissed_count++)
		{
			case 0:
				NPCPhrase(MelnormePhrase::HelloPissedOff1);
				break;
			case 1:
				NPCPhrase(MelnormePhrase::HelloPissedOff2);
				break;
			default:
				--pissed_count;
				NPCPhrase(MelnormePhrase::HelloPissedOff3);
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_PISSED_COUNT, static_cast<uint32_t>(pissed_count));

		AlienTalkSegue((uint16_t)~0);

		XFormColorMap(GetColorMapAddress(CommData.AlienColorMap, 2),
					  c_gameSecond / 2);
	}
	else if (PlayerSaid(R, MelnormePhrase::BegForgiveness))
	{
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_YACK_STACK4, static_cast<uint32_t>(1));

		NPCPhrase(MelnormePhrase::LotsToMakeUpFor);
	}
	else if (PlayerSaid(R, MelnormePhrase::YouAreSoRight))
	{
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_YACK_STACK4, static_cast<uint32_t>(2));

		NPCPhrase(MelnormePhrase::OneLastChance);
	}

	switch (uqm::gameState().flag(uqm::GameFlag::MELNORME_YACK_STACK4))
	{
		case 0:
			Response(MelnormePhrase::BegForgiveness, DoMelnormePissed);
			break;
		case 1:
			Response(MelnormePhrase::YouAreSoRight, DoMelnormePissed);
			break;
		case 2:
			Response(MelnormePhrase::OkStripMe, ExitConversation);
			break;
	}
	Response(MelnormePhrase::FightSomeMore, ExitConversation);
	Response(MelnormePhrase::ByeMelnormePissedOff, ExitConversation);
}

static void DoMelnormeHate(ResponseRef R)
{
	uint8_t hate_count {};

	(void)R; // ignored
	hate_count = uqm::gameState().flag(uqm::GameFlag::MELNORME_HATE_COUNT);
	switch (hate_count++)
	{
		case 0:
			NPCPhrase(MelnormePhrase::HelloHateYou1);
			break;
		case 1:
			NPCPhrase(MelnormePhrase::HelloHateYou2);
			break;
		default:
			--hate_count;
			NPCPhrase(MelnormePhrase::HelloHateYou3);
			break;
	}
	uqm::gameState().setFlag(uqm::GameFlag::MELNORME_HATE_COUNT, static_cast<uint32_t>(hate_count));

	AlienTalkSegue((uint16_t)~0);

	XFormColorMap(GetColorMapAddress(CommData.AlienColorMap, 2),
				  c_gameSecond / 2);

	Response(MelnormePhrase::WellIfThatsTheWayYouFeel, ExitConversation);
	Response(MelnormePhrase::YouHateUsSoWeGoAway, ExitConversation);
}

static void Intro(void)
{
	prevMsgMode = SetStatusMessageMode(StatMsgMode::Credits);

	if (IsHd())
	{
		CommData.AlienAmbientArray[2].AnimFlags |= AnimationFlags::TriggerFullRedraw;
		CommData.AlienAmbientArray[3].AnimFlags |= AnimationFlags::TriggerFullRedraw;
		CommData.AlienAmbientArray[4].AnimFlags &= ~AnimationFlags::Circular;
	}

	if (uqm::gameState().flag(uqm::GameFlag::MET_MELNORME) == 0)
	{
		uqm::gameState().setFlag(uqm::GameFlag::MET_MELNORME, static_cast<uint32_t>(1));
		DoFirstMeeting(0);
	}
	else
	{
		switch (uqm::gameState().flag(uqm::GameFlag::MELNORME_ANGER))
		{
			case 0:
				if (uqm::gameState().flag(uqm::GameFlag::MELNORME_YACK_STACK2) <= 5)
				{
					DoFirstMeeting(0);
				}
				else
				{
					NatureOfConversation(0);
				}
				break;
			case 1:
				DoMelnormeMiffed(0);
				break;
			case 2:
				DoMelnormePissed(0);
				break;
			default:
				DoMelnormeHate(0);
				break;
		}
	}
}

static uint16_t uninit_melnorme(void)
{
	uqm::lua().endComm();
	return 0;
}

static void post_melnorme_enc(void)
{
	if (prevMsgMode != StatMsgMode::Undefined)
	{
		SetStatusMessageMode(prevMsgMode);
	}
	DrawStatusMessage(nullptr);
}

ConversationData* init_melnorme_comm(void)
{
	melnorme_desc.init_encounter_func = Intro;
	melnorme_desc.post_encounter_func = post_melnorme_enc;
	melnorme_desc.uninit_encounter_func = uninit_melnorme;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	melnorme_desc.AlienTextBaseline.x =
		TextXOffs() + (SisTextWidth() >> 1);
	melnorme_desc.AlienTextBaseline.y = 0;
	melnorme_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	local_stack0 = 0;
	local_stack1 = 0;

	prevMsgMode = StatMsgMode::Undefined;

	setSegue(Segue::Peace);
	AskedToBuy = false;
	ConversationData* retval {&melnorme_desc};

	return (retval);
}

} // namespace uqm
