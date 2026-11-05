// Constants for the seeds of the random number generators.
#pragma once
#ifndef UQM_LIBS_MATH_RANDOMDEFS_H_
#define UQM_LIBS_MATH_RANDOMDEFS_H_

inline constexpr auto PrimeA {16807};
inline constexpr auto MaxSeed {2147483644};
inline constexpr auto MinSeed {3};
constexpr auto SaneSeed(auto a) { return (((a) < MinSeed || (a) > MaxSeed) ? false : true); }

#endif /* UQM_LIBS_MATH_RANDOMDEFS_H_ */
