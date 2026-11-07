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

#include <stdlib.h>

#include "battlecontrols.h"
#include "controls.h"
#include "core/log/log.h"
#include "core/stl/atomic.h"
#include "core/stl/chrono.h"
#include "core/stl/mutex.h"
#include "init.h"
#include "intel.h"
#include "libs/file/textfile.h"
#include "uqm/inputscript.h"
// For computer_intelligence
#ifdef NETPLAY
#include "supermelee/netplay/netmelee.h"
#endif
#include "libs/callback/async.h"
#include "libs/graphics/gfx_common.h"
#include "libs/inplib.h"
#include "libs/input/sdl/vcontrol.h"
#include "libs/time/timelib.h"
#include "save.h"
#include "settings.h"
#include "setup.h"
#include "setupmenu.h"
#include "sounds.h"
#include "starcon.h"
#include "tactrans.h"
#include "uqmdebug.h"

namespace uqm
{
static constexpr auto AccelerationIncrement {c_gameSecond / 12};
static constexpr auto MenuRepeatDelay {c_gameSecond / 2};

struct InputStateDesc
{
	bool (*InputFunc)(void* pInputState);
};

// These static variables are the values that are set by the controllers.

template <typename T>
struct MenuAnnotations
{
	uqstl::array<uqstl::array<T, *BattleKey::NumKeys>, uqstl::to_underlying(ControlTemplate::NUM)> key;
	uqstl::array<T, *MenuKey::NumMenuKeys> menu;
};


uqstl::array<ControlTemplate, NumPlayers> PlayerControlTemplates;
ControllerInputState CurrentInputState, PulsedInputState;
static ControllerInputState CachedInputState, OldInputState;
static MenuAnnotations<TimePeriod> RepeatDelays;
static MenuAnnotations<TimeCount> Times;
static TimePeriod GestaltRepeatDelay;
static TimeCount GestaltTime;
static bool OldGestalt, CachedGestalt;
static TimePeriod _max_accel, _min_accel, _step_accel;
static bool _gestalt_keys;

static MenuSoundFlags sound_0, sound_1;

ImmediateKeyState ImmediateInputState {};

uqstl::atomic<bool> ExitRequested {};
uqstl::atomic<bool> GamePaused {};
uqstl::atomic<bool> OnScreenKeyboardLocked {};

static InputFrameCallback* inputCallback;

static uqstl::optional<uqm::InputScript> inputScript {};
static void (*inputScriptMenuDump)(void) {};
static bool inputScriptFinishedLogged {};
static uqstl::mutex inputScriptMutex {};
static uqstl::chrono::steady_clock::time_point inputScriptStart {};

bool LoadInputScript(const char* path)
{
	const auto text {uqm::readTextFile(path)};
	if (!text)
	{
		uqm::log::error("Input script: cannot open '{}'.", path);
		return false;
	}

	uqstl::string error {};
	inputScript = uqm::InputScript::parse(*text, error);
	if (!inputScript)
	{
		uqm::log::error("Input script '{}': {}", path, error);
		return false;
	}
	uqm::log::info("Input script: running '{}' ({} commands).", path, inputScript->commands().size());
	inputScriptStart = uqstl::chrono::steady_clock::now();
	return true;
}

void SetInputScriptMenuDump(void (*dump)(void))
{
	inputScriptMenuDump = dump;
}

static int scriptMenuKey(uqm::ScriptKey key)
{
	switch (key)
	{
		case uqm::ScriptKey::Up:
			return *MenuKey::MenuUp;
		case uqm::ScriptKey::Down:
			return *MenuKey::MenuDown;
		case uqm::ScriptKey::Left:
			return *MenuKey::MenuLeft;
		case uqm::ScriptKey::Right:
			return *MenuKey::MenuRight;
		case uqm::ScriptKey::Select:
			return *MenuKey::MenuSelect;
		case uqm::ScriptKey::Cancel:
			return *MenuKey::MenuCancel;
		case uqm::ScriptKey::Special:
			return *MenuKey::MenuSpecial;
		case uqm::ScriptKey::Delete:
			return *MenuKey::MenuDelete;
		case uqm::ScriptKey::Backspace:
			return *MenuKey::MenuBackspace;
		case uqm::ScriptKey::PageUp:
			return *MenuKey::MenuPageUp;
		case uqm::ScriptKey::PageDown:
			return *MenuKey::MenuPageDown;
		case uqm::ScriptKey::Home:
			return *MenuKey::MenuHome;
		case uqm::ScriptKey::End:
			return *MenuKey::MenuEnd;
		case uqm::ScriptKey::Screenshot:
			return *MenuKey::Screenshot;
	}
	return *MenuKey::MenuSelect;
}

// Adds the scripted key, if any, to the input of this frame.
static void applyInputScript(void)
{
	const uqstl::lock_guard lock {inputScriptMutex};
	const auto now {uqstl::chrono::duration_cast<uqstl::chrono::milliseconds>(uqstl::chrono::steady_clock::now() - inputScriptStart).count()};
	const uqm::InputScript::Step step {inputScript->update(static_cast<uint32_t>(now))};

	if (step.press == uqm::ScriptKey::Screenshot)
	{
		// The main thread takes screenshots from the immediate input state.
		ImmediateInputState.menu[*MenuKey::Screenshot] = 1;
	}
	else if (step.press)
	{
		const int key {scriptMenuKey(*step.press)};
		CurrentInputState.menu[key] = 1;
		CachedInputState.menu[key] = 1;
	}
	if (step.dumpMenu)
	{
		if (inputScriptMenuDump)
		{
			inputScriptMenuDump();
		}
		else
		{
			uqm::log::warn("Input script: dumpmenu: no setup menu is open.");
		}
	}
	if (step.log)
	{
		uqm::log::info("Input script: {}", *step.log);
	}
	if (step.saveSlot)
	{
		SaveSummaryData summary {};
		if (!IsGameInProgress())
		{
			uqm::log::error("Input script: no game runs, so there is no game to save.");
		}
		else if (SaveGame(*step.saveSlot, &summary, "Input script"))
		{
			uqm::log::info("Input script: saved the game to slot {}.", *step.saveSlot);
		}
		else
		{
			uqm::log::error("Input script: could not save the game to slot {}.", *step.saveSlot);
		}
	}
	if (step.quit)
	{
		uqm::log::info("Input script: quit.");
		uqm::log::default_logger()->flush();
		// The main thread stops the game thread and then exits, as when the window closes.
		QuitPosted = 1;
	}
	if (inputScript->finished() && !inputScriptFinishedLogged)
	{
		uqm::log::info("Input script: finished.");
		inputScriptFinishedLogged = true;
	}
}

static void _clear_menu_state(void)
{
	int i {};
	int j {};
	for (i = 0; i < uqstl::to_underlying(ControlTemplate::NUM); i++)
	{
		for (j = 0; j < BattleKey::NumKeys; j++)
		{
			PulsedInputState.key[i][j] = 0;
			CachedInputState.key[i][j] = 0;
		}
	}
	for (i = 0; i < MenuKey::NumMenuKeys; i++)
	{
		PulsedInputState.menu[i] = 0;
		CachedInputState.menu[i] = 0;
	}
	CachedGestalt = false;
}

void ResetKeyRepeat(void)
{
	const TimeCount initTime {GetTimeCounter()};
	int i {};
	int j {};
	for (i = 0; i < uqstl::to_underlying(ControlTemplate::NUM); i++)
	{
		for (j = 0; j < BattleKey::NumKeys; j++)
		{
			RepeatDelays.key[i][j] = _max_accel;
			Times.key[i][j] = initTime;
		}
	}
	for (i = 0; i < MenuKey::NumMenuKeys; i++)
	{
		RepeatDelays.menu[i] = _max_accel;
		Times.menu[i] = initTime;
	}
	GestaltRepeatDelay = _max_accel;
	GestaltTime = initTime;
}

static void _check_for_pulse(int* current, int* cached, int* old, TimePeriod* accel,
							 TimeCount* newtime, TimeCount* oldtime)
{
	if (*cached && *old)
	{
		if (*newtime - *oldtime < *accel)
		{
			*current = 0;
		}
		else
		{
			*current = *cached;
			if (*accel > _min_accel)
			{
				*accel -= _step_accel;
			}
			if (*accel < _min_accel)
			{
				*accel = _min_accel;
			}
			*oldtime = *newtime;
		}
	}
	else
	{
		*current = *cached;
		*oldtime = *newtime;
		*accel = _max_accel;
	}
}

/* BUG: If a key from a currently unused control template is held,
 * this will affect the gestalt repeat rate.  This isn't a problem
 * *yet*, but it will be once the user gets to define control
 * templates on his own --McM */
static void _check_gestalt(TimeCount NewTime)
{
	int i {};
	int j {};
	OldGestalt = CachedGestalt;

	CachedGestalt = 0;
	bool CurrentGestalt {0};
	for (i = 0; i < uqstl::to_underlying(ControlTemplate::NUM); i++)
	{
		for (j = 0; j < BattleKey::NumKeys; j++)
		{
			CachedGestalt |= (bool)ImmediateInputState.key[i][j];
			CurrentGestalt |= (bool)PulsedInputState.key[i][j];
		}
	}
	for (i = 0; i < MenuKey::NumMenuKeys; i++)
	{
		CachedGestalt |= (bool)ImmediateInputState.menu[i];
		CurrentGestalt |= (bool)PulsedInputState.menu[i];
	}

	if (OldGestalt && CachedGestalt)
	{
		if (NewTime - GestaltTime < GestaltRepeatDelay)
		{
			for (i = 0; i < PulsedInputState.key.size(); i++)
			{
				for (j = 0; j < BattleKey::NumKeys; j++)
				{
					PulsedInputState.key[i][j] = 0;
				}
			}
			for (i = 0; i < MenuKey::NumMenuKeys; i++)
			{
				PulsedInputState.menu[i] = 0;
			}
		}
		else
		{
			for (i = 0; i < uqstl::to_underlying(ControlTemplate::NUM); i++)
			{
				for (j = 0; j < BattleKey::NumKeys; j++)
				{
					PulsedInputState.key[i][j] = CachedInputState.key[i][j];
				}
			}
			for (i = 0; i < MenuKey::NumMenuKeys; i++)
			{
				PulsedInputState.menu[i] = CachedInputState.menu[i];
			}
			if (GestaltRepeatDelay > _min_accel)
			{
				GestaltRepeatDelay -= _step_accel;
			}
			if (GestaltRepeatDelay < _min_accel)
			{
				GestaltRepeatDelay = _min_accel;
			}
			GestaltTime = NewTime;
		}
	}
	else
	{
		for (i = 0; i < uqstl::to_underlying(ControlTemplate::NUM); i++)
		{
			for (j = 0; j < BattleKey::NumKeys; j++)
			{
				PulsedInputState.key[i][j] = CachedInputState.key[i][j];
			}
		}
		for (i = 0; i < MenuKey::NumMenuKeys; i++)
		{
			PulsedInputState.menu[i] = CachedInputState.menu[i];
		}
		GestaltTime = NewTime;
		GestaltRepeatDelay = _max_accel;
	}
}

void UpdateInputState(void)
{
	TimeCount NewTime;
	/* First, if the game is, in fact, paused, we stall until
	 * unpaused.  Every thread with control over game logic calls
	 * UpdateInputState routinely, so we handle pause and exit
	 * state updates here. */

	// Automatically pause and enter low-activity state while inactive,
	// for example, window minimized.
	if (!GameActive)
	{
		SleepGame();
	}

	if (GamePaused)
	{
		PauseGame();
	}

	if (ExitRequested)
	{
		ConfirmExit();
	}

	CurrentInputState = ImmediateInputState;
	OldInputState = CachedInputState;
	CachedInputState = ImmediateInputState;
	if (inputScript)
	{
		applyInputScript();
	}
	BeginInputFrame();
	NewTime = GetTimeCounter();
	if (_gestalt_keys)
	{
		_check_gestalt(NewTime);
	}
	else
	{
		int i {};
		int j {};
		for (i = 0; i < uqstl::to_underlying(ControlTemplate::NUM); i++)
		{
			for (j = 0; j < BattleKey::NumKeys; j++)
			{
				_check_for_pulse(&PulsedInputState.key[i][j],
								 &CachedInputState.key[i][j],
								 &OldInputState.key[i][j], &RepeatDelays.key[i][j],
								 &NewTime, &Times.key[i][j]);
			}
		}
		for (i = 0; i < MenuKey::NumMenuKeys; i++)
		{
			_check_for_pulse(&PulsedInputState.menu[i],
							 &CachedInputState.menu[i], &OldInputState.menu[i],
							 &RepeatDelays.menu[i], &NewTime, &Times.menu[i]);
		}
	}

	if (CurrentInputState.menu[*MenuKey::Pause])
	{
		GamePaused = true;
	}

	if (CurrentInputState.menu[*MenuKey::Exit])
	{
		ExitRequested = true;
	}

#if defined(DEBUG) || defined(USE_DEBUG_KEY)
	if (PulsedInputState.menu[*MenuKey::Debug])
	{
		debugKeyPressedSynchronous();
	}
	if (PulsedInputState.menu[*MenuKey::Debug2])
	{
		debugKey2PressedSynchronous();
	}
	if (PulsedInputState.menu[*MenuKey::Debug3])
	{
		debugKey3PressedSynchronous();
	}
	if (PulsedInputState.menu[*MenuKey::Debug4])
	{
		debugKey4PressedSynchronous();
	}
#endif
}

InputFrameCallback* SetInputCallback(InputFrameCallback* callback)
{
	InputFrameCallback* old = inputCallback;

	// Replacing an existing callback with another is not a problem,
	// but currently this should never happen, which is why the assert.
	assert(!old || !callback);
	inputCallback = callback;

	return old;
}

void SetMenuRepeatDelay(TimePeriod min, TimePeriod max, TimePeriod step, bool gestalt)
{
	_min_accel = min;
	_max_accel = max;
	_step_accel = step;
	_gestalt_keys = gestalt;
	//_clear_menu_state ();
	ResetKeyRepeat();
}

void SetDefaultMenuRepeatDelay(void)
{
	_min_accel = AccelerationIncrement;
	_max_accel = MenuRepeatDelay;
	_step_accel = AccelerationIncrement;
	_gestalt_keys = false;
	//_clear_menu_state ();
	ResetKeyRepeat();
}

void FlushInput(void)
{
	TFB_ResetControls();
	_clear_menu_state();
}

static MenuSoundFlags MenuKeysToSoundFlags(const ControllerInputState* state)
{
	MenuSoundFlags soundFlags {MenuSoundFlags::None};
	if (state->menu[*MenuKey::MenuUp])
	{
		soundFlags |= MenuSoundFlags::Up;
	}
	if (state->menu[*MenuKey::MenuDown])
	{
		soundFlags |= MenuSoundFlags::Down;
	}
	if (state->menu[*MenuKey::MenuLeft])
	{
		soundFlags |= MenuSoundFlags::Left;
	}
	if (state->menu[*MenuKey::MenuRight])
	{
		soundFlags |= MenuSoundFlags::Right;
	}
	if (state->menu[*MenuKey::MenuSelect])
	{
		soundFlags |= MenuSoundFlags::Select;
	}
	if (state->menu[*MenuKey::MenuCancel])
	{
		soundFlags |= MenuSoundFlags::Cancel;
	}
	if (state->menu[*MenuKey::MenuSpecial])
	{
		soundFlags |= MenuSoundFlags::Special;
	}
	if (state->menu[*MenuKey::MenuPageUp])
	{
		soundFlags |= MenuSoundFlags::PageUp;
	}
	if (state->menu[*MenuKey::MenuPageDown])
	{
		soundFlags |= MenuSoundFlags::PageDown;
	}
	if (state->menu[*MenuKey::MenuDelete])
	{
		soundFlags |= MenuSoundFlags::Delete;
	}
	if (state->menu[*MenuKey::MenuBackspace])
	{
		soundFlags |= MenuSoundFlags::Delete;
	}

	return soundFlags;
}

void DoInput(void* pInputState, bool resetInput)
{
	if (resetInput)
	{
		FlushInput();
	}

	do
	{
		Async_process();
		TaskSwitch();

		UpdateInputState();

		const MenuSoundFlags soundFlags {MenuKeysToSoundFlags(&PulsedInputState)};

		if (MenuSounds && testFlag(soundFlags, sound_0 | sound_1))
		{
			const SoundRef S {GetSound(MenuSounds, testFlag(soundFlags, sound_1) ? MenuSoundEffect::Success : MenuSoundEffect::Move)};

			PlaySoundEffect(S, 0, NotPositional(), nullptr, 0);
		}

		if (inputCallback)
		{
			inputCallback();
		}

	} while (((InputStateDesc*)pInputState)->InputFunc(pInputState));

	if (resetInput)
	{
		FlushInput();
	}
}

void SetMenuSounds(MenuSoundFlags s0, MenuSoundFlags s1)
{
	sound_0 = s0;
	sound_1 = s1;
}

void GetMenuSounds(MenuSoundFlags* s0, MenuSoundFlags* s1)
{
	*s0 = sound_0;
	*s1 = sound_1;
}

static BattleInputState ControlInputToBattleInput(const int* keyState)
{
	BattleInputState InputState {};

	if (keyState[*BattleKey::Up])
	{
		InputState |= BattleInputState::Thrust;
	}
	if (keyState[*BattleKey::Left])
	{
		InputState |= BattleInputState::Left;
	}
	if (keyState[*BattleKey::Right])
	{
		InputState |= BattleInputState::Right;
	}
	if (keyState[*BattleKey::Weapon])
	{
		InputState |= BattleInputState::Weapon;
	}
	if (keyState[*BattleKey::Special])
	{
		InputState |= BattleInputState::Special;
	}
	if (keyState[*BattleKey::Escape])
	{
		InputState |= BattleInputState::Escape;
	}
	if (keyState[*BattleKey::Down])
	{
		InputState |= BattleInputState::Down;
	}
	if (keyState[*BattleKey::Thrust])
	{
		InputState |= BattleInputState::ThrustAlt;
	}

	return InputState;
}

BattleInputState CurrentInputToBattleInput(uint16_t player)
{
	return ControlInputToBattleInput(
		CurrentInputState.key[static_cast<int>(PlayerControlTemplates[player])].data());
}

BattleInputState PulsedInputToBattleInput(uint16_t player)
{
	return ControlInputToBattleInput(
		PulsedInputState.key[static_cast<int>(PlayerControlTemplates[player])].data());
}

bool AnyButtonPress(bool CheckSpecial)
{
	int i {};
	int j {};
	(void)CheckSpecial; // Ignored
	UpdateInputState();
	for (i = 0; i < CurrentInputState.key.size(); i++)
	{
		for (j = 0; j < BattleKey::NumKeys; j++)
		{
			if (CurrentInputState.key[i][j])
			{
				return true;
			}
		}
	}
	for (i = 0; i < MenuKey::NumMenuKeys; i++)
	{
		if (CurrentInputState.menu[i])
		{
			return true;
		}
	}
	return false;
}

bool DirKeysPress(void)
{
	return (
		CurrentInputState.menu[*MenuKey::MenuLeft] || CurrentInputState.menu[*MenuKey::MenuRight] || CurrentInputState.menu[*MenuKey::MenuUp] || CurrentInputState.menu[*MenuKey::MenuDown]);
}

bool ActKeysPress(void)
{
	UpdateInputState();

	return (
		CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][*BattleKey::Weapon]
		|| CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][*BattleKey::Special]
		|| CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][*BattleKey::Escape]
		|| CurrentInputState.menu[*MenuKey::MenuSelect]
		|| CurrentInputState.menu[*MenuKey::MenuCancel]
		|| CurrentInputState.menu[*MenuKey::MenuSpecial]);
}

bool ConfirmExit(void)
{
	TimePeriod old_max_accel;
	TimePeriod old_min_accel;
	TimePeriod old_step_accel;

	old_max_accel = _max_accel;
	old_min_accel = _min_accel;
	old_step_accel = _step_accel;
	const bool old_gestalt_keys {_gestalt_keys};

	SetDefaultMenuRepeatDelay();

	const bool result {DoConfirmExit()};

	SetMenuRepeatDelay(old_min_accel, old_max_accel, old_step_accel,
					   old_gestalt_keys);
	return result;
}

void TestSpeechSound(SoundRef snd)
{
	PlaySpeechEffect(snd, NotPositional(), nullptr, 0);
}

} // namespace uqm