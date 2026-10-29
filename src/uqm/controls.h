//Copyright Paul Reiche, Fred Ford. 1992-2002#include "libs/time/timelib.h"

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

#ifndef UQM_CONTROLS_H_
#define UQM_CONTROLS_H_

#include "libs/compiler.h"
#include "libs/strlib.h"
#include "libs/time/timelib.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

// Enumerated type for controls
enum
{
	KEY_UP,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_WEAPON,
	KEY_SPECIAL,
	KEY_ESCAPE,
	KEY_THRUST,
	NUM_KEYS
};
enum
{
	KEY_PAUSE,
	KEY_EXIT,
	KEY_ABORT,
	KEY_DEBUG,
	KEY_FULLSCREEN,
	KEY_MENU_UP,
	KEY_MENU_DOWN,
	KEY_MENU_LEFT,
	KEY_MENU_RIGHT,
	KEY_MENU_SELECT,
	KEY_MENU_CANCEL,
	KEY_MENU_SPECIAL,
	KEY_MENU_PAGE_UP,
	KEY_MENU_PAGE_DOWN,
	KEY_MENU_HOME,
	KEY_MENU_END,
	KEY_MENU_ZOOM_IN,
	KEY_MENU_ZOOM_OUT,
	KEY_MENU_DELETE,
	KEY_MENU_BACKSPACE,
	KEY_MENU_EDIT_CANCEL,
	KEY_MENU_SEARCH,
	KEY_MENU_NEXT,
	KEY_MENU_TOGGLEMAP,
	KEY_SCREENSHOT,
	KEY_DEBUG_2,
	KEY_DEBUG_3,
	KEY_DEBUG_4,
	KEY_MENU_ANY, /* abstract char key */
	NUM_MENU_KEYS
};

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


struct CONTROLLER_INPUT_STATE
{
	int key[static_cast<int>(ControlTemplate::NUM)][NUM_KEYS];
	int menu[NUM_MENU_KEYS];

	// TODO: need to define this because of volatile ImmediateInputState variable!? Figure this out. This is ugly
	CONTROLLER_INPUT_STATE& operator=(const volatile CONTROLLER_INPUT_STATE& rhs)
	{
		for (int t = 0; t < static_cast<int>(ControlTemplate::NUM); ++t)
		{
			for (int k = 0; k < NUM_KEYS; ++k)
			{
				key[t][k] = rhs.key[t][k];
			}
		}

		for (int mk = 0; mk < NUM_MENU_KEYS; ++mk)
		{
			menu[mk] = rhs.menu[mk];
		}

		return *this;
	}
};

typedef uint8_t BATTLE_INPUT_STATE;
#define BATTLE_LEFT ((BATTLE_INPUT_STATE)(1 << 0))
#define BATTLE_RIGHT ((BATTLE_INPUT_STATE)(1 << 1))
#define BATTLE_THRUST ((BATTLE_INPUT_STATE)(1 << 2))
#define BATTLE_WEAPON ((BATTLE_INPUT_STATE)(1 << 3))
#define BATTLE_SPECIAL ((BATTLE_INPUT_STATE)(1 << 4))
#define BATTLE_ESCAPE ((BATTLE_INPUT_STATE)(1 << 5))
#define BATTLE_DOWN ((BATTLE_INPUT_STATE)(1 << 6))
#define BATTLE_THRUST_ALT ((BATTLE_INPUT_STATE)(1 << 7))

BATTLE_INPUT_STATE CurrentInputToBattleInput(uint16_t player);
BATTLE_INPUT_STATE PulsedInputToBattleInput(uint16_t player);

extern CONTROLLER_INPUT_STATE CurrentInputState;
extern CONTROLLER_INPUT_STATE PulsedInputState;
extern volatile CONTROLLER_INPUT_STATE ImmediateInputState;
extern ControlTemplate PlayerControlTemplates[];
extern bool WarpFromMenu;

void UpdateInputState(void);
// Runs a script of menu key presses. Returns false if the file cannot be read or parsed.
bool LoadInputScript(const char* path);
// The seed of a deterministic run, or nullopt. Call after LoadInputScript.
uqstl::optional<uint32_t> GetDeterministicSeed(void);
// Return typed characters, like GetNextCharacter and GetLastCharacter, from the input script when it has recorded frames.
UniChar GetNextInputCharacter(void);
UniChar GetLastInputCharacter(void);
// Sets the function that the "dumpmenu" script command calls. Null when no setup menu is open.
void SetInputScriptMenuDump(void (*dump)(void));
extern void FlushInput(void);
void SetMenuRepeatDelay(uint32_t min, uint32_t max, uint32_t step, bool gestalt);
void SetDefaultMenuRepeatDelay(void);
void ResetKeyRepeat(void);
bool PauseGame(void);
void SleepGame(void);
bool DoConfirmExit(void);
bool ConfirmExit(void);

#define WAIT_INFINITE ((TimePeriod) - 1)
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
// pInputState must point to a struct derived from INPUT_STATE_DESC
void DoInput(void* pInputState, bool resetInput);

extern volatile bool GamePaused;
extern volatile bool ExitRequested;

typedef struct joy_char joy_char_t;

typedef struct textentry_state
{
	// standard state required by DoInput
	bool (*InputFunc)(struct textentry_state* pTES);

	// these are semi-private read-only
	bool Initialized;
	uint32_t NextTime;	   // use this for input frame timing
	bool Success;		   // edit confirmed or canceled
	char* CacheStr;		   // cached copy to revert immediate changes
	STRING JoyAlphaString; // joystick alphabet definition
	bool JoystickMode;	   // true when doing joystick input
	bool UpperRegister;	   // true when entering Caps
	joy_char_t* JoyAlpha;  // joystick alphabet
	int JoyAlphaLength;
	joy_char_t* JoyUpper; // joystick upper register
	joy_char_t* JoyLower; // joystick lower register
	int JoyRegLength;
	char* InsPt; // set to current pos of insertion point
	// these are public and must be set before calling DoTextEntry
	char* BaseStr; // set to string to edit
	int CursorPos; // set to current cursor pos in chars
	int MaxSize;   // set to max size of edited string

	bool (*ChangeCallback)(struct textentry_state* pTES);
	// returns true if last change is OK
	bool (*FrameCallback)(struct textentry_state* pTES);
	// called on every input frame; do whatever;
	// returns true to continue processing
	void* CbParam; // callback parameter, use as you like

} TEXTENTRY_STATE;

extern bool DoTextEntry(TEXTENTRY_STATE* pTES);

extern void TestSpeechSound(STRING snd);

#if 0 //defined(__cplusplus)
}
#endif

#endif
