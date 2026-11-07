// The game state flags, such as SHOFIXTI_VISITS, by enum and by name.

#pragma once
#ifndef UQM_SESSION_GAMEFLAGS_H
#define UQM_SESSION_GAMEFLAGS_H

#include <cstddef>
#include <cstdint>
#include "core/stl/optional.h"
#include "core/stl/stl.h"
#include "core/stl/string.h"

namespace uqm
{

enum class GameFlag : uint16_t
{
#define UQM_GAME_FLAG(name, bits) name,
#include "uqm/session/GameFlags.inc"
#undef UQM_GAME_FLAG
};

struct GameFlagInfo
{
	uqstl::string_view name;
	// The number of bits that a save keeps for the value.
	uint8_t bits;

	// The largest value that fits in the bits.
	[[nodiscard]] constexpr uint32_t maxValue() const noexcept
	{
		return bits >= 32 ? UINT32_MAX : (uint32_t {1} << bits) - 1;
	}
};

inline constexpr GameFlagInfo c_gameFlags[] {
#define UQM_GAME_FLAG(name, bits) {#name, bits},
#include "uqm/session/GameFlags.inc"
#undef UQM_GAME_FLAG
};

inline constexpr size_t c_gameFlagCount {uqstl::size(c_gameFlags)};

[[nodiscard]] constexpr const GameFlagInfo& gameFlagInfo(GameFlag flag) noexcept
{
	return c_gameFlags[uqstl::to_underlying(flag)];
}

// The flag with this name, or nullopt if no flag has the name.
[[nodiscard]] uqstl::optional<GameFlag> findGameFlag(uqstl::string_view name);

} // namespace uqm

#endif // UQM_SESSION_GAMEFLAGS_H
