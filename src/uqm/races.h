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
#ifndef UQM_RACES_H
#define UQM_RACES_H


#include <cstdint>

#include "core/misc/bittricks.h"
#include "core/stl/array.h"
#include "core/types/enum.h"
#include "displist.h"
#include "element.h"
#include "libs/reslib.h"
#include "libs/sndlib.h"
#include "units.h"
#include "uqm/inteldefs.h"

namespace uqm
{
struct Starship;
using StarshipHandle = LinkHandle;

// TODO: remove RacesPerPlayer remnant of SC1

inline constexpr auto MaxShipsPerSide {14};

/* ShipInfoDesc.ship_flags - ship specific flags */
/* bits 0 and 1 are now available */
enum class ShipFlags : uint16_t
{
	None = 0,
	SeekingWeapon = 1 << 2,
	SeekingSpecial = 1 << 3,
	PointDefense = 1 << 4,
	// Ship has some point-defense capabilities
	ImmediateWeapon = 1 << 5,
	CrewImmune = 1 << 6,
	FiresFore = 1 << 7,
	FiresRight = 1 << 8,
	FiresAft = 1 << 9,
	FiresLeft = 1 << 10,
	ShieldDefense = 1 << 11,
	DontChase = 1 << 12,
	PlayerCaptain = 1 << 13,
};
ENUM_BITWISE_OPS(ShipFlags)

// The flags of the four directions that a ship can fire in, clockwise from the front.
inline constexpr uqstl::array<ShipFlags, 4> FireDirections {ShipFlags::FiresFore, ShipFlags::FiresRight, ShipFlags::FiresAft, ShipFlags::FiresLeft};
/* The protagonist himself is on board. He gets a different color. */


/* StatusFlags - heat of battle specific flags */
enum class StatusFlags : uint16_t
{
	None = 0,
	Left = 1 << 0,
	Right = 1 << 1,
	Thrust = 1 << 2,
	Weapon = 1 << 3,
	Special = 1 << 4,
	LowOnEnergy = 1 << 5,
	ShipBeyondMaxSpeed = 1 << 6,
	ShipAtMaxSpeed = 1 << 7,
	ShipInGravityWell = 1 << 8,
	PlayVictoryDitty = 1 << 9,

	ShofixtiExplosion = 1 << 10,
	AndrosynCometToggle = 1 << 11,
};
ENUM_BITWISE_OPS(StatusFlags)

/* These track the old resource package orderings for the ship resource indices */
enum class SpeciesId
{
	None,
	Arilou,
	Chmmr,
	Earthling,
	Orz,
	Pkunk,
	Shofixti,
	Spathi,
	Supox,
	Thraddash,
	Utwig,
	Vux,
	Yehat,
	Melnorme,
	Druuge,
	Ilwrath,
	Mycon,
	Slylandro,
	Umgah,
	UrQuan,
	Zoqfotpik,
	Syreen,
	KohrAh,
	Androsynth,
	Chenjesu,
	Mmrnmhrm,
	LastMelee = Mmrnmhrm,
	SisShip,
	SaMatra,
	UrQuanProbe,
	NumSpecies
};
consteval bool isArithmeticEnum(SpeciesId) { return true; }
// TODO figure out why we need this.
inline SpeciesId postIncSpeciesId_HACK(SpeciesId& id)
{
	const SpeciesId ret {id};
	++reinterpret_cast<int&>(id);
	return ret;
}


struct CaptainGraphics
{
	ResourceName captain_rsc;
	FrameRef background;
	FrameRef turn;
	FrameRef thrust;
	FrameRef weapon;
	FrameRef special;
	uint8_t tl_offset;
	uint8_t tr_offset;
	uint8_t thrust_offset;
	uint8_t weapon_offset;
	uint8_t special_offset;
};

enum class MovementState
{
	Pursue = 0,
	Avoid,
	Entice,
	NoMovement
};

struct ObjectEvaluation
{
	Element* ObjectPtr;
	uint16_t facing;
	uint16_t which_turn;
	MovementState MoveState;
};

typedef void(IntelligenceFunc)(Element* ShipPtr,
							   ObjectEvaluation* ObjectsOfConcern, uint16_t ConcernCounter);
struct ShipIntelligence
{
	uint16_t ManeuverabilityIndex;
	uint16_t WeaponRange;
	IntelligenceFunc* intelligence_func;
};

struct ShipCharacteristics
{
	uint16_t max_thrust;
	uint16_t thrust_increment;
	uint8_t energy_regeneration;
	uint8_t weapon_energy_cost;
	uint8_t special_energy_cost;
	uint8_t energy_wait;
	uint8_t turn_wait;
	uint8_t thrust_wait;
	uint8_t weapon_wait;
	uint8_t special_wait;
	uint8_t ship_mass;
};

struct ShipInfoDesc
{
	const char* idStr;
	ShipFlags ship_flags;
	uint8_t ship_cost;

	uint16_t crew_level;
	uint16_t max_crew;
	uint8_t energy_level;
	uint8_t max_energy;

	ResourceName race_strings_rsc;
	ResourceName icons_rsc;
	ResourceName melee_icon_rsc;

	uqm::StringTable* race_strings;
	FrameRef icons;
	FrameRef melee_icon;
};

inline constexpr auto InfiniteRadius {(uint16_t)~0};

struct FleetStatus
{
	uint16_t strength;
	GfxPoint known_loc;
};

struct ShipResources
{
	ResourceName ship_rsc[NumViews];
	ResourceName weapon_rsc[NumViews];
	ResourceName special_rsc[NumViews];
	CaptainGraphics captain_control;
	ResourceName victory_ditty_rsc;
	ResourceName ship_sounds_rsc;

	FrameRef ship[NumViews];
	FrameRef weapon[NumViews];
	FrameRef special[NumViews];
	MusicRef victory_ditty;
	SoundBankRef ship_sounds;
	// The index in ship_sounds of the sound of the primary weapon.
	uint16_t weapon_sound_index;
};


struct RaceDesc;

typedef void(PREPROCESS_FUNC)(Element* ElementPtr);
typedef void(POSTPROCESS_FUNC)(Element* ElementPtr);
typedef uint16_t(INIT_WEAPON_FUNC)(Element* ElementPtr, ElementHandle Weapon[]);
typedef void(UNINIT_FUNC)(RaceDesc* pRaceDesc);

struct RaceDesc
{
	ShipInfoDesc ship_info;
	FleetStatus fleet;
	ShipCharacteristics characteristics;
	ShipResources ship_data;
	ShipIntelligence cyborg_control;

	UNINIT_FUNC* uninit_func;
	PREPROCESS_FUNC* preprocess_func;
	POSTPROCESS_FUNC* postprocess_func;
	INIT_WEAPON_FUNC* init_weapon_func;

	void* data; // private ship data, ship code owns this

	void* CodeRef;
};


struct ShipBase
{
	/* QueueLink elements; must be first */
	LinkHandle pred;
	LinkHandle succ;

	SpeciesId SpeciesID;
	uint8_t captains_name_index; /* Also used in full-game to detect if a Starship is an escort
			 * or the flagship (captains_name_index == 0) */
};


struct Starship : ShipBase
{

	RaceDesc* RaceDescPtr;

	// Ship information
	uint16_t crew_level;
	// In full-game battles: crew left
	// In SuperMelee: irrelevant
	uint16_t max_crew;
	uint8_t ship_cost;
	// In Super Melee ship queue: ship cost
	// In full-game: irrelevant
	uint16_t index;
	// original queue index
	uqm::StringTable* race_strings;
	FrameRef icons;

	// Battle states
	uint8_t weapon_counter;	 // In battle: frames left before primary weapon can be used
	uint8_t special_counter; // In battle: frames left before special can be used
	uint8_t energy_counter;	 // In battle: frames left before energy regeneration

	StatusFlags ship_input_state;
	StatusFlags cur_status_flags;
	StatusFlags old_status_flags;

	ElementHandle hShip;
	uint16_t ShipFacing;

	//  0: bottom player; In full-game: the human player (RPG)
	//  1: top player; In full-game: the NPC opponent
	// -1: neutral; this should currently never happen (asserts)
	int16_t playerNr;
	// Human, Computer or Network control flags, see intelDefs.h
	PlayerControlFlags control {PlayerControlFlags::None};
};

inline constexpr auto RpgPlayerNum {0};
inline constexpr auto NpcPlayerNum {1};

static inline Starship* LockStarShip(const Queue* pq, StarshipHandle h)
{
	assert(GetLinkSize(pq) == sizeof(Starship));
	return (Starship*)LockLink(pq, h);
}

constexpr auto UnlockStarShip(const auto& pq, const auto& h) { return UnlockLink(pq, h); }


using ShipFragmentHandle = LinkHandle;

inline constexpr auto InfiniteFleet {(uint16_t)~0};

struct ShipFragment : ShipBase
{
	uint8_t race_id;
	uint8_t index;
	uint16_t crew_level;
	/* For ships in npc_built_ship_q, the value InfiniteFleet for
			 * crew_level indicates an infinite number of ships. */
	uint16_t max_crew;

	uint8_t energy_level;
	uint8_t max_energy;
	// XXX: energy_level and max_energy are unused. We save and load
	//   them, but otherwise nothing needs them atm.

	uqm::StringTable* race_strings;
	FrameRef icons;
	FrameRef melee_icon; /* Only used by Shipyard */
};

static inline ShipFragment* LockShipFrag(const Queue* pq, ShipFragmentHandle h)
{
	assert(GetLinkSize(pq) == sizeof(ShipFragment));
	return (ShipFragment*)LockLink(pq, h);
}

constexpr auto UnlockShipFrag(const auto& pq, const auto& h) { return UnlockLink(pq, h); }
constexpr auto FreeShipFrag(const auto& pq, const auto& h) { return FreeLink(pq, h); }


using FleetInfoHandle = LinkHandle;

struct FleetInfo
{
	// QueueLink elements; must be first
	FleetInfoHandle pred;
	FleetInfoHandle succ;

	SpeciesId SpeciesID;
	const char* shipIdStr;

	uint16_t allied_state; /* GOOD_GUY, BAD_GUY or DEAD_GUY */
	uint8_t days_left;	   /* Days left before the fleet reachers 'dest_loc'. */
	uint8_t growth_fract;
	uint16_t crew_level;
	uint16_t max_crew;
	uint8_t growth;
	uint8_t max_energy;
	GfxPoint loc; /* Location of the fleet (center) */

	uqm::StringTable* race_strings;
	/* Race specific strings, see doc/devel/racestrings. */
	FrameRef icons;
	FrameRef melee_icon;

	uint16_t actual_strength;
	/* Measure for the size of the sphere of influence.
			 * 0 if there is none and no ships will be generated.
	         * '(uint16_t) ~0' if there is none, and the ship generation
			 * is handled separately. */
	uint16_t known_strength;
	/* Measure for the size of the sphere of influence when last
			 * checked the starmap.
			 * 0 if the race's SoI is not known. */
	GfxPoint known_loc;
	/* Location of the SoI (center) when last checked
			 * the starmap. */

	uint8_t growth_err_term;
	uint8_t func_index;
	/* Function index defined in clock.h (the same as in SetEvent())
			 * for the function to call when the fleet reaches 'dest_loc'.
			 * '(uint8_t) ~0' means no function to call. */
	GfxPoint dest_loc;
	/* Location to which the fleet (center) is moving. */
	bool can_build;
	/* Determines if you can build the ship regardless
			 * of allegiance */
};

// Values for FleetInfo.allied_state
enum class RaceAllegiance
{
	Dead = 0, // Race is extinct
	Good,	  // Race is allied with the player
	Bad,	  // Race is not allied with the player
};
consteval bool isArithmeticEnum(RaceAllegiance) { return true; }

static inline FleetInfo* LockFleetInfo(const Queue* pq, FleetInfoHandle h)
{
	assert(GetLinkSize(pq) == sizeof(FleetInfo));
	return (FleetInfo*)LockLink(pq, h);
}

constexpr auto UnlockFleetInfo(const auto& pq, const auto& h) { return UnlockLink(pq, h); }

// Used as index into avail_race_q.
enum class RaceId
{
	Arilou,
	Chmmr,
	Human,
	Orz,
	Pkunk,
	Shofixti,
	Spathi,
	Supox,
	Thraddash,
	Utwig,
	Vux,
	Yehat,
	Melnorme,
	Druuge,
	Ilwrath,
	Mycon,
	Slylandro,
	Umgah,
	Urquan,
	Zoqfotpik,

	Syreen,
	BlackUrquan,
	Androsynth,
	Chenjesu,
	Mmrnmhrm,
	NumBuildableShips,
	YehatRebel = NumBuildableShips,
	UrquanDrone,
	Samatra = UrquanDrone,

	NumAvailableRaces
};
consteval bool isArithmeticEnum(RaceId) { return true; }

inline constexpr Color RaceColors[] {
	BuildColor(MakeRgb15(0x00, 0x00, 0x10), 0x53), /* ARILOU_SHIP */
};

inline constexpr uint16_t RaceShipForComm[] {
	*RaceId::Arilou,	  /* ARILOU_CONVERSATION */
	*RaceId::Chmmr,		  /* CHMMR_CONVERSATION */
	*RaceId::Human,		  /* COMMANDER_CONVERSATION */
	*RaceId::Orz,		  /* ORZ_CONVERSATION */
	*RaceId::Pkunk,		  /* PKUNK_CONVERSATION */
	*RaceId::Shofixti,	  /* SHOFIXTI_CONVERSATION */
	*RaceId::Spathi,	  /* SPATHI_CONVERSATION */
	*RaceId::Supox,		  /* SUPOX_CONVERSATION */
	*RaceId::Thraddash,	  /* THRADD_CONVERSATION */
	*RaceId::Utwig,		  /* UTWIG_CONVERSATION */
	*RaceId::Vux,		  /* VUX_CONVERSATION */
	*RaceId::Yehat,		  /* YEHAT_CONVERSATION */
	*RaceId::Melnorme,	  /* MELNORME_CONVERSATION */
	*RaceId::Druuge,	  /* DRUUGE_CONVERSATION */
	*RaceId::Ilwrath,	  /* ILWRATH_CONVERSATION */
	*RaceId::Mycon,		  /* MYCON_CONVERSATION */
	*RaceId::Slylandro,	  /* SLYLANDRO_CONVERSATION */
	*RaceId::Umgah,		  /* UMGAH_CONVERSATION */
	*RaceId::Urquan,	  /* URQUAN_CONVERSATION */
	*RaceId::Zoqfotpik,	  /* ZOQFOTPIK_CONVERSATION */
	*RaceId::Syreen,	  /* SYREEN_CONVERSATION */
	*RaceId::BlackUrquan, /* BLACKURQ_CONVERSATION */
	*RaceId::Androsynth,  /* ANDROSYNTH_CONVERSATION */
	*RaceId::Chenjesu,	  /* CHENJESU_CONVERSATION */
	*RaceId::Mmrnmhrm,	  /* MMRNMHRM_CONVERSATION */
	*RaceId::Umgah,		  /* TALKING_PET_CONVERSATION */
	*RaceId::Slylandro,	  /* SLYLANDRO_HOME_CONVERSATION */
	*RaceId::UrquanDrone, /* URQUAN_DRONE_CONVERSATION */
	*RaceId::Yehat,		  /* YEHAT_REBEL_CONVERSATION */
	*RaceId::Human		  /* INVALID_CONVERSATION */
};

constexpr auto LogToIp(const auto& s) { return ((s) << 1); }
inline constexpr int16_t RaceIpSpeed[] {
	LogToIp(40), /* ARILOU_SHIP */
	LogToIp(27), /* CHMMR_SHIP */
	LogToIp(24), /* HUMAN_SHIP */
	LogToIp(40), /* ORZ_SHIP */
	LogToIp(40), /* PKUNK_SHIP */
	LogToIp(35), /* SHOFIXTI_SHIP */
	LogToIp(48), /* SPATHI_SHIP */
	LogToIp(40), /* SUPOX_SHIP */
	LogToIp(28), /* THRADDASH_SHIP */
	LogToIp(30), /* UTWIG_SHIP */
	LogToIp(21), /* VUX_SHIP */
	LogToIp(30), /* YEHAT_SHIP */
	LogToIp(40), /* MELNORME_SHIP */
	LogToIp(20), /* DRUUGE_SHIP */
	LogToIp(25), /* ILWRATH_SHIP */
	LogToIp(27), /* MYCON_SHIP */
	LogToIp(60), /* SLYLANDRO_SHIP */
	LogToIp(18), /* UMGAH_SHIP */
	LogToIp(30), /* URQUAN_SHIP */
	LogToIp(40), /* ZOQFOTPIK_SHIP */
	LogToIp(36), /* SYREEN_SHIP */
	LogToIp(30), /* BLACK_URQUAN_SHIP */
	LogToIp(24), /* ANDROSYNTH_SHIP */
	LogToIp(27), /* CHENJESU_SHIP */
	LogToIp(50), /* MMRNMHRM_SHIP */
	LogToIp(30), /* YEHAT_REBEL_SHIP */
	LogToIp(90), /* URQUAN_DRONE_SHIP */
};

constexpr auto LogToHyper(const auto& s) { return (WorldToVelocity(s) >> 1); }
inline constexpr uint16_t RaceHyperspacePercent[] {
	20, /* ARILOU_SHIP */
	0,	/* CHMMR_SHIP */
	0,	/* HUMAN_SHIP */
	20, /* ORZ_SHIP */
	40, /* PKUNK_SHIP */
	0,	/* SHOFIXTI_SHIP */
	20, /* SPATHI_SHIP */
	40, /* SUPOX_SHIP */
	60, /* THRADDASH_SHIP */
	40, /* UTWIG_SHIP */
	40, /* VUX_SHIP */
	60, /* YEHAT_SHIP */
	0,	/* MELNORME_SHIP */
	30, /* DRUUGE_SHIP */
	60, /* ILWRATH_SHIP */
	40, /* MYCON_SHIP */
	2,	/* SLYLANDRO_SHIP */
	30, /* UMGAH_SHIP */
	70, /* URQUAN_SHIP */
	0,	/* ZOQFOTPIK_SHIP */
	0,	/* SYREEN_SHIP */
	70, /* BLACK_URQUAN_SHIP */
	0,	/* ANDROSYNTH_SHIP */
	0,	/* CHENJESU_SHIP */
	0,	/* MMRNMHRM_SHIP */
	60, /* YEHAT_REBEL_SHIP */
	0,	/* URQUAN_DRONE_SHIP */
};

inline constexpr uint8_t RaceInterplanetaryPercent[] {
	2,	/* ARILOU_SHIP */
	0,	/* CHMMR_SHIP */
	0,	/* HUMAN_SHIP */
	20, /* ORZ_SHIP */
	20, /* PKUNK_SHIP */
	0,	/* SHOFIXTI_SHIP */
	10, /* SPATHI_SHIP */
	20, /* SUPOX_SHIP */
	20, /* THRADDASH_SHIP */
	20, /* UTWIG_SHIP */
	20, /* VUX_SHIP */
	40, /* YEHAT_SHIP */
	0,	/* MELNORME_SHIP */
	20, /* DRUUGE_SHIP */
	60, /* ILWRATH_SHIP */
	20, /* MYCON_SHIP */
	5,	/* SLYLANDRO_SHIP */
	20, /* UMGAH_SHIP */
	40, /* URQUAN_SHIP */
	0,	/* ZOQFOTPIK_SHIP */
	0,	/* SYREEN_SHIP */
	40, /* BLACK_URQUAN_SHIP */
	0,	/* ANDROSYNTH_SHIP */
	0,	/* CHENJESU_SHIP */
	0,	/* MMRNMHRM_SHIP */
	40, /* YEHAT_REBEL_SHIP */
	0,	/* URQUAN_DRONE_SHIP */
};

inline constexpr uint8_t RaceMusicBool[] {
	1, /* ARILOU_SHIP */
	1, /* CHMMR_SHIP */
	0, /* HUMAN_SHIP */
	1, /* ORZ_SHIP */
	1, /* PKUNK_SHIP */
	0, /* SHOFIXTI_SHIP */
	1, /* SPATHI_SHIP */
	1, /* SUPOX_SHIP */
	1, /* THRADDASH_SHIP */
	1, /* UTWIG_SHIP */
	1, /* VUX_SHIP */
	1, /* YEHAT_SHIP */
	0, /* MELNORME_SHIP */
	1, /* DRUUGE_SHIP */
	1, /* ILWRATH_SHIP */
	1, /* MYCON_SHIP */
	0, /* SLYLANDRO_SHIP */
	1, /* UMGAH_SHIP */
	1, /* URQUAN_SHIP */
	1, /* ZOQFOTPIK_SHIP */
	1, /* SYREEN_SHIP */
	1, /* BLACK_URQUAN_SHIP */
	0, /* ANDROSYNTH_SHIP */
	0, /* CHENJESU_SHIP */
	0, /* MMRNMHRM_SHIP */
	1, /* YEHAT_REBEL_SHIP */
	0, /* URQUAN_DRONE_SHIP */
};

// How many ships will an Encounter consist of.
// The first number specifies the minimum, the second the maximum.
// The chance is 50% for each ship past the minimum to be present.
inline constexpr uint8_t RaceEncounterMakeup[] {
	uqm::makeByte(1, 5), /* ARILOU_SHIP */
	0,					 /* CHMMR_SHIP */
	0,					 /* HUMAN_SHIP */
	uqm::makeByte(1, 5), /* ORZ_SHIP */
	uqm::makeByte(1, 5), /* PKUNK_SHIP */
	0,					 /* SHOFIXTI_SHIP */
	uqm::makeByte(1, 5), /* SPATHI_SHIP */
	uqm::makeByte(1, 5), /* SUPOX_SHIP */
	uqm::makeByte(1, 5), /* THRADDASH_SHIP */
	uqm::makeByte(1, 5), /* UTWIG_SHIP */
	uqm::makeByte(1, 5), /* VUX_SHIP */
	uqm::makeByte(1, 5), /* YEHAT_SHIP */
	uqm::makeByte(1, 1), /* MELNORME_SHIP */
	uqm::makeByte(1, 5), /* DRUUGE_SHIP */
	uqm::makeByte(1, 5), /* ILWRATH_SHIP */
	uqm::makeByte(1, 5), /* MYCON_SHIP */
	uqm::makeByte(1, 1), /* SLYLANDRO_SHIP */
	uqm::makeByte(1, 5), /* UMGAH_SHIP */
	uqm::makeByte(1, 5), /* URQUAN_SHIP */
	uqm::makeByte(1, 5), /* ZOQFOTPIK_SHIP */
	0,					 /* SYREEN_SHIP */
	uqm::makeByte(1, 5), /* BLACK_URQUAN_SHIP */
	0,					 /* ANDROSYNTH_SHIP*/
	0,					 /* CHENJESU_SHIP */
	0,					 /* MMRNMHRM_SHIP */
	uqm::makeByte(1, 5), /* YEHAT_REBEL_SHIP */
};


// JMS: For showing the SC1-era situation in starmap

// JMS: For showing the SC1-era situation in starmap

enum class HomeId
{
	Chmmr,
	Orz,
	Pkunk,
	Shofixti,
	Spathi,
	Supox,
	Thraddash,
	Utwig,
	Vux,
	Yehat,
	Druuge,
	Ilwrath,
	Mycon,
	Slylandro,
	Umgah,
	Zoqfotpik,

	Syreen,
	Androsynth,
	NumAvailableHomes
};
consteval bool isArithmeticEnum(HomeId) { return true; }

static inline HomeId SpeciesToHomeID(SpeciesId species_id)
{
	switch (species_id)
	{
		case SpeciesId::Chmmr:
			return HomeId::Chmmr;
		case SpeciesId::Orz:
			return HomeId::Orz;
		case SpeciesId::Pkunk:
			return HomeId::Pkunk;
		case SpeciesId::Shofixti:
			return HomeId::Shofixti;
		case SpeciesId::Spathi:
			return HomeId::Spathi;
		case SpeciesId::Supox:
			return HomeId::Supox;
		case SpeciesId::Thraddash:
			return HomeId::Thraddash;
		case SpeciesId::Utwig:
			return HomeId::Utwig;
		case SpeciesId::Vux:
			return HomeId::Vux;
		case SpeciesId::Yehat:
			return HomeId::Yehat;
		case SpeciesId::Druuge:
			return HomeId::Druuge;
		case SpeciesId::Ilwrath:
			return HomeId::Ilwrath;
		case SpeciesId::Mycon:
			return HomeId::Mycon;
		case SpeciesId::Slylandro:
			return HomeId::Slylandro;
		case SpeciesId::Umgah:
			return HomeId::Umgah;
		case SpeciesId::Zoqfotpik:
			return HomeId::Zoqfotpik;
		case SpeciesId::Syreen:
			return HomeId::Syreen;
		case SpeciesId::Androsynth:
			return HomeId::Androsynth;
		default:
			return HomeId::NumAvailableHomes;
	}
}

} // namespace uqm


#endif // UQM_RACES_H
