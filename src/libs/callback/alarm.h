/*
 *  Copyright 2006  Serge van den Boom <svdb@stack.nl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef LIBS_CALLBACK_ALARM_H_
#define LIBS_CALLBACK_ALARM_H_

#include "port.h"
#include <cstdint>

typedef uint32_t AlarmTime;
static inline uint32_t
alarmTimeToMsUint32(AlarmTime time)
{
	return (uint32_t)time;
}

typedef struct Alarm Alarm;
typedef void* AlarmCallbackArg;
typedef void (*AlarmCallback)(AlarmCallbackArg arg);

struct Alarm
{
	AlarmTime time;
	AlarmCallback callback;
	AlarmCallbackArg arg;
};

void Alarm_init(void);
void Alarm_uninit(void);
Alarm* Alarm_addAbsoluteMs(uint32_t ms, AlarmCallback callback, AlarmCallbackArg arg);
Alarm* Alarm_addRelativeMs(uint32_t ms, AlarmCallback callback, AlarmCallbackArg arg);
void Alarm_remove(Alarm* alarm);
bool Alarm_processOne(void);
uint32_t Alarm_timeBeforeNextMs(void);

#endif /* LIBS_CALLBACK_ALARM_H_ */
