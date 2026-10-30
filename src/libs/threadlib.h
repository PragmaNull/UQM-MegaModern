// Sleep functions that count in game ticks.
#ifndef LIBS_THREADLIB_H_
#define LIBS_THREADLIB_H_

#include "libs/time/timelib.h"

// Sleep without processing asynchronous operations.
void HibernateThread(TimePeriod timePeriod);
void HibernateThreadUntil(TimeCount wakeTime);

// Sleep, and process asynchronous operations until the wake time.
void SleepThread(TimePeriod timePeriod);
void SleepThreadUntil(TimeCount wakeTime);

// Sleeps for a short time, so that busy-wait loops do not use all of a CPU core.
void TaskSwitch(void);

#endif /* LIBS_THREADLIB_H_ */
