// Tests for scripted menu input.
#include <gtest/gtest.h>

#include "uqm/inputscript.h"

namespace uqm
{

namespace
{

InputScript parseOrFail(uqstl::string_view text)
{
	uqstl::string error {};
	auto script {InputScript::parse(text, error)};
	EXPECT_TRUE(script.has_value()) << error;
	return script.value_or(InputScript {});
}

} // namespace

TEST(InputScriptTest, ParsesCommands)
{
	const InputScript script {parseOrFail("# comment\n\ndown 2\r\nSelect\nwait 500\ndelay 50\ndumpmenu\nlog hello world\nscreenshot\nquit\n")};
	const auto commands {script.commands()};
	ASSERT_EQ(commands.size(), 9u);
	EXPECT_EQ(commands[0].kind, ScriptCommand::Kind::Press);
	EXPECT_EQ(commands[0].key, ScriptKey::Down);
	EXPECT_EQ(commands[1].key, ScriptKey::Down);
	EXPECT_EQ(commands[2].key, ScriptKey::Select);
	EXPECT_EQ(commands[3].kind, ScriptCommand::Kind::Wait);
	EXPECT_EQ(commands[3].milliseconds, 500u);
	EXPECT_EQ(commands[4].kind, ScriptCommand::Kind::Delay);
	EXPECT_EQ(commands[5].kind, ScriptCommand::Kind::DumpMenu);
	EXPECT_EQ(commands[6].kind, ScriptCommand::Kind::Log);
	EXPECT_EQ(commands[6].text, "hello world");
	EXPECT_EQ(commands[7].key, ScriptKey::Screenshot);
	EXPECT_EQ(commands[8].kind, ScriptCommand::Kind::Quit);
}

TEST(InputScriptTest, SeedMakesTheRunDeterministic)
{
	EXPECT_FALSE(parseOrFail("down\n").seed().has_value());
	EXPECT_EQ(parseOrFail("seed 42\ndown\n").seed(), 42u);
	uqstl::string error {};
	EXPECT_FALSE(InputScript::parse("seed soon", error));
}

TEST(InputScriptTest, ReportsInvalidLines)
{
	uqstl::string error {};
	EXPECT_FALSE(InputScript::parse("down\njump\n", error));
	EXPECT_EQ(error, "line 2: unknown command 'jump'");
	EXPECT_FALSE(InputScript::parse("wait soon", error));
	EXPECT_NE(error.find("line 1"), uqstl::string::npos);
	EXPECT_FALSE(InputScript::parse("down 0", error));
}

TEST(InputScriptTest, PressLastsOneFrameThenWaitsForTheDelay)
{
	InputScript script {parseOrFail("down\nup\n")};

	auto step {script.update(1000)};
	ASSERT_TRUE(step.press.has_value());
	EXPECT_EQ(*step.press, ScriptKey::Down);

	EXPECT_FALSE(script.update(1001).press);
	EXPECT_FALSE(script.update(1001 + InputScript::c_defaultDelay - 1).press);

	step = script.update(1001 + InputScript::c_defaultDelay);
	ASSERT_TRUE(step.press.has_value());
	EXPECT_EQ(*step.press, ScriptKey::Up);
	EXPECT_FALSE(script.finished());

	EXPECT_FALSE(script.update(2000).press);
	EXPECT_TRUE(script.finished());
}

TEST(InputScriptTest, WaitDelaysTheNextCommand)
{
	InputScript script {parseOrFail("wait 300\nselect\n")};
	EXPECT_FALSE(script.update(0).press);
	EXPECT_FALSE(script.update(299).press);
	EXPECT_TRUE(script.update(300).press.has_value());
}

TEST(InputScriptTest, DelayChangesTheWaitAfterEachPress)
{
	InputScript script {parseOrFail("delay 10\ndown\ndown\n")};
	EXPECT_TRUE(script.update(0).press.has_value());
	EXPECT_FALSE(script.update(1).press);
	EXPECT_FALSE(script.update(10).press);
	EXPECT_TRUE(script.update(11).press.has_value());
}

TEST(InputScriptTest, ReturnsDumpLogAndQuitSteps)
{
	InputScript script {parseOrFail("log start\ndumpmenu\nquit\n")};
	EXPECT_EQ(script.update(0).log, uqstl::optional<uqstl::string> {"start"});
	EXPECT_TRUE(script.update(0).dumpMenu);
	EXPECT_FALSE(script.update(1).quit);
	EXPECT_TRUE(script.update(InputScript::c_defaultDelay + 1).quit);
	EXPECT_TRUE(script.finished());
}

} // namespace uqm
