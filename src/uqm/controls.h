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
#ifndef UQM_CONTROLS_H
#define UQM_CONTROLS_H

#include "core/types/enum.h"
#include "core/stl/atomic.h"

#include "core/misc/bittricks.h"
#include "libs/strlib.h"
#include "libs/sndlib.h"
#include "core/stl/vector.h"
#include "libs/time/timelib.h"

namespace uqm
{
#if 0 //defined(__cplusplus)
extern "C" {
#endif

// Enumerated type for controls
enum class BattleKey
{
	Up,
	Down,
	Left,
	Right,
	Weapon,
	Special,
	Escape,
	Thrust,
	NumKeys
};
consteval bool isArithmeticEnum(BattleKey) { return true; }
enum class MenuKey
{
	Pause,
	Exit,
	Abort,
	Debug,
	Fullscreen,
	MenuUp,
	MenuDown,
	MenuLeft,
	MenuRight,
	MenuSelect,
	MenuCancel,
	MenuSpecial,
	MenuPageUp,
	MenuPageDown,
	MenuHome,
	MenuEnd,
	MenuZoomIn,
	MenuZoomOut,
	MenuDelete,
	MenuBackspace,
	MenuEditCancel,
	MenuSearch,
	MenuNext,
	MenuTogglemap,
	Screenshot,
	Debug2,
	Debug3,
	Debug4,
	MenuAny, /* abstract char key */
	NumMenuKeys
};
consteval bool isArithmeticEnum(MenuKey) { return true; }

enum class ControlTemplate
{
	KB_1,
	KB_2,
	KB_3,
	JOY_1,
	JOY_2,
	JOY_3,
	NUM
};

static constexpr const char* toString(ControlTemplate val)
{
	switch (val)
	{
		case ControlTemplate::KB_1:
			return "Keyboard 1";
		case ControlTemplate::KB_2:
			return "Keyboard 2";
		case ControlTemplate::KB_3:
			return "Keyboard 3";
		case ControlTemplate::JOY_1:
			return "Joy 1";
		case ControlTemplate::JOY_2:
			return "Joy 2";
		case ControlTemplate::JOY_3:
			return "Joy 3";
		case ControlTemplate::NUM:
		default:
			return "???";
	};
}


// The input that the main thread writes while the game thread reads it.
struct ImmediateKeyState
{
	uqstl::atomic<int> key[uqstl::to_underlying(ControlTemplate::NUM)][*BattleKey::NumKeys];
	uqstl::atomic<int> menu[*MenuKey::NumMenuKeys];
};

struct ControllerInputState
{
	int key[uqstl::to_underlying(ControlTemplate::NUM)][*BattleKey::NumKeys];
	int menu[*MenuKey::NumMenuKeys];

	ControllerInputState& operator=(const ImmediateKeyState& rhs)
	{
		for (int t = 0; t < uqstl::to_underlying(ControlTemplate::NUM); ++t)
		{
			for (int k = 0; k < BattleKey::NumKeys; ++k)
			{
				key[t][k] = rhs.key[t][k];
			}
		}

		for (int mk = 0; mk < MenuKey::NumMenuKeys; ++mk)
		{
			menu[mk] = rhs.menu[mk];
		}

		return *this;
	}
};

typedef uint8_t BATTLE_INPUT_STATE; // TODO PragmaNull : replace with falg enum.
inline constexpr auto BattleLeft {(BATTLE_INPUT_STATE)(1 << 0)};
inline constexpr auto BattleRight {(BATTLE_INPUT_STATE)(1 << 1)};
inline constexpr auto BattleThrust {(BATTLE_INPUT_STATE)(1 << 2)};
inline constexpr auto BattleWeapon {(BATTLE_INPUT_STATE)(1 << 3)};
inline constexpr auto BattleSpecial {(BATTLE_INPUT_STATE)(1 << 4)};
inline constexpr auto BattleEscape {(BATTLE_INPUT_STATE)(1 << 5)};
inline constexpr auto BattleDown {(BATTLE_INPUT_STATE)(1 << 6)};
inline constexpr auto BattleThrustAlt {(BATTLE_INPUT_STATE)(1 << 7)};

BATTLE_INPUT_STATE CurrentInputToBattleInput(uint16_t player);
BATTLE_INPUT_STATE PulsedInputToBattleInput(uint16_t player);

extern ControllerInputState CurrentInputState;
extern ControllerInputState PulsedInputState;
extern ImmediateKeyState ImmediateInputState;
extern ControlTemplate PlayerControlTemplates[];
extern bool WarpFromMenu;

void UpdateInputState(void);
// Runs a script of menu key presses. Returns false if the file cannot be read or parsed.
bool LoadInputScript(const char* path);
// Sets the function that the "dumpmenu" script command calls. Null when no setup menu is open.
void SetInputScriptMenuDump(void (*dump)(void));
extern void FlushInput(void);
void SetMenuRepeatDelay(TimePeriod min, TimePeriod max, TimePeriod step, bool gestalt);
void SetDefaultMenuRepeatDelay(void);
void ResetKeyRepeat(void);
bool PauseGame(void);
void SleepGame(void);
bool DoConfirmExit(void);
bool ConfirmExit(void);

inline constexpr auto WaitInfinite {TimePeriod::max()};
bool WaitForAnyButton(bool newButton, TimePeriod duration,
					  bool resetInput);
bool WaitForActButton(bool newButton, TimePeriod duration,
					  bool resetInput);
bool WaitForAnyButtonUntil(bool newButton, TimeCount timeOut,
						   bool resetInput);
bool WaitForActButtonUntil(bool newButton, TimeCount timeOut,
						   bool resetInput);
bool WaitForNoInput(TimePeriod duration, bool resetInput);
bool WaitForNoInputUntil(TimeCount timeOut, bool resetInput);

extern BATTLE_INPUT_STATE GetDirectionalJoystickInput(int direction, int player);

void DoPopupWindow(const char* msg);

typedef void(InputFrameCallback)(void);
InputFrameCallback* SetInputCallback(InputFrameCallback*);
// pInputState must point to a struct derived from InputStateDesc
void DoInput(void* pInputState, bool resetInput);

extern uqstl::atomic<bool> GamePaused;
extern uqstl::atomic<bool> ExitRequested;

// A UTF-8 character in the joystick alphabet.
struct joy_char_t
{
	unsigned char len;
	char enc[7];
};

struct TextEntryState
{
	// standard state required by DoInput
	bool (*InputFunc)(struct TextEntryState* pTES);

	// these are semi-private read-only
	bool Initialized;
	TimeCount NextTime;					// use this for input frame timing
	bool Success;						// edit confirmed or canceled
	uqstl::vector<char> CacheStr;		// cached copy to revert immediate changes
	bool JoystickMode;					// true when doing joystick input
	bool UpperRegister;					// true when entering Caps
	uqstl::vector<joy_char_t> JoyAlpha; // joystick alphabet
	int JoyAlphaLength;
	uqstl::vector<joy_char_t> JoyUpper; // joystick upper register
	uqstl::vector<joy_char_t> JoyLower; // joystick lower register
	int JoyRegLength;
	char* InsPt; // set to current pos of insertion point
	// these are public and must be set before calling DoTextEntry
	char* BaseStr; // set to string to edit
	int CursorPos; // set to current cursor pos in chars
	int MaxSize;   // set to max size of edited string

	bool (*ChangeCallback)(struct TextEntryState* pTES);
	// returns true if last change is OK
	bool (*FrameCallback)(struct TextEntryState* pTES);
	// called on every input frame; do whatever;
	// returns true to continue processing
	void* CbParam; // callback parameter, use as you like
};

extern bool DoTextEntry(TextEntryState* pTES);

extern void TestSpeechSound(SOUND snd);

#if 0 //defined(__cplusplus)
}
#endif

} // namespace uqm


#endif // UQM_CONTROLS_H
