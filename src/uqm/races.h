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

#ifndef UQM_RACES_H_
#define UQM_RACES_H_

#include <cstdint>
#include "core/misc/bittricks.h"
#include "units.h"
#include "displist.h"
#include "uqm/inteldefs.h"

typedef struct STARSHIP STARSHIP;
typedef HLINK HSTARSHIP;

#include "element.h"
#include "libs/sndlib.h"
#include "libs/reslib.h"


// TODO: remove RacesPerPlayer remnant of SC1

inline constexpr auto MaxShipsPerSide {14};

/* SHIP_INFO.ship_flags - ship specific flags */
/* bits 0 and 1 are now available */
inline constexpr auto SeekingWeapon {(1 << 2)};
inline constexpr auto SeekingSpecial {(1 << 3)};
inline constexpr auto PointDefense {(1 << 4)};
/* Ship has some point-defense capabilities */
inline constexpr auto ImmediateWeapon {(1 << 5)};
inline constexpr auto CrewImmune {(1 << 6)};
inline constexpr auto FiresFore {(1 << 7)};
inline constexpr auto FiresRight {(1 << 8)};
inline constexpr auto FiresAft {(1 << 9)};
inline constexpr auto FiresLeft {(1 << 10)};
inline constexpr auto ShieldDefense {(1 << 11)};
inline constexpr auto DontChase {(1 << 12)};
inline constexpr auto PlayerCaptain {(1 << 13)};
/* The protagonist himself is on board. He gets a different color. */


typedef uint16_t STATUS_FLAGS;

/* STATUS_FLAGS - heat of battle specific flags */
inline constexpr auto LeftFlag {(1 << 0)};
inline constexpr auto RightFlag {(1 << 1)};
inline constexpr auto ThrustFlag {(1 << 2)};
inline constexpr auto WeaponFlag {(1 << 3)};
inline constexpr auto SpecialFlag {(1 << 4)};
inline constexpr auto LowOnEnergy {(1 << 5)};
inline constexpr auto ShipBeyondMaxSpeed {(1 << 6)};
inline constexpr auto ShipAtMaxSpeed {(1 << 7)};
inline constexpr auto ShipInGravityWell {(1 << 8)};
inline constexpr auto PlayVictoryDitty {(1 << 9)};

inline constexpr auto ShofixtiExplosion {(1 << 10)};
inline constexpr auto AndrosynCometToggle {(1 << 11)};

/* These track the old resource package orderings for the ship resource indices */
enum SPECIES_ID
{
	NO_ID,
	ARILOU_ID,
	CHMMR_ID,
	EARTHLING_ID,
	ORZ_ID,
	PKUNK_ID,
	SHOFIXTI_ID,
	SPATHI_ID,
	SUPOX_ID,
	THRADDASH_ID,
	UTWIG_ID,
	VUX_ID,
	YEHAT_ID,
	MELNORME_ID,
	DRUUGE_ID,
	ILWRATH_ID,
	MYCON_ID,
	SLYLANDRO_ID,
	UMGAH_ID,
	UR_QUAN_ID,
	ZOQFOTPIK_ID,
	SYREEN_ID,
	KOHR_AH_ID,
	ANDROSYNTH_ID,
	CHENJESU_ID,
	MMRNMHRM_ID,
	LAST_MELEE_ID = MMRNMHRM_ID,
	SIS_SHIP_ID,
	SA_MATRA_ID,
	UR_QUAN_PROBE_ID,
	NUM_SPECIES_ID
};
// TODO figure out why we need this.
inline SPECIES_ID postIncSpeciesId_HACK(SPECIES_ID& id)
{
	const SPECIES_ID ret {id};
	++reinterpret_cast<int&>(id);
	return ret;
}


struct CAPTAIN_STUFF
{
	RESOURCE captain_rsc;
	FRAME background;
	FRAME turn;
	FRAME thrust;
	FRAME weapon;
	FRAME special;
	uint8_t tl_offset;
	uint8_t tr_offset;
	uint8_t thrust_offset;
	uint8_t weapon_offset;
	uint8_t special_offset;
};

enum MOVEMENT_STATE
{
	PURSUE = 0,
	AVOID,
	ENTICE,
	NO_MOVEMENT
};

struct EVALUATE_DESC
{
	ELEMENT* ObjectPtr;
	uint16_t facing;
	uint16_t which_turn;
	MOVEMENT_STATE MoveState;
};

typedef void(IntelligenceFunc)(ELEMENT* ShipPtr,
							   EVALUATE_DESC* ObjectsOfConcern, uint16_t ConcernCounter);
struct INTEL_STUFF
{
	uint16_t ManeuverabilityIndex;
	uint16_t WeaponRange;
	IntelligenceFunc* intelligence_func;
};

struct CHARACTERISTIC_STUFF
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

struct SHIP_INFO
{
	const char* idStr;
	uint16_t ship_flags;
	uint8_t ship_cost;

	uint16_t crew_level;
	uint16_t max_crew;
	uint8_t energy_level;
	uint8_t max_energy;

	RESOURCE race_strings_rsc;
	RESOURCE icons_rsc;
	RESOURCE melee_icon_rsc;

	uqm::StringTable* race_strings;
	FRAME icons;
	FRAME melee_icon;
};

inline constexpr auto InfiniteRadius {((uint16_t)~0)};

struct FLEET_STUFF
{
	uint16_t strength;
	GFXPOINT known_loc;
};

struct DATA_STUFF
{
	RESOURCE ship_rsc[NumViews];
	RESOURCE weapon_rsc[NumViews];
	RESOURCE special_rsc[NumViews];
	CAPTAIN_STUFF captain_control;
	RESOURCE victory_ditty_rsc;
	RESOURCE ship_sounds_rsc;

	FRAME ship[NumViews];
	FRAME weapon[NumViews];
	FRAME special[NumViews];
	MUSIC_REF victory_ditty;
	SOUND_REF ship_sounds;
	// The index in ship_sounds of the sound of the primary weapon.
	uint16_t weapon_sound_index;
};


typedef struct race_desc RACE_DESC;

typedef void(PREPROCESS_FUNC)(ELEMENT* ElementPtr);
typedef void(POSTPROCESS_FUNC)(ELEMENT* ElementPtr);
typedef uint16_t(INIT_WEAPON_FUNC)(ELEMENT* ElementPtr, HELEMENT Weapon[]);
typedef void(UNINIT_FUNC)(RACE_DESC* pRaceDesc);

struct race_desc
{
	SHIP_INFO ship_info;
	FLEET_STUFF fleet;
	CHARACTERISTIC_STUFF characteristics;
	DATA_STUFF ship_data;
	INTEL_STUFF cyborg_control;

	UNINIT_FUNC* uninit_func;
	PREPROCESS_FUNC* preprocess_func;
	POSTPROCESS_FUNC* postprocess_func;
	INIT_WEAPON_FUNC* init_weapon_func;

	void* data; // private ship data, ship code owns this

	void* CodeRef;
};


struct SHIP_BASE
{
	/* LINK elements; must be first */
	HLINK pred;
	HLINK succ;

	SPECIES_ID SpeciesID;
	uint8_t captains_name_index; /* Also used in full-game to detect if a STARSHIP is an escort
			 * or the flagship (captains_name_index == 0) */
};


struct STARSHIP : SHIP_BASE
{

	RACE_DESC* RaceDescPtr;

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
	FRAME icons;

	// Battle states
	uint8_t weapon_counter;	 // In battle: frames left before primary weapon can be used
	uint8_t special_counter; // In battle: frames left before special can be used
	uint8_t energy_counter;	 // In battle: frames left before energy regeneration

	uint8_t ship_input_state;
	STATUS_FLAGS cur_status_flags;
	STATUS_FLAGS old_status_flags;

	HELEMENT hShip;
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

static inline STARSHIP* LockStarShip(const QUEUE* pq, HSTARSHIP h)
{
	assert(GetLinkSize(pq) == sizeof(STARSHIP));
	return (STARSHIP*)LockLink(pq, h);
}

constexpr auto UnlockStarShip(const auto& pq, const auto& h) { return UnlockLink(pq, h); }


typedef HLINK HSHIPFRAG;

inline constexpr auto InfiniteFleet {((uint16_t)~0)};

struct SHIP_FRAGMENT : SHIP_BASE
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
	FRAME icons;
	FRAME melee_icon; /* Only used by Shipyard */
};

static inline SHIP_FRAGMENT* LockShipFrag(const QUEUE* pq, HSHIPFRAG h)
{
	assert(GetLinkSize(pq) == sizeof(SHIP_FRAGMENT));
	return (SHIP_FRAGMENT*)LockLink(pq, h);
}

constexpr auto UnlockShipFrag(const auto& pq, const auto& h) { return UnlockLink(pq, h); }
constexpr auto FreeShipFrag(const auto& pq, const auto& h) { return FreeLink(pq, h); }


typedef HLINK HFLEETINFO;

struct FLEET_INFO
{
	// LINK elements; must be first
	HFLEETINFO pred;
	HFLEETINFO succ;

	SPECIES_ID SpeciesID;
	const char* shipIdStr;

	uint16_t allied_state; /* GOOD_GUY, BAD_GUY or DEAD_GUY */
	uint8_t days_left;	   /* Days left before the fleet reachers 'dest_loc'. */
	uint8_t growth_fract;
	uint16_t crew_level;
	uint16_t max_crew;
	uint8_t growth;
	uint8_t max_energy;
	GFXPOINT loc; /* Location of the fleet (center) */

	uqm::StringTable* race_strings;
	/* Race specific strings, see doc/devel/racestrings. */
	FRAME icons;
	FRAME melee_icon;

	uint16_t actual_strength;
	/* Measure for the size of the sphere of influence.
			 * 0 if there is none and no ships will be generated.
	         * '(uint16_t) ~0' if there is none, and the ship generation
			 * is handled separately. */
	uint16_t known_strength;
	/* Measure for the size of the sphere of influence when last
			 * checked the starmap.
			 * 0 if the race's SoI is not known. */
	GFXPOINT known_loc;
	/* Location of the SoI (center) when last checked
			 * the starmap. */

	uint8_t growth_err_term;
	uint8_t func_index;
	/* Function index defined in clock.h (the same as in SetEvent())
			 * for the function to call when the fleet reaches 'dest_loc'.
			 * '(uint8_t) ~0' means no function to call. */
	GFXPOINT dest_loc;
	/* Location to which the fleet (center) is moving. */
	bool can_build;
	/* Determines if you can build the ship regardless
			 * of allegiance */
};

// Values for FLEET_INFO.allied_state
enum
{
	DEAD_GUY = 0, // Race is extinct
	GOOD_GUY,	  // Race is allied with the player
	BAD_GUY,	  // Race is not allied with the player
};

static inline FLEET_INFO* LockFleetInfo(const QUEUE* pq, HFLEETINFO h)
{
	assert(GetLinkSize(pq) == sizeof(FLEET_INFO));
	return (FLEET_INFO*)LockLink(pq, h);
}

constexpr auto UnlockFleetInfo(const auto& pq, const auto& h) { return UnlockLink(pq, h); }

// Used as index into avail_race_q.
typedef enum
{
	ARILOU_SHIP,
	CHMMR_SHIP,
	HUMAN_SHIP,
	ORZ_SHIP,
	PKUNK_SHIP,
	SHOFIXTI_SHIP,
	SPATHI_SHIP,
	SUPOX_SHIP,
	THRADDASH_SHIP,
	UTWIG_SHIP,
	VUX_SHIP,
	YEHAT_SHIP,
	MELNORME_SHIP,
	DRUUGE_SHIP,
	ILWRATH_SHIP,
	MYCON_SHIP,
	SLYLANDRO_SHIP,
	UMGAH_SHIP,
	URQUAN_SHIP,
	ZOQFOTPIK_SHIP,

	SYREEN_SHIP,
	BLACK_URQUAN_SHIP,
	ANDROSYNTH_SHIP,
	CHENJESU_SHIP,
	MMRNMHRM_SHIP,
	NUM_BUILDABLE_SHIPS,
	YEHAT_REBEL_SHIP = NUM_BUILDABLE_SHIPS,
	URQUAN_DRONE_SHIP,
	SAMATRA_SHIP = URQUAN_DRONE_SHIP,

	NUM_AVAILABLE_RACES
} RACE_ID;

inline constexpr Color RaceColors[] {
	BuildColor(MakeRgb15(0x00, 0x00, 0x10), 0x53), /* ARILOU_SHIP */
};

inline constexpr uint16_t RaceShipForComm[] {
	ARILOU_SHIP,	   /* ARILOU_CONVERSATION */
	CHMMR_SHIP,		   /* CHMMR_CONVERSATION */
	HUMAN_SHIP,		   /* COMMANDER_CONVERSATION */
	ORZ_SHIP,		   /* ORZ_CONVERSATION */
	PKUNK_SHIP,		   /* PKUNK_CONVERSATION */
	SHOFIXTI_SHIP,	   /* SHOFIXTI_CONVERSATION */
	SPATHI_SHIP,	   /* SPATHI_CONVERSATION */
	SUPOX_SHIP,		   /* SUPOX_CONVERSATION */
	THRADDASH_SHIP,	   /* THRADD_CONVERSATION */
	UTWIG_SHIP,		   /* UTWIG_CONVERSATION */
	VUX_SHIP,		   /* VUX_CONVERSATION */
	YEHAT_SHIP,		   /* YEHAT_CONVERSATION */
	MELNORME_SHIP,	   /* MELNORME_CONVERSATION */
	DRUUGE_SHIP,	   /* DRUUGE_CONVERSATION */
	ILWRATH_SHIP,	   /* ILWRATH_CONVERSATION */
	MYCON_SHIP,		   /* MYCON_CONVERSATION */
	SLYLANDRO_SHIP,	   /* SLYLANDRO_CONVERSATION */
	UMGAH_SHIP,		   /* UMGAH_CONVERSATION */
	URQUAN_SHIP,	   /* URQUAN_CONVERSATION */
	ZOQFOTPIK_SHIP,	   /* ZOQFOTPIK_CONVERSATION */
	SYREEN_SHIP,	   /* SYREEN_CONVERSATION */
	BLACK_URQUAN_SHIP, /* BLACKURQ_CONVERSATION */
	ANDROSYNTH_SHIP,   /* ANDROSYNTH_CONVERSATION */
	CHENJESU_SHIP,	   /* CHENJESU_CONVERSATION */
	MMRNMHRM_SHIP,	   /* MMRNMHRM_CONVERSATION */
	UMGAH_SHIP,		   /* TALKING_PET_CONVERSATION */
	SLYLANDRO_SHIP,	   /* SLYLANDRO_HOME_CONVERSATION */
	URQUAN_DRONE_SHIP, /* URQUAN_DRONE_CONVERSATION */
	YEHAT_SHIP,		   /* YEHAT_REBEL_CONVERSATION */
	HUMAN_SHIP		   /* INVALID_CONVERSATION */
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

// How many ships will an encounter consist of.
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

typedef enum
{
	CHMMR_HOME,
	ORZ_HOME,
	PKUNK_HOME,
	SHOFIXTI_HOME,
	SPATHI_HOME,
	SUPOX_HOME,
	THRADDASH_HOME,
	UTWIG_HOME,
	VUX_HOME,
	YEHAT_HOME,
	DRUUGE_HOME,
	ILWRATH_HOME,
	MYCON_HOME,
	SLYLANDRO_HOME,
	UMGAH_HOME,
	ZOQFOTPIK_HOME,

	SYREEN_HOME,
	ANDROSYNTH_HOME,
	NUM_AVAILABLE_HOMES
} HOME_ID;

static inline HOME_ID SpeciesToHomeID(SPECIES_ID species_id)
{
	switch (species_id)
	{
		case CHMMR_ID:
			return CHMMR_HOME;
		case ORZ_ID:
			return ORZ_HOME;
		case PKUNK_ID:
			return PKUNK_HOME;
		case SHOFIXTI_ID:
			return SHOFIXTI_HOME;
		case SPATHI_ID:
			return SPATHI_HOME;
		case SUPOX_ID:
			return SUPOX_HOME;
		case THRADDASH_ID:
			return THRADDASH_HOME;
		case UTWIG_ID:
			return UTWIG_HOME;
		case VUX_ID:
			return VUX_HOME;
		case YEHAT_ID:
			return YEHAT_HOME;
		case DRUUGE_ID:
			return DRUUGE_HOME;
		case ILWRATH_ID:
			return ILWRATH_HOME;
		case MYCON_ID:
			return MYCON_HOME;
		case SLYLANDRO_ID:
			return SLYLANDRO_HOME;
		case UMGAH_ID:
			return UMGAH_HOME;
		case ZOQFOTPIK_ID:
			return ZOQFOTPIK_HOME;
		case SYREEN_ID:
			return SYREEN_HOME;
		case ANDROSYNTH_ID:
			return ANDROSYNTH_HOME;
		default:
			return NUM_AVAILABLE_HOMES;
	}
}


#endif /* UQM_RACES_H_ */
