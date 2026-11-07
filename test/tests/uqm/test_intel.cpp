#include <gtest/gtest.h>
#include "uqm/intel.h"
#include "uqm/element.h"
#include "options/OptionDefs.h"

// hack.. but I don't want to have to include uqm/setup.cpp as well

//uqm::BYTE m_playerControl[NumPlayers] {};

namespace uqm
{

//GodModeFlags m_godModes {};

// Test fixture for antiCheat and antiCheatAlt functions
class AntiCheatTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		// Initialize PlayerControl array
		m_playerControl[0] = PlayerControlFlags::None;
		m_playerControl[1] = PlayerControlFlags::None;

		// Reset god mode flags
		m_godModes = GodModeFlags::None;

		// Initialize test Element
		m_testElement = {};
		m_testElement.playerNr = 0;
	}

	void TearDown() override
	{
		// Reset global state
		m_playerControl[0] = PlayerControlFlags::None;
		m_playerControl[1] = PlayerControlFlags::None;
		m_godModes = GodModeFlags::None;
	}

	Element m_testElement {};
	uqstl::array<PlayerControlFlags, NumPlayers> m_playerControl {};
	GodModeFlags m_godModes {};
};

// ============================================================================
// antiCheatImpl() tests
// ============================================================================

TEST_F(AntiCheatTest, AntiCheat_BothPlayersAI_ReturnsFalse)
{
	// Arrange: Both players are AI-controlled
	m_playerControl[0] = ComputerControlFlags;
	m_playerControl[1] = ComputerControlFlags;
	m_godModes = GodModeFlags::InfiniteBattery;
	m_testElement.playerNr = 0;

	// Act & Assert: Should return false regardless of other conditions
	EXPECT_FALSE(antiCheatImpl(&m_testElement, false, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
	EXPECT_FALSE(antiCheatImpl(&m_testElement, true, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheat_GodModeDisabled_ReturnsFalse)
{
	// Arrange: God mode is disabled
	m_playerControl[0] = PlayerControlFlags::Human;
	m_playerControl[1] = ComputerControlFlags;
	m_godModes = GodModeFlags::None;
	m_testElement.playerNr = 0;

	// Act & Assert: Should return false when god mode is not enabled
	EXPECT_FALSE(antiCheatImpl(&m_testElement, false, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
	EXPECT_FALSE(antiCheatImpl(&m_testElement, true, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheat_WrongGodModeFlag_ReturnsFalse)
{
	// Arrange: Different god mode flag is enabled
	m_playerControl[0] = PlayerControlFlags::Human;
	m_playerControl[1] = ComputerControlFlags;
	m_godModes = GodModeFlags::InfiniteBattery;
	m_testElement.playerNr = 0;

	// Act & Assert: Should return false when requesting different god mode
	EXPECT_FALSE(antiCheatImpl(&m_testElement, false, GodModeFlags::NoDamage, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheat_Player0AI_ElementPlayer1_SwapFalse_ReturnsTrue)
{
	// Arrange: Player 0 is AI, Element belongs to player 1, no swap
	m_playerControl[0] = ComputerControlFlags;
	m_playerControl[1] = PlayerControlFlags::Human;
	m_godModes = GodModeFlags::InfiniteBattery;
	m_testElement.playerNr = 1;

	// Act & Assert: Should allow cheat (AI's opponent gets benefit)
	EXPECT_TRUE(antiCheatImpl(&m_testElement, false, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheat_Player0AI_ElementPlayer0_SwapFalse_ReturnsFalse)
{
	// Arrange: Player 0 is AI, Element belongs to player 0, no swap
	m_playerControl[0] = ComputerControlFlags;
	m_playerControl[1] = PlayerControlFlags::Human;
	m_godModes = GodModeFlags::InfiniteBattery;
	m_testElement.playerNr = 0;

	// Act & Assert: Should not allow cheat (AI's own Element)
	EXPECT_FALSE(antiCheatImpl(&m_testElement, false, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheat_Player1AI_ElementPlayer0_SwapFalse_ReturnsTrue)
{
	// Arrange: Player 1 is AI, Element belongs to player 0, no swap
	m_playerControl[0] = PlayerControlFlags::Human;
	m_playerControl[1] = ComputerControlFlags;
	m_godModes = GodModeFlags::InfiniteBattery;
	m_testElement.playerNr = 0;

	// Act & Assert: Should allow cheat (AI's opponent gets benefit)
	EXPECT_TRUE(antiCheatImpl(&m_testElement, false, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheat_Player1AI_ElementPlayer1_SwapFalse_ReturnsFalse)
{
	// Arrange: Player 1 is AI, Element belongs to player 1, no swap
	m_playerControl[0] = PlayerControlFlags::Human;
	m_playerControl[1] = ComputerControlFlags;
	m_godModes = GodModeFlags::InfiniteBattery;
	m_testElement.playerNr = 1;

	// Act & Assert: Should not allow cheat (AI's own Element)
	EXPECT_FALSE(antiCheatImpl(&m_testElement, false, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheat_Player0AI_ElementPlayer0_SwapTrue_ReturnsTrue)
{
	// Arrange: Player 0 is AI, Element belongs to player 0, swap enabled
	m_playerControl[0] = ComputerControlFlags;
	m_playerControl[1] = PlayerControlFlags::Human;
	m_godModes = GodModeFlags::InfiniteBattery;
	m_testElement.playerNr = 0;

	// Act & Assert: With swap, player 0 maps to opponent
	EXPECT_TRUE(antiCheatImpl(&m_testElement, true, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheat_Player0AI_ElementPlayer1_SwapTrue_ReturnsFalse)
{
	// Arrange: Player 0 is AI, Element belongs to player 1, swap enabled
	m_playerControl[0] = ComputerControlFlags;
	m_playerControl[1] = PlayerControlFlags::Human;
	m_godModes = GodModeFlags::InfiniteBattery;
	m_testElement.playerNr = 1;

	// Act & Assert: With swap, player 1 maps to AI's own Element
	EXPECT_FALSE(antiCheatImpl(&m_testElement, true, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheat_Player1AI_ElementPlayer0_SwapTrue_ReturnsFalse)
{
	// Arrange: Player 1 is AI, Element belongs to player 0, swap enabled
	m_playerControl[0] = PlayerControlFlags::Human;
	m_playerControl[1] = ComputerControlFlags;
	m_godModes = GodModeFlags::InfiniteBattery;
	m_testElement.playerNr = 0;

	// Act & Assert: With swap, player 0 maps to AI's own Element
	EXPECT_FALSE(antiCheatImpl(&m_testElement, true, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheat_Player1AI_ElementPlayer1_SwapTrue_ReturnsTrue)
{
	// Arrange: Player 1 is AI, Element belongs to player 1, swap enabled
	m_playerControl[0] = PlayerControlFlags::Human;
	m_playerControl[1] = ComputerControlFlags;
	m_godModes = GodModeFlags::InfiniteBattery;
	m_testElement.playerNr = 1;

	// Act & Assert: With swap, player 1 maps to opponent
	EXPECT_TRUE(antiCheatImpl(&m_testElement, true, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheat_BothPlayersHuman_ReturnsFalse)
{
	// Arrange: Both players are human-controlled
	m_playerControl[0] = PlayerControlFlags::Human;
	m_playerControl[1] = PlayerControlFlags::Human;
	m_godModes = GodModeFlags::InfiniteBattery;
	m_testElement.playerNr = 0;

	// Act & Assert: No AI player, so no cheat should apply
	EXPECT_FALSE(antiCheatImpl(&m_testElement, false, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
	EXPECT_FALSE(antiCheatImpl(&m_testElement, true, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheat_AllGodModesEnabled_ReturnsTrue)
{
	// Arrange: All god modes enabled
	m_playerControl[0] = ComputerControlFlags;
	m_playerControl[1] = PlayerControlFlags::Human;
	m_godModes = GodModeFlags::All;
	m_testElement.playerNr = 1;

	// Act & Assert: Should work with any god mode flag
	EXPECT_TRUE(antiCheatImpl(&m_testElement, false, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
	EXPECT_TRUE(antiCheatImpl(&m_testElement, false, GodModeFlags::NoDamage, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheat_CyborgControl_TreatedAsComputer)
{
	// Arrange: Cyborg control should be treated as computer control
	m_playerControl[0] = PlayerControlFlags::Cyborg;
	m_playerControl[1] = PlayerControlFlags::Human;
	m_godModes = GodModeFlags::InfiniteBattery;
	m_testElement.playerNr = 1;

	// Act & Assert: PlayerControlFlags::Cyborg is part of ComputerControlFlags
	EXPECT_TRUE(antiCheatImpl(&m_testElement, false, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheat_PsytronControl_TreatedAsComputer)
{
	// Arrange: Psytron control should be treated as computer control
	m_playerControl[0] = PlayerControlFlags::Psytron;
	m_playerControl[1] = PlayerControlFlags::Human;
	m_godModes = GodModeFlags::InfiniteBattery;
	m_testElement.playerNr = 1;

	// Act & Assert: PlayerControlFlags::Psytron is part of ComputerControlFlags
	EXPECT_TRUE(antiCheatImpl(&m_testElement, false, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheat_NetworkControl_ReturnsFalse)
{
	// Arrange: Network control is not considered AI
	m_playerControl[0] = PlayerControlFlags::Network;
	m_playerControl[1] = PlayerControlFlags::Human;
	m_godModes = GodModeFlags::InfiniteBattery;
	m_testElement.playerNr = 1;

	// Act & Assert: Network control is not ComputerControlFlags
	EXPECT_FALSE(antiCheatImpl(&m_testElement, false, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

// ============================================================================
// antiCheatAltImpl() tests
// ============================================================================

TEST_F(AntiCheatTest, AntiCheatAlt_BothPlayersAI_ReturnsFalse)
{
	// Arrange: Both players are AI-controlled
	m_playerControl[0] = ComputerControlFlags;
	m_playerControl[1] = ComputerControlFlags;
	m_godModes = GodModeFlags::InfiniteBattery;

	// Act & Assert: Should return false for AI vs AI
	EXPECT_FALSE(antiCheatAltImpl(GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheatAlt_GodModeDisabled_ReturnsFalse)
{
	// Arrange: God mode is disabled
	m_playerControl[0] = PlayerControlFlags::Human;
	m_playerControl[1] = ComputerControlFlags;
	m_godModes = GodModeFlags::None;

	// Act & Assert: Should return false when god mode is not enabled
	EXPECT_FALSE(antiCheatAltImpl(GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheatAlt_WrongGodModeFlag_ReturnsFalse)
{
	// Arrange: Different god mode flag is enabled
	m_playerControl[0] = PlayerControlFlags::Human;
	m_playerControl[1] = ComputerControlFlags;
	m_godModes = GodModeFlags::InfiniteBattery;

	// Act & Assert: Should return false when requesting different god mode
	EXPECT_FALSE(antiCheatAltImpl(GodModeFlags::NoDamage, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheatAlt_Player0AI_Player1Human_ReturnsTrue)
{
	// Arrange: Player 0 is AI, Player 1 is human
	m_playerControl[0] = ComputerControlFlags;
	m_playerControl[1] = PlayerControlFlags::Human;
	m_godModes = GodModeFlags::InfiniteBattery;

	// Act & Assert: Should allow cheat for human vs AI
	EXPECT_TRUE(antiCheatAltImpl(GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheatAlt_Player0Human_Player1AI_ReturnsTrue)
{
	// Arrange: Player 0 is human, Player 1 is AI
	m_playerControl[0] = PlayerControlFlags::Human;
	m_playerControl[1] = ComputerControlFlags;
	m_godModes = GodModeFlags::InfiniteBattery;

	// Act & Assert: Should allow cheat for human vs AI
	EXPECT_TRUE(antiCheatAltImpl(GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheatAlt_BothPlayersHuman_ReturnsFalse)
{
	// Arrange: Both players are human-controlled
	m_playerControl[0] = PlayerControlFlags::Human;
	m_playerControl[1] = PlayerControlFlags::Human;
	m_godModes = GodModeFlags::InfiniteBattery;

	// Act & Assert: No AI player, so no cheat should apply
	EXPECT_FALSE(antiCheatAltImpl(GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheatAlt_AllGodModesEnabled_ReturnsTrue)
{
	// Arrange: All god modes enabled
	m_playerControl[0] = ComputerControlFlags;
	m_playerControl[1] = PlayerControlFlags::Human;
	m_godModes = GodModeFlags::All;

	// Act & Assert: Should work with any god mode flag
	EXPECT_TRUE(antiCheatAltImpl(GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
	EXPECT_TRUE(antiCheatAltImpl(GodModeFlags::NoDamage, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheatAlt_CyborgControl_TreatedAsComputer)
{
	// Arrange: Cyborg control should be treated as computer control
	m_playerControl[0] = PlayerControlFlags::Cyborg;
	m_playerControl[1] = PlayerControlFlags::Human;
	m_godModes = GodModeFlags::InfiniteBattery;

	// Act & Assert: PlayerControlFlags::Cyborg is part of ComputerControlFlags
	EXPECT_TRUE(antiCheatAltImpl(GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheatAlt_PsytronControl_TreatedAsComputer)
{
	// Arrange: Psytron control should be treated as computer control
	m_playerControl[0] = PlayerControlFlags::Psytron;
	m_playerControl[1] = PlayerControlFlags::Human;
	m_godModes = GodModeFlags::InfiniteBattery;

	// Act & Assert: PlayerControlFlags::Psytron is part of ComputerControlFlags
	EXPECT_TRUE(antiCheatAltImpl(GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheatAlt_NetworkControl_ReturnsFalse)
{
	// Arrange: Network control is not considered AI
	m_playerControl[0] = PlayerControlFlags::Network;
	m_playerControl[1] = PlayerControlFlags::Human;
	m_godModes = GodModeFlags::InfiniteBattery;

	// Act & Assert: Network control is not ComputerControlFlags
	EXPECT_FALSE(antiCheatAltImpl(GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, AntiCheatAlt_MixedControlFlags_ReturnsCorrectly)
{
	// Arrange: Player has multiple control flags (e.g., CYBORG + something else)
	m_playerControl[0] = PlayerControlFlags::Cyborg | PlayerControlFlags::DifficultyStandard;
	m_playerControl[1] = PlayerControlFlags::Human;
	m_godModes = GodModeFlags::InfiniteBattery;

	// Act & Assert: Should still detect AI control
	EXPECT_TRUE(antiCheatAltImpl(GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

// ============================================================================
// Edge case and comprehensive tests
// ============================================================================

TEST_F(AntiCheatTest, EdgeCase_NeutralPlayerNumber)
{
	// Arrange: Element with neutral player number (-1)
	m_playerControl[0] = ComputerControlFlags;
	m_playerControl[1] = PlayerControlFlags::Human;
	m_godModes = GodModeFlags::InfiniteBattery;
	m_testElement.playerNr = NeutralPlayerNum;

	// Act & Assert: Neutral elements should not get cheats
	EXPECT_FALSE(antiCheatImpl(&m_testElement, false, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
	EXPECT_FALSE(antiCheatImpl(&m_testElement, true, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, EdgeCase_MultipleFlagsInGodMode)
{
	// Arrange: Multiple god mode flags enabled
	m_playerControl[0] = ComputerControlFlags;
	m_playerControl[1] = PlayerControlFlags::Human;
	m_godModes = GodModeFlags::InfiniteBattery | GodModeFlags::NoDamage;
	m_testElement.playerNr = 1;

	// Act & Assert: Should work with either flag
	EXPECT_TRUE(antiCheatImpl(&m_testElement, false, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
	EXPECT_TRUE(antiCheatImpl(&m_testElement, false, GodModeFlags::NoDamage, m_playerControl, m_godModes));
}

TEST_F(AntiCheatTest, Consistency_AntiCheatVsAntiCheatAlt)
{
	// Verify that antiCheat and antiCheatAlt are consistent in their
	// handling of player control checks

	// Setup: One AI, one human
	m_playerControl[0] = ComputerControlFlags;
	m_playerControl[1] = PlayerControlFlags::Human;
	m_godModes = GodModeFlags::InfiniteBattery;

	// antiCheatAlt should return true for human vs AI
	EXPECT_TRUE(antiCheatAltImpl(GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));

	// antiCheat should return true when Element belongs to human player
	m_testElement.playerNr = 1;
	EXPECT_TRUE(antiCheatImpl(&m_testElement, false, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));

	// antiCheat should return false when Element belongs to AI player
	m_testElement.playerNr = 0;
	EXPECT_FALSE(antiCheatImpl(&m_testElement, false, GodModeFlags::InfiniteBattery, m_playerControl, m_godModes));
}

} // namespace uqm