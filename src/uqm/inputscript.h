// Scripted menu input, read from a text file, for testing the game without a keyboard.
#pragma once
#ifndef UQM_INPUTSCRIPT_H_
#define UQM_INPUTSCRIPT_H_

#include "core/stl/stl.h"
#include "core/stl/string.h"
#include "core/stl/vector.h"

#include <cstdint>
#include <optional>

namespace uqm
{

enum class ScriptKey
{
	Up,
	Down,
	Left,
	Right,
	Select,
	Cancel,
	Special,
	Delete,
	Backspace,
	PageUp,
	PageDown,
	Home,
	End,
	Screenshot,
};

// One line of the script file. Lines are:
//   <key> [count]   press a key, e.g. "down 3"
//   wait <ms>       wait before the next command
//   delay <ms>      set the wait after each command (default 150)
//   dumpmenu        log the current setup menu
//   log <text>      write text to the log
//   quit            stop the game
//   seed <n>        run deterministically: a virtual game clock and random numbers from seed n
//   frame <n> ...   the input from input frame n on, see FrameInput
//   check <n> <hex> the expected hash of the game state at input frame n
// Empty lines and lines that start with '#' are ignored.
struct ScriptCommand
{
	enum class Kind
	{
		Press,
		Wait,
		Delay,
		DumpMenu,
		Log,
		Quit,
	};

	Kind kind {Kind::Wait};
	ScriptKey key {ScriptKey::Select};
	uint32_t milliseconds {};
	uqstl::string text {};
};

// The names of the menu keys, in the order of the KEY_PAUSE enum, and of the battle keys, in the order of the KEY_UP enum.
uqstl::span<const char* const> menuKeyNames();
uqstl::span<const char* const> battleKeyNames();
inline constexpr int c_scriptMenuKeyCount {29};
inline constexpr int c_scriptBattleKeyCount {8};
inline constexpr int c_scriptTemplateCount {6};

// Recorded input from one input frame on: frame <n> [held <keys>] [pulsed <keys>] [text <codes>] [last <code>] [end].
// Keys are comma lists of menu key names and t<template>.<battle key>, e.g. "down,t1.thrust". Codes are character codes.
// Held and pulsed keys stay until the next line. The characters are for this frame only. 'end' stops the game.
struct FrameInput
{
	uint64_t frame {};
	// Bit i is menu key i.
	uint64_t heldMenu {};
	uint64_t pulsedMenu {};
	// Bit (template * battle key count + key) is a battle key.
	uint64_t heldBattle {};
	uint64_t pulsedBattle {};
	uqstl::vector<uint32_t> text {};
	uint32_t last {};
	bool end {};

	// Returns true if the held and pulsed keys are the same.
	[[nodiscard]] bool sameKeys(const FrameInput& other) const noexcept;
	// Returns the script line for this input.
	[[nodiscard]] uqstl::string format() const;
};

class InputScript
{
public:
	struct Step
	{
		uqstl::optional<ScriptKey> press {};
		bool dumpMenu {};
		uqstl::optional<uqstl::string> log {};
		bool quit {};
	};

	static constexpr uint32_t c_defaultDelay {150};

	// Returns nullopt and sets 'error' if a line is not valid.
	[[nodiscard]] static uqstl::optional<InputScript> parse(uqstl::string_view text, uqstl::string& error);

	// Call once for each input frame, with a time in milliseconds.
	// A key press lasts one frame. The next frame releases it.
	[[nodiscard]] Step update(uint32_t now);

	[[nodiscard]] bool finished() const noexcept;

	// The seed of a deterministic run, or nullopt.
	[[nodiscard]] uqstl::optional<uint32_t> seed() const noexcept { return m_seed; }

	[[nodiscard]] uqstl::span<const ScriptCommand> commands() const noexcept
	{
		return m_commands;
	}

	// The recorded frames, in frame order.
	[[nodiscard]] uqstl::span<const FrameInput> frames() const noexcept { return m_frames; }

	// Returns the recorded input for input frame 'frame', or nullptr before the first recorded frame.
	// Call with frames in increasing order.
	[[nodiscard]] const FrameInput* frameInput(uint64_t frame);

	// Returns the expected state hash at input frame 'frame', or nullopt. Call with frames in increasing order.
	[[nodiscard]] uqstl::optional<uint64_t> checkAt(uint64_t frame);

private:
	uqstl::vector<ScriptCommand> m_commands {};
	uqstl::vector<FrameInput> m_frames {};
	size_t m_nextFrame {};
	uqstl::vector<uqstl::pair<uint64_t, uint64_t>> m_checks {};
	size_t m_nextCheck {};
	uqstl::optional<uint32_t> m_seed {};
	size_t m_next {};
	uint32_t m_delay {c_defaultDelay};
	uqstl::optional<uint32_t> m_waitUntil {};
	bool m_releasePending {};
};

} // namespace uqm

#endif // UQM_INPUTSCRIPT_H_
