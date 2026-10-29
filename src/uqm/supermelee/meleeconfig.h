// Converts SuperMelee sides and teams to and from the melee.json format.
#pragma once
#ifndef UQM_SUPERMELEE_MELEECONFIG_H_
#define UQM_SUPERMELEE_MELEECONFIG_H_

#include "core/stl/stl.h"
#include "core/stl/string.h"
#include "core/stl/vector.h"
#include "uqm/inteldefs.h"
#include "uqm/supermelee/meleeship.h"

#include <optional>

namespace uqm
{

struct MeleeSideConfig
{
	PlayerControlFlags control {PlayerControlFlags::None};
	uqstl::string teamName {};
	// MELEE_NONE marks an empty fleet slot.
	uqstl::vector<MeleeShip> ships {};
};

[[nodiscard]] uqstl::string meleeConfigToJson(uqstl::span<const MeleeSideConfig> sides);

// Returns nullopt if 'json' is not a valid melee configuration.
[[nodiscard]] uqstl::optional<uqstl::vector<MeleeSideConfig>> meleeConfigFromJson(uqstl::string_view json);

} // namespace uqm

#endif // UQM_SUPERMELEE_MELEECONFIG_H_
