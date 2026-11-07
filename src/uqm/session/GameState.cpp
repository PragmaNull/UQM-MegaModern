// The state of one game, as a save file holds it.

#include "GameState.h"
#include "core/log/log.h"


namespace uqm
{
uqstl::optional<uint32_t> GameState::flag(uqstl::string_view name) const
{
	if (const auto gameFlag {findGameFlag(name)})
	{
		return flag(*gameFlag);
	}
	const auto found {m_otherFlags.find(name)};
	if (found == m_otherFlags.end())
	{
		return uqstl::nullopt;
	}
	return found->second;
}

void GameState::setFlag(uqstl::string_view name, uint32_t value)
{
	if (const auto gameFlag {findGameFlag(name)})
	{
		setFlag(*gameFlag, value);
		return;
	}
	const auto found {m_otherFlags.find(name)};
	if (found == m_otherFlags.end())
	{
		m_otherFlags.emplace(uqstl::string {name}, value);
	}
	else
	{
		found->second = value;
	}
}

void GameState::clearFlags() noexcept
{
	m_flagValues.fill(0);
	m_otherFlags.clear();
}


uint32_t getGameStateUint(const char* name)
{
	return uqm::gameState().flag(name).value_or(0);
}

void setGameStateUint(const char* name, uint32_t val)
{
#ifdef STATE_DEBUG
	uqm::log::debug("State '{}' set to {}.", name, val);
#endif
	uqm::gameState().setFlag(name, val);
}

} // namespace uqm
