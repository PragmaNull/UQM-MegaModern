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
#ifndef UQM_GLOBDATA_H_
#define UQM_GLOBDATA_H_

#include "options/OptionTypes.h"
#include "options/options.h"
#include "clock.h"
#include "libs/gfxlib.h"
#include "libs/reslib.h"
#include "libs/sndlib.h"
#include "sis.h"
#include "velocity.h"
#include "commanim.h"

// general numbers-speech generator info
// should accomodate most common base-10 languages
// many languages require various plural forms
// for digit names like "hundred"
// possibly needs reworking for others
struct SpeechDigitName
{
	// an array of these structs must be in ascending remainder order
	// terminate the array with Divider == 0

	// digit divider, i.e. 1, 10, 100, etc.
	int Divider;
	// maximum remainder for this name
	// name will be used if Number % Divider <= MaxRemainder
	int MaxRemainder;
	// string table index for this name
	// i.e. "hundred" in English
	uint16_t StrIndex;
};

struct SpeechDigit
{
	// digit divider, i.e. 1, 10, 100, etc.
	int Divider;
	// digit sub, i.e. 10 for teens
	// subtracted from the value to get an index into StrDigits
	int Subtrahend;
	// ptr to 10 indices for this digit
	// index is string table ptr when > 0
	//       is invalid (should not happen) or
	//       is a a 'skip digit' indicator when == 0
	// StrDigits can be nullptr, in which case
	// the value is interpreted recursively
	uint16_t* StrDigits;
	// digit Names, can be nullptr, in which case
	// CommonNameIndex is used
	SpeechDigitName* Names;
	// common digit name string table index
	// i.e. "hundred" in English
	uint16_t CommonNameIndex;
};

// this accomodates up to "billions" in english
inline constexpr auto MaxSpeechDigits {7};

struct NumberSpeechDesc
{
	// slots used in Digits array
	uint16_t NumDigits;
	// slots for each digit in numbers
	// teens is exception
	// 0-9, 10-19, ..20-90, ..100-900, etc.
	SpeechDigit Digits[MaxSpeechDigits];
};
const typedef NumberSpeechDesc* NUMBER_SPEECH;

typedef uint32_t LDAS_FLAGS;


// BW: had to move that from commglue.h to here because now ConversationData features it
typedef enum
{
	ARILOU_CONVERSATION,
	CHMMR_CONVERSATION,
	COMMANDER_CONVERSATION,
	ORZ_CONVERSATION,
	PKUNK_CONVERSATION,
	SHOFIXTI_CONVERSATION,
	SPATHI_CONVERSATION,
	SUPOX_CONVERSATION,
	THRADD_CONVERSATION,
	UTWIG_CONVERSATION,
	VUX_CONVERSATION,
	YEHAT_CONVERSATION,
	MELNORME_CONVERSATION,
	DRUUGE_CONVERSATION,
	ILWRATH_CONVERSATION,
	MYCON_CONVERSATION,
	SLYLANDRO_CONVERSATION,
	UMGAH_CONVERSATION,
	URQUAN_CONVERSATION,
	ZOQFOTPIK_CONVERSATION,
	SYREEN_CONVERSATION,
	BLACKURQ_CONVERSATION,
	TALKING_PET_CONVERSATION,
	SLYLANDRO_HOME_CONVERSATION,
	URQUAN_DRONE_CONVERSATION,
	YEHAT_REBEL_CONVERSATION,
	INVALID_CONVERSATION,

	NUM_CONVERSATIONS,
} CONVERSATION;

struct AlienAltResources
{
	RESOURCE AlienFrameRes;
	RESOURCE AlienColorMapRes;
	RESOURCE AlienSongRes;
};

struct ConversationData
{
	CONVERSATION AlienConv;
	void (*init_encounter_func)(void);
	/* Called when entering communications */
	void (*post_encounter_func)(void);
	/* Called when leaving communications or combat normally */
	uint16_t (*uninit_encounter_func)(void);
	/* Called when Encounter is done for cleanup */

	RESOURCE AlienFrameRes;
	RESOURCE AlienFontRes;
	Color AlienTextFColor, AlienTextBColor;
	GfxPoint AlienTextBaseline;
	uint16_t AlienTextWidth;
	TEXT_ALIGN AlienTextAlign;
	TEXT_VALIGN AlienTextValign;
	RESOURCE AlienColorMapRes;
	RESOURCE AlienSongRes;
	AlienAltResources AltRes;

	RESOURCE ConversationPhrasesRes;

	uint16_t NumAnimations;
	AnimationDesc AlienAmbientArray[MaxAnimations];

	// Transition animation to/from talking state;
	// the first frame is neutral (sort of like YoyoAnim)
	AnimationDesc AlienTransitionDesc;
	// Talking animation, like RandomAnim, except random frames
	// always alternate with a neutral frame;
	// the first frame is neutral
	AnimationDesc AlienTalkDesc;

	NUMBER_SPEECH AlienNumberSpeech;

	FRAME AlienFrame;
	FONT AlienFont;
	COLORMAP AlienColorMap;
	MUSIC_REF AlienSong;
	uqm::StringTable* ConversationPhrases;
	uint16_t AlienColorMapIndex;
};

enum
{
	PORTAL_SPAWNER_DEVICE = 0,
	TALKING_PET_DEVICE,
	UTWIG_BOMB_DEVICE,
	SUN_EFFICIENCY_DEVICE,
	ROSY_SPHERE_DEVICE,
	AQUA_HELIX_DEVICE,
	CLEAR_SPINDLE_DEVICE,
	ULTRON_0_DEVICE,
	ULTRON_1_DEVICE,
	ULTRON_2_DEVICE,
	ULTRON_3_DEVICE,
	MAIDENS_DEVICE,
	UMGAH_HYPERWAVE_DEVICE,
	BURVIX_HYPERWAVE_DEVICE,
	DATA_PLATE_1_DEVICE,
	DATA_PLATE_2_DEVICE,
	DATA_PLATE_3_DEVICE,
	TAALO_PROTECTOR_DEVICE,
	EGG_CASING0_DEVICE,
	EGG_CASING1_DEVICE,
	EGG_CASING2_DEVICE,
	SYREEN_SHUTTLE_DEVICE,
	VUX_BEAST_DEVICE,
	DESTRUCT_CODE_DEVICE,
	URQUAN_WARP_DEVICE,
	WIMBLIS_TRIDENT_DEVICE,
	GLOWING_ROD_DEVICE,
	LUNAR_BASE_DEVICE,

	NUM_DEVICES
};

inline auto YearsToKohrahVictory() { return (uqm::UQMOptions::read().deCleansing ? 100 : 4); }


// Values for GameData.glob_flags:
inline constexpr auto ReadSpeedMask {((1 << 3) - 1)};
inline constexpr auto NumReadSpeeds {5};
inline constexpr auto CombatSpeedShift {6};
inline constexpr auto CombatSpeedMask {(((1 << 2) - 1) << CombatSpeedShift)};
inline constexpr auto NumCombatSpeeds {4};

inline constexpr auto MusicDisabled {(1 << 3)};
inline constexpr auto SoundDisabled {(1 << 4)};
inline constexpr auto CyborgEnabled {(1 << 5)};

enum
{
	SUPER_MELEE = 0, /* Is also used while in the main menu */
	IN_LAST_BATTLE,
	IN_ENCOUNTER,
	IN_HYPERSPACE /* in HyperSpace or QuasiSpace */,
	IN_INTERPLANETARY,
	WON_LAST_BATTLE,

	/* The following three are only used when displaying save game
	 * summaries */
	IN_QUASISPACE,
	IN_PLANET_ORBIT,
	IN_STARBASE,

	CHECK_PAUSE = uqm::makeWord(0, (1 << 0)),
	IN_BATTLE = uqm::makeWord(0, (1 << 1)),
	/* Is also set while in HyperSpace/QuasiSpace */
	START_ENCOUNTER = uqm::makeWord(0, (1 << 2)),
	START_INTERPLANETARY = uqm::makeWord(0, (1 << 3)),
	CHECK_LOAD = uqm::makeWord(0, (1 << 4)),
	CHECK_RESTART = uqm::makeWord(0, (1 << 5)),
	CHECK_ABORT = uqm::makeWord(0, (1 << 6)),
};
typedef uint16_t ACTIVITY;

#include "uqm/session/GameState.h"

namespace uqm
{
// What the game program does now, for example the main menu or HyperSpace. It stays between sessions.
[[nodiscard]] ACTIVITY& currentActivity();
// The in-game options, with the bits of the glob_flags values above. They stay between sessions.
[[nodiscard]] uint8_t& globalFlags();
} // namespace uqm


inline constexpr auto MaxEncounters {16};
inline constexpr auto MaxBattleGroups {32};

/* DEFGRP enumeration. These identify scripted TrueSpace encounters
 * more consistently than offsets into the DefgrpinfoFile state
 * file. */
enum
{
	DEFGRP_NONE,
	DEFGRP_SHOFIXTI,
	DEFGRP_ZOQFOT,
	DEFGRP_MELNORME0,
	DEFGRP_MELNORME1,
	DEFGRP_MELNORME2,
	DEFGRP_MELNORME3,
	DEFGRP_MELNORME4,
	DEFGRP_MELNORME5,
	DEFGRP_MELNORME6,
	DEFGRP_MELNORME7,
	DEFGRP_MELNORME8,
	DEFGRP_URQUAN_PROBE,
	DEFGRP_COLONY,
	DEFGRP_SAMATRA,
	NUM_DEFGRPS
};

//#define STATE_DEBUG


// For dynamic variable names
constexpr auto DSetGameState(const auto& SName, const auto& val) { return setGameStateUint(SName, static_cast<uint32_t>(val)); }
constexpr auto DGetGameState(const auto& SName) { return getGameStateUint(SName); }

extern GFXCONTEXT RadarContext;

extern void FreeSC2Data(void);
extern bool LoadSC2Data(void);

extern bool InitStarseed(bool newgame);

bool inFullGame(void);
bool inSuperMelee(void);
//bool inBattle (void);
//bool inInterPlanetary (void);
//bool inSolarSystem (void);
//bool inOrbit (void);
bool inHQSpace(void);
bool inHyperSpace(void);
bool inQuasiSpace(void);
inline bool isPC(uqm::EmulationMode optWhich) { return optWhich == uqm::EmulationMode::PC; }
inline bool is3DO(uqm::EmulationMode optWhich) { return optWhich == uqm::EmulationMode::Console3DO; }
extern int replaceChar(char* pStr, const char find, const char replace);

extern void LoadFleetInfo(void);
extern bool InitGameStructures(void);
extern void UninitGameStructures(void);

// Difficulty
inline uqm::Difficulty getDifficulty()
{
	return uqm::gameState().sis().Difficulty;
}

template <typename T>
inline T difficultyCase(T easy, T normal, T hard)
{
	switch (getDifficulty())
	{
		case uqm::Difficulty::Normal:
			return normal;
		case uqm::Difficulty::Easy:
			return easy;
		case uqm::Difficulty::Hard:
			return hard;
		case uqm::Difficulty::ChooseYourOwn:
		default:
			return normal; // Default to normal if somehow an invalid difficulty is set
	}
}

inline bool isDifficulty(uqm::Difficulty difficulty)
{
	return getDifficulty() == difficulty;
}

template <typename T>
inline T ifEasyDifficulty(T trueVal, T falseVal) { return isDifficulty(uqm::Difficulty::Easy) ? trueVal : falseVal; }
template <typename T>
inline T ifNormaDifficulty(T trueVal, T falseVal) { return isDifficulty(uqm::Difficulty::Normal) ? trueVal : falseVal; }
template <typename T>
inline T ifHardDifficulty(T trueVal, T falseVal) { return isDifficulty(uqm::Difficulty::Hard) ? trueVal : falseVal; }
//#define DIF_NORM (DIFFICULTY == uqm::Difficulty::Normal ? true : false)
//#define isDifficulty(uqm::Difficulty::Easy) (DIFFICULTY == uqm::Difficulty::Easy ? true : false)
//#define isDifficulty(uqm::Difficulty::Hard) (DIFFICULTY == uqm::Difficulty::Hard ? true : false)
//#define IF_NORM(a, b) (!DIF_NORM ? (a) : (b))
//#define IF_EASY(a, b) (!isDifficulty(uqm::Difficulty::Easy) ? (a) : (b))
//#define IF_HARD(a, b) (!isDifficulty(uqm::Difficulty::Hard) ? (a) : (b))
//#define DIF_STR(a) ((a) == NORM ? "Normal" : ((a) == EASY ? "Easy" : (a) == HARD ? "Hard" : \
//																				   "CYO"))

// Extended
inline auto IsExtended() { return (uqm::gameState().sis().Extended ? true : false); }


// Nomad
static inline uqm::NomadMode getNomadMode() { return uqm::gameState().sis().Nomad; }

// Storage Queue
inline auto StorageQ() { return (uqm::UQMOptions::read().shipStore || isDifficulty(uqm::Difficulty::Hard) || uqm::UQMOptions::read().fleetPointSys); }

// The stored value for no location. The location states are 16 bits; they hold int16_t coordinates.
constexpr uint16_t c_noLocation {0xffff};

static inline GfxPoint LoadLastLoc(void)
{
	return GfxPoint {(int16_t)uqm::gameState().flag(uqm::GameFlag::LAST_LOCATION_X),
					 (int16_t)uqm::gameState().flag(uqm::GameFlag::LAST_LOCATION_Y)};
}

static inline void SaveLastLoc(GfxPoint pt)
{
	uqm::gameState().setFlag(uqm::GameFlag::LAST_LOCATION_X, static_cast<uint32_t>(static_cast<uint16_t>(pt.x)));
	uqm::gameState().setFlag(uqm::GameFlag::LAST_LOCATION_Y, static_cast<uint32_t>(static_cast<uint16_t>(pt.y)));
}

static inline void ZeroLastLoc(void)
{
	uqm::gameState().setFlag(uqm::GameFlag::LAST_LOCATION_X, static_cast<uint32_t>(c_noLocation));
	uqm::gameState().setFlag(uqm::GameFlag::LAST_LOCATION_Y, static_cast<uint32_t>(c_noLocation));
}

static inline GfxPoint LoadAdvancedQuasiPilot(void)
{
	return GfxPoint {(int16_t)uqm::gameState().flag(uqm::GameFlag::ADV_AUTOPILOT_QUASI_X),
					 (int16_t)uqm::gameState().flag(uqm::GameFlag::ADV_AUTOPILOT_QUASI_Y)};
}

static inline GfxPoint LoadAdvancedAutoPilot(void)
{
	return GfxPoint {(int16_t)uqm::gameState().flag(uqm::GameFlag::ADV_AUTOPILOT_SAVE_X),
					 (int16_t)uqm::gameState().flag(uqm::GameFlag::ADV_AUTOPILOT_SAVE_Y)};
}

static inline void SaveAdvancedQuasiPilot(GfxPoint pt)
{
	uqm::gameState().setFlag(uqm::GameFlag::ADV_AUTOPILOT_QUASI_X, static_cast<uint32_t>(static_cast<uint16_t>(pt.x)));
	uqm::gameState().setFlag(uqm::GameFlag::ADV_AUTOPILOT_QUASI_Y, static_cast<uint32_t>(static_cast<uint16_t>(pt.y)));
}

static inline void SaveAdvancedAutoPilot(GfxPoint pt)
{
	uqm::gameState().setFlag(uqm::GameFlag::ADV_AUTOPILOT_SAVE_X, static_cast<uint32_t>(static_cast<uint16_t>(pt.x)));
	uqm::gameState().setFlag(uqm::GameFlag::ADV_AUTOPILOT_SAVE_Y, static_cast<uint32_t>(static_cast<uint16_t>(pt.y)));
}

static inline void ZeroAdvancedQuasiPilot(void)
{
	uqm::gameState().setFlag(uqm::GameFlag::ADV_AUTOPILOT_QUASI_X, static_cast<uint32_t>(c_noLocation));
	uqm::gameState().setFlag(uqm::GameFlag::ADV_AUTOPILOT_QUASI_Y, static_cast<uint32_t>(c_noLocation));
}

static inline void ZeroAdvancedAutoPilot(void)
{
	uqm::gameState().setFlag(uqm::GameFlag::ADV_AUTOPILOT_SAVE_X, static_cast<uint32_t>(c_noLocation));
	uqm::gameState().setFlag(uqm::GameFlag::ADV_AUTOPILOT_SAVE_Y, static_cast<uint32_t>(c_noLocation));
}

constexpr auto BoolStr(const auto& a) { return ((a) ? "True" : "False"); }

// Earth Coordinates
inline constexpr auto EarthOuterX {-725};
inline constexpr auto EarthOuterY {597};

// Druuge Crew Values
inline auto MinSold() { return difficultyCase(200, 100, 10); }
inline auto MaxSold() { return difficultyCase(500, 250, 25); }

static inline bool IsHomeworldKnown(uint32_t homeworld)
{
	if (homeworld > 18)
	{
		return false;
	}

	return (bool)(uqm::gameState().flag(uqm::GameFlag::KNOW_HOMEWORLD) & (1 << homeworld));
}

static inline void SetHomeworldKnown(uint32_t homeworld)
{
	if (IsHomeworldKnown(homeworld))
	{
		return;
	}

	uint32_t current {uqm::gameState().flag(uqm::GameFlag::KNOW_HOMEWORLD)};
	current |= 1 << homeworld;
	uqm::gameState().setFlag(uqm::GameFlag::KNOW_HOMEWORLD, static_cast<uint32_t>(current));
}

#if 0 //defined(__cplusplus)
}
#endif

#endif /* UQM_GLOBDATA_H_ */
