// The game state flags, such as SHOFIXTI_VISITS, by enum and by name.

#include "GameFlags.h"

#include "core/stl/map.h"


namespace uqm
{
uqstl::optional<GameFlag> findGameFlag(uqstl::string_view name)
{
	static const uqstl::map<uqstl::string_view, GameFlag> s_byName {[] {
		uqstl::map<uqstl::string_view, GameFlag> byName {};
		for (size_t index {0}; index < c_gameFlagCount; ++index)
		{
			byName.emplace(c_gameFlags[index].name, static_cast<GameFlag>(index));
		}
		return byName;
	}()};

	const auto found {s_byName.find(name)};
	if (found == s_byName.end())
	{
		return uqstl::nullopt;
	}
	return found->second;
}

} // namespace uqm
