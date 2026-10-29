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

	[[nodiscard]] uqstl::span<const ScriptCommand> commands() const noexcept
	{
		return m_commands;
	}

private:
	uqstl::vector<ScriptCommand> m_commands {};
	size_t m_next {};
	uint32_t m_delay {c_defaultDelay};
	uqstl::optional<uint32_t> m_waitUntil {};
	bool m_releasePending {};
};

} // namespace uqm

#endif // UQM_INPUTSCRIPT_H_
