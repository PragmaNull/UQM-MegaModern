#pragma once
#ifndef UQM_SUPERMELEE_MELEESHIP_H
#define UQM_SUPERMELEE_MELEESHIP_H

#include "core/types/enum.h"
#include <cstdint>

#if 0 //defined(__cplusplus)
extern "C" {
#endif

enum class MeleeShip
{
	Androsynth,
	Arilou,
	Chenjesu,
	Chmmr,
	Druuge,
	Earthling,
	Ilwrath,
	KohrAh,
	Melnorme,
	Mmrnmhrm,
	Mycon,
	Orz,
	Pkunk,
	Shofixti,
	Slylandro,
	Spathi,
	Supox,
	Syreen,
	Thraddash,
	Umgah,
	Urquan,
	Utwig,
	Vux,
	Yehat,
	Zoqfotpik,

	Unset = ((uint8_t)~0) - 1,
	// Used with the Update protocol, to register in the sentTeam
	None = (uint8_t)~0
	// Empty fleet position.
};
consteval bool isArithmeticEnum(MeleeShip) { return true; }
inline constexpr auto NumMeleeShips {MeleeShip::Zoqfotpik + 1};
inline MeleeShip operator+=(MeleeShip lhs, int rhs) { return static_cast<MeleeShip>(static_cast<int>(lhs) + rhs); }
inline MeleeShip operator-=(MeleeShip lhs, int rhs) { return static_cast<MeleeShip>(static_cast<int>(lhs) - rhs); }
inline MeleeShip operator++(MeleeShip val) { return static_cast<MeleeShip>(static_cast<int>(val) + 1); }
inline MeleeShip operator--(MeleeShip val) { return static_cast<MeleeShip>(static_cast<int>(val) - 1); }

static inline bool MeleeShip_valid(MeleeShip ship)
{
	return (ship < NumMeleeShips) || (ship == MeleeShip::None);
}

#if 0 //defined(__cplusplus)
}
#endif

#endif // UQM_SUPERMELEE_MELEESHIP_H
