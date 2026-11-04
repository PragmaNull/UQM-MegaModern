// Constants for the seeds of the random number generators.
#pragma once
#ifndef UQM_LIBS_MATH_RANDOMDEFS_H_
#define UQM_LIBS_MATH_RANDOMDEFS_H_

#define PrimeA 16807
#define MAX_SEED 2147483644
#define MIN_SEED 3
#define SANE_SEED(a) (((a) < MIN_SEED || (a) > MAX_SEED) ? false : true)

#endif /* UQM_LIBS_MATH_RANDOMDEFS_H_ */
