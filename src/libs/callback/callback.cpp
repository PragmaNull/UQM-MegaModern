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

#include "callback.h"

#include <assert.h>
#include <cstdint>
#include <stdlib.h>
#include <sys/types.h>

#include "core/thread/Mutex.h"


#define CALLBACK_INTERNAL

struct CallbackLink
{
	CallbackLink* next;
	CallbackFunction callback;
	CallbackArg arg;
};

static uqm::Mutex callbackListLock {};

static CallbackLink* callbacks UQM_GUARDED_BY(callbackListLock);
static CallbackLink** callbacksEnd UQM_GUARDED_BY(callbackListLock);
static CallbackLink* const* callbacksProcessEnd UQM_GUARDED_BY(callbackListLock);

void Callback_init(void)
{
	const uqm::LockGuard lock {callbackListLock};
	callbacks = nullptr;
	callbacksEnd = &callbacks;
	callbacksProcessEnd = &callbacks;
}

void Callback_uninit(void)
{
	// TODO: cleanup the queue?
}

// Callbacks are guaranteed to be called in the order that they are queued.
CallbackID Callback_add(CallbackFunction callback, CallbackArg arg)
{
	CallbackLink* link = (CallbackLink*)(malloc(sizeof(CallbackLink)));
	link->callback = callback;
	link->arg = arg;
	link->next = nullptr;

	const uqm::LockGuard lock {callbackListLock};
	*callbacksEnd = link;
	callbacksEnd = &link->next;
	return (CallbackID)link;
}


static void CallbackLink_delete(CallbackLink* link)
{
	free(link);
}

static CallbackLink** CallbackLink_find(CallbackLink* link) UQM_REQUIRES(callbackListLock)
{
	for (CallbackLink** ptr {&callbacks}; *ptr != nullptr; ptr = &(*ptr)->next)
	{
		if (*ptr == link)
		{
			return ptr;
		}
	}
	return nullptr;
}

// Removes 'link' from the queue. Returns false if 'link' is not in the queue.
static bool CallbackLink_unlink(CallbackLink* link)
{
	const uqm::LockGuard lock {callbackListLock};

	CallbackLink** linkPtr {CallbackLink_find(link)};
	if (linkPtr == nullptr)
	{
		return false;
	}

	if (callbacksEnd == &(*linkPtr)->next)
	{
		callbacksEnd = linkPtr;
	}
	if (callbacksProcessEnd == &(*linkPtr)->next)
	{
		callbacksProcessEnd = linkPtr;
	}
	*linkPtr = (*linkPtr)->next;
	return true;
}

bool Callback_remove(CallbackID id)
{
	CallbackLink* link = (CallbackLink*)id;
	if (!CallbackLink_unlink(link))
	{
		return false;
	}
	CallbackLink_delete(link);
	return true;
}

static inline void CallbackLink_doCallback(CallbackLink* link)
{
	(link->callback)(link->arg);
}

// Removes the first callback from the queue and returns it. Returns nullptr if no callback before
// 'callbacksProcessEnd' is left.
static CallbackLink* CallbackList_popForProcess()
{
	const uqm::LockGuard lock {callbackListLock};
	if (callbacksProcessEnd == &callbacks)
	{
		return nullptr;
	}
	assert(callbacks != nullptr);
	// If callbacks == nullptr, then callbacksProcessEnd == &callbacks
	CallbackLink* const link {callbacks};
	callbacks = link->next;
	if (callbacksEnd == &link->next)
	{
		callbacksEnd = &callbacks;
	}
	if (callbacksProcessEnd == &link->next)
	{
		callbacksProcessEnd = &callbacks;
	}
	return link;
}

// Call all queued callbacks currently in the queue. Callbacks queued
// from inside the called functions will not be processed until the next
// call of Callback_process().
// It is allowed to remove callbacks from inside the called functions.
// NB: Callback_process() must never be called from more than one thread
//     at the same time. It's the only sensible way to ensure that the
//     callbacks are called in the order in which they were queued.
//     It is however allowed to call Callback_process() from inside the
//     callback function called by Callback_process() itself.
void Callback_process(void)
{
	// We set 'callbacksProcessEnd' to callbacksEnd. Callbacks added
	// from inside a callback function will be placed after
	// callbacksProcessEnd, and will hence not be processed this
	// call of Callback_process().
	{
		const uqm::LockGuard lock {callbackListLock};
		callbacksProcessEnd = callbacksEnd;
	}

	while (CallbackLink* const link {CallbackList_popForProcess()})
	{
		CallbackLink_doCallback(link);
		CallbackLink_delete(link);
	}
}

bool Callback_haveMore(void)
{
	const uqm::LockGuard lock {callbackListLock};
	return callbacks != nullptr;
}
