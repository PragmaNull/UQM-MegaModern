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
#ifndef UQM_COMMGLUE_H
#define UQM_COMMGLUE_H


#include "core/types/enum.h"
#include "globdata.h"
#include "libs/callback/callback.h"
#include "libs/sound/trackplayer.h"
#include "resinst.h"

namespace uqm
{
extern ConversationData CommData;
extern uqstl::array<char, 2048> shared_phrase_buf;

// The number of a phrase, from a phrase enum value or from a number.
constexpr int phraseNumber(ArithmeticEnum auto phrase) { return *phrase; }
constexpr int phraseNumber(std::integral auto phrase) { return static_cast<int>(phrase); }

constexpr auto PlayerSaid(const auto& r, const auto& i) { return ((r) == (i)); }
inline bool PhraseEnabled(const auto& p) { return (*CommData.ConversationPhrases->text(phraseNumber(p) - 1) != '\0'); }
inline void DisablePhrase(const auto& p) { CommData.ConversationPhrases->set(phraseNumber(p) - 1, uqstl::string_view {"", 1}); }


using ResponseRef = uint16_t;

using ResponseFunc = void (*)(ResponseRef R);

extern void DoResponsePhrase(ResponseRef R, ResponseFunc response_func, char* ContstructStr);
inline void Response(const auto& i, ResponseFunc a) { DoResponsePhrase(static_cast<ResponseRef>(phraseNumber(i)), a, nullptr); }

// The CallbackFunction is queued and executes synchronously
// on the Starcon2Main thread
extern void NPCPhrase_cb(int index, CallbackFunction cb);
inline void NPCPhrase_cb(ArithmeticEnum auto index, CallbackFunction cb) { NPCPhrase_cb(*index, cb); }
inline void NPCPhrase(const auto& index) { NPCPhrase_cb(phraseNumber(index), nullptr); }
extern void NPCPhrase_splice(int index);
inline void NPCPhrase_splice(ArithmeticEnum auto index) { NPCPhrase_splice(*index); }
extern void NPCNumber(int number, const char* fmt);

extern void construct_response(uqstl::span<char> buf, int R /* promoted from
		ResponseRef */
							   ,
							   ...);

// A phrase enum value as an argument of construct_response().
constexpr auto responseArg(ArithmeticEnum auto phrase) { return *phrase; }
template <typename T>
	requires(uqstl::is_pointer_v<T> || uqstl::is_null_pointer_v<T> || uqstl::is_integral_v<T>)
constexpr auto responseArg(T arg)
{
	return arg;
}

template <ArithmeticEnum E, typename... Args>
void construct_response(uqstl::span<char> buf, E R, Args... args)
{
	construct_response(buf, *R, responseArg(args)...);
}

enum class Segue
{
	Peace,
	// When initiating a conversation, open comms directly.
	// When terminating a conversation, depart in peace.
	Hostile,
	// When initiating a conversation, offer the choice to attack.
	// When terminating a conversation, go into battle.
	Victory,
	// (when terminating a conversation) instant victory
	Defeat,
	// (when terminating a conversation) game over
};
consteval bool isArithmeticEnum(Segue) { return true; }

void setSegue(Segue segue);
Segue getSegue(void);

extern ResponseRef phraseIdStrToNum(const char* phraseId);
extern const char* phraseIdNumToStr(ResponseRef response);

extern ConversationData* init_race(Conversation comm_id);

extern ConversationData* init_arilou_comm(void);

extern ConversationData* init_blackurq_comm(void);

extern ConversationData* init_chmmr_comm(void);

extern ConversationData* init_commander_comm(void);

extern ConversationData* init_druuge_comm(void);

extern ConversationData* init_ilwrath_comm(void);

extern ConversationData* init_melnorme_comm(void);

extern ConversationData* init_mycon_comm(void);

extern ConversationData* init_orz_comm(void);

extern ConversationData* init_pkunk_comm(void);

extern ConversationData* init_rebel_yehat_comm(void);

extern ConversationData* init_shofixti_comm(void);

extern ConversationData* init_slyland_comm(void);

extern ConversationData* init_slylandro_comm(void);

extern ConversationData* init_spahome_comm(void);

extern ConversationData* init_spathi_comm(void);

extern ConversationData* init_starbase_comm(void);

extern ConversationData* init_supox_comm(void);

extern ConversationData* init_syreen_comm(void);

extern ConversationData* init_talkpet_comm(void);

extern ConversationData* init_thradd_comm(void);

extern ConversationData* init_umgah_comm(void);

extern ConversationData* init_urquan_comm(void);

extern ConversationData* init_utwig_comm(void);

extern ConversationData* init_vux_comm(void);

extern ConversationData* init_yehat_comm(void);

extern ConversationData* init_zoqfot_comm(void);

extern ConversationData* init_umgah_comm(void);

inline constexpr auto MaxInterpolate {8192}; // the max length we can robo-interpolate
inline constexpr auto MaxClipname {128};	 // e.g. "pkunk-004c.ogg"
inline constexpr auto NumRoboTracks {11};	 // Need 11 for getPoint (8 digits, 2 points, 1 by)

enum class RobotPhrase : uint16_t
{
	NullPhrase,
	Digit0,
	Digit1,
	Digit2,
	Digit3,
	Digit4,
	Digit5,
	Digit6,
	Digit7,
	Digit8,
	Digit9,
	Point,
	By,
	ColorRed,
	ColorOrange,
	ColorYellow,
	ColorGreen,
	ColorBlue,
	ColorWhite,
	IlwrathColorRed,
	IlwrathColorOrange,
	IlwrathColorYellow,
	IlwrathColorGreen,
	IlwrathColorBlue,
	IlwrathColorWhite,
	Prefix0, // Unused but easier
	Alpha,
	Beta,
	Gamma,
	Delta,
	Epsilon,
	Zeta,
	Eta,
	Theta,
	Iota,
	Kappa,
	Lambda,
	Mu,
	Nu,
	Xi,
	Postfix0,
	Postfix1,
	Postfix2,
	Postfix3,
	Postfix4,
	Postfix5,
	Postfix6,
	Postfix7,
	Postfix8,
	Postfix9,
	Postfix10,
	Postfix11,
	Postfix12,
	Postfix13,
	Postfix14,
	Postfix15,
	Postfix16,
	Postfix17,
	Postfix18,
	Postfix19,
	Postfix20,
	Postfix21,
	Postfix22,
	Postfix23,
	Postfix24,
	Postfix25,
	Postfix26,
	Postfix27,
	Postfix28,
	Postfix29,
	Postfix30,
	Postfix31,
	Postfix32,
	Postfix33,
	Postfix34,
	Postfix35,
	Postfix36,
	Postfix37,
	Postfix38,
	Postfix39,
	Postfix40,
	Postfix41,
	Postfix42,
	Postfix43,
	Postfix44,
	Postfix45,
	Postfix46,
	Postfix47,
	Postfix48,
	Postfix49,
	Postfix50,
	Postfix51,
	Postfix52,
	Postfix53,
	Postfix54,
	Postfix55,
	Postfix56,
	Postfix57,
	Postfix58,
	Postfix59,
	Postfix60,
	Postfix61,
	Postfix62,
	Postfix63,
	Postfix64,
	Postfix65,
	Postfix66,
	Postfix67,
	Postfix68,
	Postfix69,
	Postfix70,
	Postfix71,
	Postfix72,
	Postfix73,
	Postfix74,
	Postfix75,
	Postfix76,
	Postfix77,
	Postfix78,
	Postfix79,
	Postfix80,
	Postfix81,
	Postfix82,
	Postfix83,
	Postfix84,
	Postfix85,
	Postfix86,
	Postfix87,
	Postfix88,
	Postfix89,
	Postfix90,
	Postfix91,
	Postfix92,
	Postfix93,
	Postfix94,
	Postfix95,
	Postfix96,
	Postfix97,
	Postfix98,
	Postfix99,
	Postfix100,
	Postfix101,
	Postfix102,
	Postfix103,
	Postfix104,
	Postfix105,
	Postfix106,
	Postfix107,
	Postfix108,
	Postfix109,
	Postfix110,
	Postfix111,
	Postfix112,
	Postfix113,
	Postfix114,
	Postfix115,
	Postfix116,
	Postfix117,
	Postfix118,
	Postfix119,
	Postfix120,
	Postfix121,
	Postfix122,
	Postfix123,
	Postfix124,
	Postfix125,
	Postfix126,
	Postfix127,
	Postfix128,
	Postfix129,
	Postfix130,
	Postfix131,

	// A sub-clip with a name from the primary clip.
	SubClip = 0xFFFF,
};
consteval bool isArithmeticEnum(RobotPhrase) { return true; }

} // namespace uqm

#endif // UQM_COMMGLUE_H
