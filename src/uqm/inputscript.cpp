// Scripted menu input, read from a text file, for testing the game without a keyboard.
#include "inputscript.h"

#include "core/string/StringUtils.h"

#include <charconv>
#include <fmt/format.h>

namespace uqm
{

namespace
{

constexpr struct
{
	const char* name;
	ScriptKey key;
} c_keyNames[] {
	{"up",		   ScriptKey::Up		},
	{"down",		 ScriptKey::Down		},
	{"left",		 ScriptKey::Left		},
	{"right",	  ScriptKey::Right	  },
	{"select",	   ScriptKey::Select	},
	{"cancel",	   ScriptKey::Cancel	},
	{"special",	ScriptKey::Special	  },
	{"delete",	   ScriptKey::Delete	},
	{"backspace",  ScriptKey::Backspace },
	{"pageup",	   ScriptKey::PageUp	},
	{"pagedown",	 ScriptKey::PageDown	},
	{"home",		 ScriptKey::Home		},
	{"end",		ScriptKey::End		  },
	{"screenshot", ScriptKey::Screenshot},
};

uqstl::optional<ScriptKey> keyFromName(uqstl::string_view name)
{
	for (const auto& entry : c_keyNames)
	{
		if (compareICase(name, entry.name))
		{
			return entry.key;
		}
	}
	return uqstl::nullopt;
}

uqstl::optional<uint32_t> parseCount(uqstl::string_view text)
{
	uint32_t value {};
	if (parseStr(text, value) != uqstl::errc {})
	{
		return uqstl::nullopt;
	}
	return value;
}

constexpr const char* c_menuKeyNames[] {
	"pause", "exit", "abort", "debug", "fullscreen", "up", "down", "left", "right", "select",
	"cancel", "special", "pageup", "pagedown", "home", "end", "zoomin", "zoomout", "delete", "backspace",
	"editcancel", "search", "next", "togglemap", "screenshot", "debug2", "debug3", "debug4", "any"};

constexpr const char* c_battleKeyNames[] {"up", "down", "left", "right", "weapon", "special", "escape", "thrust"};

constexpr int c_battleKeyCount {c_scriptBattleKeyCount};
static_assert(uqstl::size(c_menuKeyNames) == c_scriptMenuKeyCount && c_scriptMenuKeyCount <= 64);
static_assert(uqstl::size(c_battleKeyNames) == c_scriptBattleKeyCount && c_scriptTemplateCount * c_scriptBattleKeyCount <= 64);

uqstl::optional<int> nameIndex(uqstl::span<const char* const> names, uqstl::string_view name)
{
	for (size_t i {0}; i < names.size(); ++i)
	{
		if (compareICase(name, names[i]))
		{
			return static_cast<int>(i);
		}
	}
	return uqstl::nullopt;
}

// Adds the keys of a comma list to the masks. Returns false if a name is not valid.
bool parseKeys(uqstl::string_view list, uint64_t& menu, uint64_t& battle)
{
	while (!list.empty())
	{
		const size_t comma {list.find(',')};
		const uqstl::string_view name {list.substr(0, comma)};
		list = comma == uqstl::string_view::npos ? uqstl::string_view {} : list.substr(comma + 1);

		if (name.size() > 3 && (name[0] == 't' || name[0] == 'T') && name[2] == '.' && name[1] >= '1' && name[1] < '1' + c_scriptTemplateCount)
		{
			const auto key {nameIndex(c_battleKeyNames, name.substr(3))};
			if (!key)
			{
				return false;
			}
			battle |= uint64_t {1} << ((name[1] - '1') * c_battleKeyCount + *key);
		}
		else if (const auto key {nameIndex(c_menuKeyNames, name)})
		{
			menu |= uint64_t {1} << *key;
		}
		else
		{
			return false;
		}
	}
	return true;
}

uqstl::string formatKeys(uint64_t menu, uint64_t battle)
{
	uqstl::string text {};
	const auto add {[&text](uqstl::string_view name) {
		if (!text.empty())
		{
			text += ',';
		}
		text += name;
	}};
	for (size_t i {0}; i < uqstl::size(c_menuKeyNames); ++i)
	{
		if ((menu >> i) & 1)
		{
			add(c_menuKeyNames[i]);
		}
	}
	for (int t {0}; t < c_scriptTemplateCount; ++t)
	{
		for (int k {0}; k < c_battleKeyCount; ++k)
		{
			if ((battle >> (t * c_battleKeyCount + k)) & 1)
			{
				add(fmt::format("t{}.{}", t + 1, c_battleKeyNames[k]));
			}
		}
	}
	return text;
}

// Parses the part of a frame line after the frame number. Returns an error text, or an empty string.
uqstl::string parseFrameInput(uqstl::string_view text, FrameInput& input)
{
	const auto nextWord {[&text]() {
		text = trim(text);
		const size_t space {text.find_first_of(" \t")};
		const uqstl::string_view word {text.substr(0, space)};
		text = space == uqstl::string_view::npos ? uqstl::string_view {} : text.substr(space);
		return word;
	}};

	for (uqstl::string_view word {nextWord()}; !word.empty(); word = nextWord())
	{
		if (compareICase(word, "held") || compareICase(word, "pulsed"))
		{
			const bool held {compareICase(word, "held")};
			if (!parseKeys(nextWord(), held ? input.heldMenu : input.pulsedMenu, held ? input.heldBattle : input.pulsedBattle))
			{
				return "unknown key name";
			}
		}
		else if (compareICase(word, "text"))
		{
			for (uqstl::string_view list {nextWord()}; !list.empty();)
			{
				const size_t comma {list.find(',')};
				const auto code {parseCount(list.substr(0, comma))};
				if (!code)
				{
					return "expected character codes";
				}
				input.text.push_back(*code);
				list = comma == uqstl::string_view::npos ? uqstl::string_view {} : list.substr(comma + 1);
			}
		}
		else if (compareICase(word, "last"))
		{
			const auto code {parseCount(nextWord())};
			if (!code)
			{
				return "expected a character code";
			}
			input.last = *code;
		}
		else if (compareICase(word, "end"))
		{
			input.end = true;
		}
		else
		{
			return fmt::format("unknown frame field '{}'", word);
		}
	}
	return {};
}

} // namespace

uqstl::span<const char* const> menuKeyNames()
{
	return c_menuKeyNames;
}

uqstl::span<const char* const> battleKeyNames()
{
	return c_battleKeyNames;
}

bool FrameInput::sameKeys(const FrameInput& other) const noexcept
{
	return heldMenu == other.heldMenu && pulsedMenu == other.pulsedMenu && heldBattle == other.heldBattle && pulsedBattle == other.pulsedBattle;
}

uqstl::string FrameInput::format() const
{
	uqstl::string line {fmt::format("frame {}", frame)};
	if (heldMenu != 0 || heldBattle != 0)
	{
		line += " held " + formatKeys(heldMenu, heldBattle);
	}
	if (pulsedMenu != 0 || pulsedBattle != 0)
	{
		line += " pulsed " + formatKeys(pulsedMenu, pulsedBattle);
	}
	if (!text.empty())
	{
		line += " text ";
		for (size_t i {0}; i < text.size(); ++i)
		{
			line += fmt::format("{}{}", i == 0 ? "" : ",", text[i]);
		}
	}
	if (last != 0)
	{
		line += fmt::format(" last {}", last);
	}
	if (end)
	{
		line += " end";
	}
	return line;
}

const FrameInput* InputScript::frameInput(uint64_t frame)
{
	while (m_nextFrame < m_frames.size() && m_frames[m_nextFrame].frame <= frame)
	{
		++m_nextFrame;
	}
	return m_nextFrame == 0 ? nullptr : &m_frames[m_nextFrame - 1];
}

uqstl::optional<InputScript> InputScript::parse(uqstl::string_view text, uqstl::string& error)
{
	InputScript script {};
	size_t lineNumber {};
	while (!text.empty())
	{
		++lineNumber;
		const size_t end {text.find('\n')};
		uqstl::string_view line {trim(text.substr(0, end))};
		text = end == uqstl::string_view::npos ? uqstl::string_view {} : text.substr(end + 1);

		if (line.empty() || line.front() == '#')
		{
			continue;
		}

		const size_t space {line.find_first_of(" \t")};
		const uqstl::string_view word {line.substr(0, space)};
		const uqstl::string_view argument {space == uqstl::string_view::npos ? uqstl::string_view {} : trim(line.substr(space))};

		const auto fail {[&](uqstl::string_view reason) {
			error = fmt::format("line {}: {}", lineNumber, reason);
			return uqstl::nullopt;
		}};

		if (compareICase(word, "wait") || compareICase(word, "delay"))
		{
			const auto ms {parseCount(argument)};
			if (!ms)
			{
				return fail("expected a time in milliseconds");
			}
			const auto kind {compareICase(word, "wait") ? ScriptCommand::Kind::Wait : ScriptCommand::Kind::Delay};
			script.m_commands.push_back({.kind = kind, .milliseconds = *ms});
		}
		else if (compareICase(word, "frame"))
		{
			const size_t numberEnd {argument.find_first_of(" \t")};
			uint64_t frame {};
			if (parseStr(argument.substr(0, numberEnd), frame) != uqstl::errc {})
			{
				return fail("expected a frame number");
			}
			if (!script.m_frames.empty() && frame <= script.m_frames.back().frame)
			{
				return fail("frame numbers must increase");
			}
			FrameInput input {.frame = frame};
			const uqstl::string frameError {parseFrameInput(numberEnd == uqstl::string_view::npos ? uqstl::string_view {} : argument.substr(numberEnd), input)};
			if (!frameError.empty())
			{
				return fail(frameError);
			}
			script.m_frames.push_back(uqstl::move(input));
		}
		else if (compareICase(word, "check"))
		{
			const size_t space {argument.find_first_of(" \t")};
			uint64_t frame {};
			uint64_t hash {};
			const uqstl::string_view hashText {space == uqstl::string_view::npos ? uqstl::string_view {} : trim(argument.substr(space))};
			if (parseStr(argument.substr(0, space), frame) != uqstl::errc {} || hashText.empty()
				|| uqstl::from_chars(hashText.data(), hashText.data() + hashText.size(), hash, 16).ptr != hashText.data() + hashText.size())
			{
				return fail("expected a frame number and a hexadecimal hash");
			}
			if (!script.m_checks.empty() && frame <= script.m_checks.back().first)
			{
				return fail("check frame numbers must increase");
			}
			script.m_checks.emplace_back(frame, hash);
		}
		else if (compareICase(word, "seed"))
		{
			const auto seed {parseCount(argument)};
			if (!seed)
			{
				return fail("expected a seed number");
			}
			script.m_seed = *seed;
		}
		else if (compareICase(word, "dumpmenu"))
		{
			script.m_commands.push_back({.kind = ScriptCommand::Kind::DumpMenu});
		}
		else if (compareICase(word, "log"))
		{
			script.m_commands.push_back({.kind = ScriptCommand::Kind::Log, .text = uqstl::string {argument}});
		}
		else if (compareICase(word, "quit"))
		{
			script.m_commands.push_back({.kind = ScriptCommand::Kind::Quit});
		}
		else if (const auto key {keyFromName(word)})
		{
			const auto count {argument.empty() ? uqstl::optional<uint32_t> {1} : parseCount(argument)};
			if (!count || *count == 0)
			{
				return fail("expected a key count of 1 or more");
			}
			for (uint32_t i {0}; i < *count; ++i)
			{
				script.m_commands.push_back({.kind = ScriptCommand::Kind::Press, .key = *key});
			}
		}
		else
		{
			return fail(fmt::format("unknown command '{}'", word));
		}
	}
	return script;
}

InputScript::Step InputScript::update(uint32_t now)
{
	if (m_releasePending)
	{
		m_releasePending = false;
		m_waitUntil = now + m_delay;
		return {};
	}
	if (m_waitUntil && now < *m_waitUntil)
	{
		return {};
	}
	m_waitUntil.reset();

	while (m_next < m_commands.size())
	{
		const ScriptCommand& command {m_commands[m_next++]};
		switch (command.kind)
		{
			case ScriptCommand::Kind::Press:
				m_releasePending = true;
				return {.press = command.key};
			case ScriptCommand::Kind::Wait:
				m_waitUntil = now + command.milliseconds;
				return {};
			case ScriptCommand::Kind::Delay:
				m_delay = command.milliseconds;
				continue;
			case ScriptCommand::Kind::DumpMenu:
				m_waitUntil = now + m_delay;
				return {.dumpMenu = true};
			case ScriptCommand::Kind::Log:
				return {.log = command.text};
			case ScriptCommand::Kind::Quit:
				return {.quit = true};
		}
	}
	return {};
}

uqstl::optional<uint64_t> InputScript::checkAt(uint64_t frame)
{
	while (m_nextCheck < m_checks.size() && m_checks[m_nextCheck].first < frame)
	{
		++m_nextCheck;
	}
	if (m_nextCheck < m_checks.size() && m_checks[m_nextCheck].first == frame)
	{
		return m_checks[m_nextCheck++].second;
	}
	return uqstl::nullopt;
}

bool InputScript::finished() const noexcept
{
	return m_next >= m_commands.size() && !m_releasePending && m_nextFrame >= m_frames.size();
}

} // namespace uqm
