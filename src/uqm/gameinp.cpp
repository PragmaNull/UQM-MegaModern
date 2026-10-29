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
#include "controls.h"
#include "core/log/log.h"
#include "uqm/inputscript.h"
#include <chrono>
#include <fstream>
#include <mutex>
#include <sstream>
#include "battlecontrols.h"
#include "init.h"
#include "intel.h"
// For computer_intelligence
#ifdef NETPLAY
#include "supermelee/netplay/netmelee.h"
#endif
#include "settings.h"
#include "sounds.h"
#include "tactrans.h"
#include "uqmdebug.h"
#include "globdata.h"
#include "libs/math/random.h"

#include <fmt/format.h>
#include "libs/async.h"
#include "libs/inplib.h"
#include "libs/time/timelib.h"
#include "libs/threadlib.h"
#include "libs/input/sdl/vcontrol.h"
#include "setup.h"
#include "setupmenu.h"
#include "libs/graphics/gfx_common.h"

#define ACCELERATION_INCREMENT (GameTicksPerSecond / 12)
#define MENU_REPEAT_DELAY (GameTicksPerSecond >> 1)

typedef struct
{
	bool (*InputFunc)(void* pInputState);
} INPUT_STATE_DESC;

// These static variables are the values that are set by the controllers.

typedef struct
{
	uint32_t key[static_cast<int>(ControlTemplate::NUM)][NUM_KEYS];
	uint32_t menu[NUM_MENU_KEYS];
} MENU_ANNOTATIONS;


ControlTemplate PlayerControlTemplates[NUM_PLAYERS];
CONTROLLER_INPUT_STATE CurrentInputState, PulsedInputState;
static CONTROLLER_INPUT_STATE CachedInputState, OldInputState;
static MENU_ANNOTATIONS RepeatDelays, Times;
static uint32_t GestaltRepeatDelay, GestaltTime;
static bool OldGestalt, CachedGestalt;
static uint32_t _max_accel, _min_accel, _step_accel;
static bool _gestalt_keys;

static MENU_SOUND_FLAGS sound_0, sound_1;

volatile CONTROLLER_INPUT_STATE ImmediateInputState;

volatile bool ExitRequested;
volatile bool GamePaused;
volatile bool OnScreenKeyboardLocked;

static InputFrameCallback* inputCallback;

static uqstl::optional<uqm::InputScript> inputScript {};
static void (*inputScriptMenuDump)(void) {};
static bool inputScriptFinishedLogged {};
static uqstl::mutex inputScriptMutex {};
static uqstl::chrono::steady_clock::time_point inputScriptStart {};
static uqstl::optional<TimeCount> inputScriptGameStart {};

static_assert(uqm::c_scriptMenuKeyCount == NUM_MENU_KEYS);
static_assert(uqm::c_scriptBattleKeyCount == NUM_KEYS);
static_assert(uqm::c_scriptTemplateCount == static_cast<int>(ControlTemplate::NUM));
// The number of UpdateInputState calls so far.
static uint64_t inputFrame {};
// The recorded characters of the current input frame.
static uqstl::vector<uint32_t> frameText {};
static size_t frameTextNext {};
static uint32_t frameLast {};

// Recording
static uqstl::ofstream recordFile {};
static uqstl::optional<uint32_t> recordSeed {};
// The input of the frame that is being recorded. Its characters arrive after UpdateInputState.
static uqstl::optional<uqm::FrameInput> recordPending {};
static uqm::FrameInput recordWritten {};
static uqstl::mutex recordMutex {};

// Input frames between state checks in a recording.
static constexpr uint64_t c_recordCheckInterval {120};
static bool stateMismatchReported {};

// Returns a hash of the game state that a replay must reproduce.
static uint64_t
computeStateHash(void)
{
	uint64_t hash {0xcbf29ce484222325};
	const auto add {[&hash](uint64_t value, int bytes) {
		for (int i {0}; i < bytes; ++i)
		{
			hash ^= (value >> (8 * i)) & 0xff;
			hash *= 0x100000001b3;
		}
	}};
	const auto addBytes {[&add](uqstl::span<const uint8_t> bytes) {
		for (const uint8_t byte : bytes)
		{
			add(byte, 1);
		}
	}};

	add(TFB_RandomState(), 4);

	const GAME_STATE& game {GlobData.Game_state};
	add(game.CurrentActivity, 2);
	add(game.GameClock.day_index, 1);
	add(game.GameClock.month_index, 1);
	add(game.GameClock.year_index, 2);
	add(static_cast<uint16_t>(game.GameClock.tick_count), 2);
	add(static_cast<uint16_t>(game.autopilot.x), 2);
	add(static_cast<uint16_t>(game.autopilot.y), 2);
	add(static_cast<uint16_t>(game.ip_location.x), 2);
	add(static_cast<uint16_t>(game.ip_location.y), 2);
	add(game.ShipFacing, 2);
	add(game.ip_planet, 1);
	add(game.in_orbit, 1);

	const SIS_STATE& sis {GlobData.SIS_state};
	add(static_cast<uint32_t>(sis.log_x), 4);
	add(static_cast<uint32_t>(sis.log_y), 4);
	add(sis.ResUnits, 4);
	add(sis.FuelOnBoard, 4);
	add(sis.CrewEnlisted, 2);
	add(sis.TotalElementMass, 2);
	add(sis.TotalBioMass, 2);
	addBytes(sis.ModuleSlots);
	addBytes(sis.DriveSlots);
	addBytes(sis.JetSlots);
	add(sis.NumLanders, 1);
	for (const uint16_t amount : sis.ElementAmounts)
	{
		add(amount, 2);
	}
	return hash;
}

static bool
isReplayingFrames(void)
{
	return inputScript && !inputScript->frames().empty();
}

// Replaces all input of this frame with the recorded input.
static void
applyFrameInput(void)
{
	const uqm::FrameInput* const input {inputScript->frameInput(inputFrame)};
	const uqm::FrameInput none {};
	const uqm::FrameInput& keys {input ? *input : none};
	const bool thisFrame {input && input->frame == inputFrame};

	for (int t {0}; t < static_cast<int>(ControlTemplate::NUM); t++)
	{
		for (int k {0}; k < NUM_KEYS; k++)
		{
			const uint64_t bit {uint64_t {1} << (t * NUM_KEYS + k)};
			CurrentInputState.key[t][k] = (keys.heldBattle & bit) != 0;
			CachedInputState.key[t][k] = CurrentInputState.key[t][k];
			PulsedInputState.key[t][k] = (keys.pulsedBattle & bit) != 0;
		}
	}
	for (int k {0}; k < NUM_MENU_KEYS; k++)
	{
		const uint64_t bit {uint64_t {1} << k};
		CurrentInputState.menu[k] = (keys.heldMenu & bit) != 0;
		CachedInputState.menu[k] = CurrentInputState.menu[k];
		PulsedInputState.menu[k] = (keys.pulsedMenu & bit) != 0;
	}
	// The main thread reads these keys from the immediate input state.
	ImmediateInputState.menu[KEY_SCREENSHOT] = CurrentInputState.menu[KEY_SCREENSHOT];
	if (CurrentInputState.menu[KEY_ABORT])
	{
		ImmediateInputState.menu[KEY_ABORT] = 1;
	}

	frameText = thisFrame ? input->text : uqstl::vector<uint32_t> {};
	frameTextNext = 0;
	frameLast = thisFrame ? input->last : 0;

	if (thisFrame && input->end)
	{
		uqm::log::info("Input script: end of the recording.");
		uqm::log::default_logger()->flush();
		ImmediateInputState.menu[KEY_ABORT] = 1;
	}
}

UniChar GetNextInputCharacter(void)
{
	UniChar ch {};
	if (isReplayingFrames())
	{
		ch = frameTextNext < frameText.size() ? static_cast<UniChar>(frameText[frameTextNext++]) : 0;
	}
	else
	{
		ch = GetNextCharacter();
	}
	if (ch != 0)
	{
		const uqstl::lock_guard lock {recordMutex};
		if (recordPending)
		{
			recordPending->text.push_back(ch);
		}
	}
	return ch;
}

UniChar GetLastInputCharacter(void)
{
	const UniChar ch {isReplayingFrames() ? static_cast<UniChar>(frameLast) : GetLastCharacter()};
	if (ch != 0)
	{
		const uqstl::lock_guard lock {recordMutex};
		if (recordPending)
		{
			recordPending->last = ch;
		}
	}
	return ch;
}

bool LoadInputScript(const char* path)
{
	uqstl::ifstream file {path, uqstl::ios::binary};
	if (!file)
	{
		uqm::log::error("Input script: cannot open '{}'.", path);
		return false;
	}
	uqstl::stringstream text {};
	text << file.rdbuf();

	uqstl::string error {};
	inputScript = uqm::InputScript::parse(text.str(), error);
	if (!inputScript)
	{
		uqm::log::error("Input script '{}': {}", path, error);
		return false;
	}
	uqm::log::info("Input script: running '{}' ({} commands).", path, inputScript->commands().size());
	inputScriptStart = uqstl::chrono::steady_clock::now();
	return true;
}

bool StartInputRecording(const char* path)
{
	recordFile.open(path, uqstl::ios::binary | uqstl::ios::trunc);
	if (!recordFile)
	{
		uqm::log::error("Input recording: cannot create '{}'.", path);
		return false;
	}
	recordSeed = inputScript && inputScript->seed() ? *inputScript->seed() : static_cast<uint32_t>(uqstl::time(nullptr));
	recordFile << "# Input recorded by UQM-MegaModern. Replay it with --input-script.\n";
	recordFile << "seed " << *recordSeed << "\n";
	recordFile.flush();
	uqm::log::info("Input recording: writing '{}' with seed {}.", path, *recordSeed);
	return true;
}

static void
writePendingFrame(void)
{
	if (!recordPending)
	{
		return;
	}
	if (!recordPending->sameKeys(recordWritten) || !recordPending->text.empty() || recordPending->last != 0 || recordPending->end)
	{
		recordFile << recordPending->format() << "\n";
		recordFile.flush();
		recordWritten = *recordPending;
	}
	recordPending.reset();
}

// Keys that a replay must not press.
static bool
isRecordedMenuKey(int key)
{
	switch (key)
	{
		case KEY_PAUSE:
		case KEY_FULLSCREEN:
		case KEY_DEBUG:
		case KEY_DEBUG_2:
		case KEY_DEBUG_3:
		case KEY_DEBUG_4:
			return false;
		default:
			return true;
	}
}

// Starts the record of this frame, from the final input state of the frame.
static void
recordFrameInput(void)
{
	const uqstl::lock_guard lock {recordMutex};
	if (!recordFile.is_open())
	{
		return;
	}
	writePendingFrame();
	if (inputFrame % c_recordCheckInterval == 0)
	{
		recordFile << fmt::format("check {} {:016x}\n", inputFrame, computeStateHash());
	}
	uqm::FrameInput input {.frame = inputFrame};
	for (int t {0}; t < static_cast<int>(ControlTemplate::NUM); t++)
	{
		for (int k {0}; k < NUM_KEYS; k++)
		{
			const uint64_t bit {uint64_t {1} << (t * NUM_KEYS + k)};
			input.heldBattle |= CurrentInputState.key[t][k] ? bit : 0;
			input.pulsedBattle |= PulsedInputState.key[t][k] ? bit : 0;
		}
	}
	for (int k {0}; k < NUM_MENU_KEYS; k++)
	{
		if (isRecordedMenuKey(k))
		{
			const uint64_t bit {uint64_t {1} << k};
			input.heldMenu |= CurrentInputState.menu[k] ? bit : 0;
			input.pulsedMenu |= PulsedInputState.menu[k] ? bit : 0;
		}
	}
	recordPending = uqstl::move(input);
}

void StopInputRecording(void)
{
	const uqstl::lock_guard lock {recordMutex};
	if (!recordFile.is_open())
	{
		return;
	}
	writePendingFrame();
	recordFile << uqm::FrameInput {.frame = inputFrame, .heldMenu = recordWritten.heldMenu, .pulsedMenu = recordWritten.pulsedMenu, .heldBattle = recordWritten.heldBattle, .pulsedBattle = recordWritten.pulsedBattle, .end = true}.format() << "\n";
	recordFile.close();
	uqm::log::info("Input recording: {} input frames recorded.", inputFrame);
}

uqstl::optional<uint32_t> GetDeterministicSeed(void)
{
	if (recordSeed)
	{
		return recordSeed;
	}
	return inputScript ? inputScript->seed() : uqstl::nullopt;
}

void SetInputScriptMenuDump(void (*dump)(void))
{
	inputScriptMenuDump = dump;
}

static int
scriptMenuKey(uqm::ScriptKey key)
{
	switch (key)
	{
		case uqm::ScriptKey::Up:
			return KEY_MENU_UP;
		case uqm::ScriptKey::Down:
			return KEY_MENU_DOWN;
		case uqm::ScriptKey::Left:
			return KEY_MENU_LEFT;
		case uqm::ScriptKey::Right:
			return KEY_MENU_RIGHT;
		case uqm::ScriptKey::Select:
			return KEY_MENU_SELECT;
		case uqm::ScriptKey::Cancel:
			return KEY_MENU_CANCEL;
		case uqm::ScriptKey::Special:
			return KEY_MENU_SPECIAL;
		case uqm::ScriptKey::Delete:
			return KEY_MENU_DELETE;
		case uqm::ScriptKey::Backspace:
			return KEY_MENU_BACKSPACE;
		case uqm::ScriptKey::PageUp:
			return KEY_MENU_PAGE_UP;
		case uqm::ScriptKey::PageDown:
			return KEY_MENU_PAGE_DOWN;
		case uqm::ScriptKey::Home:
			return KEY_MENU_HOME;
		case uqm::ScriptKey::End:
			return KEY_MENU_END;
		case uqm::ScriptKey::Screenshot:
			return KEY_SCREENSHOT;
	}
	return KEY_MENU_SELECT;
}

// Adds the scripted key, if any, to the input of this frame.
static void
applyInputScript(void)
{
	const uqstl::lock_guard lock {inputScriptMutex};
	uint32_t now {};
	if (GetDeterministicSeed())
	{
		// A deterministic run counts game time.
		const TimeCount gameNow {GetTimeCounter()};
		if (!inputScriptGameStart)
		{
			inputScriptGameStart = gameNow;
		}
		now = static_cast<uint32_t>(uint64_t {gameNow - *inputScriptGameStart} * 1000 / GameTicksPerSecond);
	}
	else
	{
		now = static_cast<uint32_t>(uqstl::chrono::duration_cast<uqstl::chrono::milliseconds>(uqstl::chrono::steady_clock::now() - inputScriptStart).count());
	}
	const uqm::InputScript::Step step {inputScript->update(now)};

	if (step.press == uqm::ScriptKey::Screenshot)
	{
		// The main thread takes screenshots from the immediate input state.
		ImmediateInputState.menu[KEY_SCREENSHOT] = 1;
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
	if (step.quit)
	{
		uqm::log::info("Input script: quit.");
		uqm::log::default_logger()->flush();
		// The main thread exits on the abort key.
		ImmediateInputState.menu[KEY_ABORT] = 1;
	}
	if (inputScript->finished() && !inputScriptFinishedLogged)
	{
		uqm::log::info("Input script: finished.");
		inputScriptFinishedLogged = true;
	}
}

static void
_clear_menu_state(void)
{
	int i, j;
	for (i = 0; i < static_cast<int>(ControlTemplate::NUM); i++)
	{
		for (j = 0; j < NUM_KEYS; j++)
		{
			PulsedInputState.key[i][j] = 0;
			CachedInputState.key[i][j] = 0;
		}
	}
	for (i = 0; i < NUM_MENU_KEYS; i++)
	{
		PulsedInputState.menu[i] = 0;
		CachedInputState.menu[i] = 0;
	}
	CachedGestalt = false;
}

void ResetKeyRepeat(void)
{
	uint32_t initTime = GetTimeCounter();
	int i, j;
	for (i = 0; i < static_cast<int>(ControlTemplate::NUM); i++)
	{
		for (j = 0; j < NUM_KEYS; j++)
		{
			RepeatDelays.key[i][j] = _max_accel;
			Times.key[i][j] = initTime;
		}
	}
	for (i = 0; i < NUM_MENU_KEYS; i++)
	{
		RepeatDelays.menu[i] = _max_accel;
		Times.menu[i] = initTime;
	}
	GestaltRepeatDelay = _max_accel;
	GestaltTime = initTime;
}

static void
_check_for_pulse(int* current, int* cached, int* old, uint32_t* accel,
				 uint32_t* newtime, uint32_t* oldtime)
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
static void
_check_gestalt(uint32_t NewTime)
{
	bool CurrentGestalt;
	int i, j;
	OldGestalt = CachedGestalt;

	CachedGestalt = 0;
	CurrentGestalt = 0;
	for (i = 0; i < static_cast<int>(ControlTemplate::NUM); i++)
	{
		for (j = 0; j < NUM_KEYS; j++)
		{
			CachedGestalt |= (bool)ImmediateInputState.key[i][j];
			CurrentGestalt |= (bool)PulsedInputState.key[i][j];
		}
	}
	for (i = 0; i < NUM_MENU_KEYS; i++)
	{
		CachedGestalt |= (bool)ImmediateInputState.menu[i];
		CurrentGestalt |= (bool)PulsedInputState.menu[i];
	}

	if (OldGestalt && CachedGestalt)
	{
		if (NewTime - GestaltTime < GestaltRepeatDelay)
		{
			for (i = 0; i < static_cast<int>(ControlTemplate::NUM); i++)
			{
				for (j = 0; j < NUM_KEYS; j++)
				{
					PulsedInputState.key[i][j] = 0;
				}
			}
			for (i = 0; i < NUM_MENU_KEYS; i++)
			{
				PulsedInputState.menu[i] = 0;
			}
		}
		else
		{
			for (i = 0; i < static_cast<int>(ControlTemplate::NUM); i++)
			{
				for (j = 0; j < NUM_KEYS; j++)
				{
					PulsedInputState.key[i][j] = CachedInputState.key[i][j];
				}
			}
			for (i = 0; i < NUM_MENU_KEYS; i++)
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
		for (i = 0; i < static_cast<int>(ControlTemplate::NUM); i++)
		{
			for (j = 0; j < NUM_KEYS; j++)
			{
				PulsedInputState.key[i][j] = CachedInputState.key[i][j];
			}
		}
		for (i = 0; i < NUM_MENU_KEYS; i++)
		{
			PulsedInputState.menu[i] = CachedInputState.menu[i];
		}
		GestaltTime = NewTime;
		GestaltRepeatDelay = _max_accel;
	}
}

void UpdateInputState(void)
{
	uint32_t NewTime;
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
		int i, j;
		for (i = 0; i < static_cast<int>(ControlTemplate::NUM); i++)
		{
			for (j = 0; j < NUM_KEYS; j++)
			{
				_check_for_pulse(&PulsedInputState.key[i][j],
								 &CachedInputState.key[i][j],
								 &OldInputState.key[i][j], &RepeatDelays.key[i][j],
								 &NewTime, &Times.key[i][j]);
			}
		}
		for (i = 0; i < NUM_MENU_KEYS; i++)
		{
			_check_for_pulse(&PulsedInputState.menu[i],
							 &CachedInputState.menu[i], &OldInputState.menu[i],
							 &RepeatDelays.menu[i], &NewTime, &Times.menu[i]);
		}
	}

	if (isReplayingFrames())
	{
		applyFrameInput();
	}
	if (inputScript && !stateMismatchReported)
	{
		if (const uqstl::optional<uint64_t> expected {inputScript->checkAt(inputFrame)}; expected && *expected != computeStateHash())
		{
			uqm::log::error("Input script: the game state differs from the recording at input frame {}.", inputFrame);
			stateMismatchReported = true;
		}
	}
	recordFrameInput();
	++inputFrame;

	if (CurrentInputState.menu[KEY_PAUSE])
	{
		GamePaused = true;
	}

	if (CurrentInputState.menu[KEY_EXIT])
	{
		ExitRequested = true;
	}

#if defined(DEBUG) || defined(USE_DEBUG_KEY)
	if (PulsedInputState.menu[KEY_DEBUG])
	{
		debugKeyPressedSynchronous();
	}
	if (PulsedInputState.menu[KEY_DEBUG_2])
	{
		debugKey2PressedSynchronous();
	}
	if (PulsedInputState.menu[KEY_DEBUG_3])
	{
		debugKey3PressedSynchronous();
	}
	if (PulsedInputState.menu[KEY_DEBUG_4])
	{
		debugKey4PressedSynchronous();
	}
#endif
}

InputFrameCallback*
SetInputCallback(InputFrameCallback* callback)
{
	InputFrameCallback* old = inputCallback;

	// Replacing an existing callback with another is not a problem,
	// but currently this should never happen, which is why the assert.
	assert(!old || !callback);
	inputCallback = callback;

	return old;
}

void SetMenuRepeatDelay(uint32_t min, uint32_t max, uint32_t step, bool gestalt)
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
	_min_accel = ACCELERATION_INCREMENT;
	_max_accel = MENU_REPEAT_DELAY;
	_step_accel = ACCELERATION_INCREMENT;
	_gestalt_keys = false;
	//_clear_menu_state ();
	ResetKeyRepeat();
}

void FlushInput(void)
{
	TFB_ResetControls();
	_clear_menu_state();
}

static MENU_SOUND_FLAGS
MenuKeysToSoundFlags(const CONTROLLER_INPUT_STATE* state)
{
	MENU_SOUND_FLAGS soundFlags;

	soundFlags = MENU_SOUND_NONE;
	if (state->menu[KEY_MENU_UP])
	{
		soundFlags |= MENU_SOUND_UP;
	}
	if (state->menu[KEY_MENU_DOWN])
	{
		soundFlags |= MENU_SOUND_DOWN;
	}
	if (state->menu[KEY_MENU_LEFT])
	{
		soundFlags |= MENU_SOUND_LEFT;
	}
	if (state->menu[KEY_MENU_RIGHT])
	{
		soundFlags |= MENU_SOUND_RIGHT;
	}
	if (state->menu[KEY_MENU_SELECT])
	{
		soundFlags |= MENU_SOUND_SELECT;
	}
	if (state->menu[KEY_MENU_CANCEL])
	{
		soundFlags |= MENU_SOUND_CANCEL;
	}
	if (state->menu[KEY_MENU_SPECIAL])
	{
		soundFlags |= MENU_SOUND_SPECIAL;
	}
	if (state->menu[KEY_MENU_PAGE_UP])
	{
		soundFlags |= MENU_SOUND_PAGEUP;
	}
	if (state->menu[KEY_MENU_PAGE_DOWN])
	{
		soundFlags |= MENU_SOUND_PAGEDOWN;
	}
	if (state->menu[KEY_MENU_DELETE])
	{
		soundFlags |= MENU_SOUND_DELETE;
	}
	if (state->menu[KEY_MENU_BACKSPACE])
	{
		soundFlags |= MENU_SOUND_DELETE;
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
		MENU_SOUND_FLAGS soundFlags;
		Async_process();
		TaskSwitch();

		UpdateInputState();

#if DEMO_MODE || CREATE_JOURNAL
		if (ArrowInput != DemoInput)
#endif
		{
#if CREATE_JOURNAL
			JournalInput(InputState);
#endif /* CREATE_JOURNAL */
		}

		soundFlags = MenuKeysToSoundFlags(&PulsedInputState);

		if (MenuSounds && (soundFlags & (sound_0 | sound_1)))
		{
			SOUND S;

			S = MenuSounds;
			if (soundFlags & sound_1)
			{
				S = SetAbsSoundIndex(S, MENU_SOUND_SUCCESS);
			}

			PlaySoundEffect(S, 0, NotPositional(), nullptr, 0);
		}

		if (inputCallback)
		{
			inputCallback();
		}

	} while (((INPUT_STATE_DESC*)pInputState)->InputFunc(pInputState));

	if (resetInput)
	{
		FlushInput();
	}
}

void SetMenuSounds(MENU_SOUND_FLAGS s0, MENU_SOUND_FLAGS s1)
{
	sound_0 = s0;
	sound_1 = s1;
}

void GetMenuSounds(MENU_SOUND_FLAGS* s0, MENU_SOUND_FLAGS* s1)
{
	*s0 = sound_0;
	*s1 = sound_1;
}

static BATTLE_INPUT_STATE
ControlInputToBattleInput(const int* keyState)
{
	BATTLE_INPUT_STATE InputState = 0;

	if (keyState[KEY_UP])
	{
		InputState |= BATTLE_THRUST;
	}
	if (keyState[KEY_LEFT])
	{
		InputState |= BATTLE_LEFT;
	}
	if (keyState[KEY_RIGHT])
	{
		InputState |= BATTLE_RIGHT;
	}
	if (keyState[KEY_WEAPON])
	{
		InputState |= BATTLE_WEAPON;
	}
	if (keyState[KEY_SPECIAL])
	{
		InputState |= BATTLE_SPECIAL;
	}
	if (keyState[KEY_ESCAPE])
	{
		InputState |= BATTLE_ESCAPE;
	}
	if (keyState[KEY_DOWN])
	{
		InputState |= BATTLE_DOWN;
	}
	if (keyState[KEY_THRUST])
	{
		InputState |= BATTLE_THRUST_ALT;
	}

	return InputState;
}

BATTLE_INPUT_STATE
CurrentInputToBattleInput(uint16_t player)
{
	return ControlInputToBattleInput(
		CurrentInputState.key[static_cast<int>(PlayerControlTemplates[player])]);
}

BATTLE_INPUT_STATE
PulsedInputToBattleInput(uint16_t player)
{
	return ControlInputToBattleInput(
		PulsedInputState.key[static_cast<int>(PlayerControlTemplates[player])]);
}

bool AnyButtonPress(bool CheckSpecial)
{
	int i, j;
	(void)CheckSpecial; // Ignored
	UpdateInputState();
	for (i = 0; i < static_cast<int>(ControlTemplate::NUM); i++)
	{
		for (j = 0; j < NUM_KEYS; j++)
		{
			if (CurrentInputState.key[i][j])
			{
				return true;
			}
		}
	}
	for (i = 0; i < NUM_MENU_KEYS; i++)
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
		CurrentInputState.menu[KEY_MENU_LEFT] || CurrentInputState.menu[KEY_MENU_RIGHT] || CurrentInputState.menu[KEY_MENU_UP] || CurrentInputState.menu[KEY_MENU_DOWN]);
}

bool ActKeysPress(void)
{
	UpdateInputState();

	return (
		CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][KEY_WEAPON]
		|| CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][KEY_SPECIAL]
		|| CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][KEY_ESCAPE]
		|| CurrentInputState.menu[KEY_MENU_SELECT]
		|| CurrentInputState.menu[KEY_MENU_CANCEL]
		|| CurrentInputState.menu[KEY_MENU_SPECIAL]);
}

bool ConfirmExit(void)
{
	uint32_t old_max_accel, old_min_accel, old_step_accel;
	bool old_gestalt_keys, result;

	old_max_accel = _max_accel;
	old_min_accel = _min_accel;
	old_step_accel = _step_accel;
	old_gestalt_keys = _gestalt_keys;

	SetDefaultMenuRepeatDelay();

	result = DoConfirmExit();

	SetMenuRepeatDelay(old_min_accel, old_max_accel, old_step_accel,
					   old_gestalt_keys);
	return result;
}

void TestSpeechSound(STRING snd)
{
	PlaySpeechEffect((SOUND)snd, NotPositional(), nullptr, 0);
}