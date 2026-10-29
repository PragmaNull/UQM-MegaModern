// Scripted menu input, read from a text file, for testing the game without a keyboard.
#include "inputscript.h"

#include "core/string/StringUtils.h"

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

} // namespace

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

bool InputScript::finished() const noexcept
{
	return m_next >= m_commands.size() && !m_releasePending;
}

} // namespace uqm
