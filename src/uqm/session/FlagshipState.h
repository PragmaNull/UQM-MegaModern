// Where the flagship is, where it goes, and how it moves.

#pragma once
#ifndef UQM_SESSION_FLAGSHIPSTATE_H
#define UQM_SESSION_FLAGSHIPSTATE_H

#include <cstdint>
#include "libs/gfxlib.h"
#include "uqm/velocity.h"

namespace uqm
{

// The point on the star map that the autopilot flies to.
class Autopilot
{
public:
	[[nodiscard]] bool hasTarget() const noexcept
	{
		return m_target.x != c_none && m_target.y != c_none;
	}

	// The target. Both coordinates are -1 if there is no target.
	[[nodiscard]] GfxPoint target() const noexcept
	{
		return m_target;
	}

	void setTarget(GfxPoint target) noexcept
	{
		m_target = target;
	}

	void clear() noexcept
	{
		m_target = {c_none, c_none};
	}

private:
	static constexpr int16_t c_none {-1};
	GfxPoint m_target {c_none, c_none};
};

struct FlagshipState
{
	Autopilot autopilot {};
	// The location in the star system, in interplanetary units.
	GfxPoint ipLocation {};
	GfxStamp shipStamp {};
	// The facing of the ship plus 1, or 0 if the ship does not enter a star system again.
	uint16_t shipFacing {};
	// The planet that the flagship is at, plus 1. 0 if it is at no planet.
	uint8_t ipPlanet {};
	// The world that the flagship orbits: 1 for the planet, more for its moons. 0 if it is not in orbit.
	uint8_t inOrbit {};
	VelocityDesc velocity {};
};

} // namespace uqm

#endif // UQM_SESSION_FLAGSHIPSTATE_H
