// Implements the thread library with the C++ standard library.
#include "libs/threadlib.h"

#include "core/log/log.h"
#include "core/platform/platform.h"
#include "core/stl/string.h"
#include "core/stl/vector.h"
#include "libs/async.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <semaphore>
#include <thread>

namespace
{

struct ThreadData
{
	uqstl::string name;
	uqstl::thread thread {};
	// The thread does not end before the creator has stored 'thread'.
	uqstl::binary_semaphore stored {0};
};

struct RecursiveMutexData
{
	uqstl::recursive_mutex mutex {};
	uqstl::atomic<uqstl::thread::id> owner {};
	// Changed only by the owner.
	int depth {};
};

struct CondVarData
{
	uqstl::mutex mutex {};
	uqstl::condition_variable cond {};
};

using CountingSemaphore = uqstl::counting_semaphore<>;

uqstl::mutex s_finishedMutex {};
uqstl::vector<ThreadData*> s_finished {};

void finishThread(ThreadData* thread)
{
	const uqstl::scoped_lock lock {s_finishedMutex};
	s_finished.push_back(thread);
}

uqstl::chrono::milliseconds ticksToDuration(TimePeriod ticks)
{
	return uqstl::chrono::milliseconds {static_cast<int64_t>(ticks) * 1000 / GameTicksPerSecond};
}

void sleepTicks(TimePeriod ticks)
{
	uqstl::this_thread::sleep_for(ticksToDuration(ticks));
}

void sleepUntilTicks(TimeCount wakeTime)
{
	const TimeCount now {GetTimeCounter()};
	if (wakeTime <= now)
	{
		TaskSwitch();
	}
	else
	{
		sleepTicks(wakeTime - now);
	}
}

} // namespace

void InitThreadSystem(void)
{
}

void UnInitThreadSystem(void)
{
	ProcessThreadLifecycles();
}

Thread CreateThread_Core(ThreadFunction func, void* data, const char* name)
{
	auto* thread {new ThreadData {name ? name : "UQM thread"}};
	thread->thread = uqstl::thread {[thread, func, data] {
		uqm::setCurrentThreadName(thread->name.c_str());
		(void)func(data);
		thread->stored.acquire();
		finishThread(thread);
	}};
	thread->stored.release();
	return thread;
}

void ProcessThreadLifecycles(void)
{
	uqstl::vector<ThreadData*> finished {};
	{
		const uqstl::scoped_lock lock {s_finishedMutex};
		finished.swap(s_finished);
	}
	for (ThreadData* thread : finished)
	{
		thread->thread.join();
		delete thread;
	}
}

ThreadLocal* GetMyThreadLocal(void)
{
	thread_local CountingSemaphore flushSem {0};
	thread_local ThreadLocal local {&flushSem};
	return &local;
}

void HibernateThread(TimePeriod timePeriod)
{
	sleepTicks(timePeriod);
}

void HibernateThreadUntil(TimeCount wakeTime)
{
	sleepUntilTicks(wakeTime);
}

void SleepThread(TimePeriod timePeriod)
{
	SleepThreadUntil(GetTimeCounter() + timePeriod);
}

// Sleep until wakeTime, but call asynchronous operations until then.
void SleepThreadUntil(TimeCount wakeTime)
{
	for (;;)
	{
		Async_process();

		const TimeCount now {GetTimeCounter()};
		if (wakeTime <= now)
		{
			return;
		}

		const uint32_t nextTimeMs {Async_timeBeforeNextMs()};
		// Overflow-safe conversion.
		TimeCount nextTime {(nextTimeMs / 1000) * GameTicksPerSecond + ((nextTimeMs % 1000) * GameTicksPerSecond / 1000)};
		if (wakeTime < nextTime)
		{
			nextTime = wakeTime;
		}

		sleepUntilTicks(nextTime);
	}
}

void TaskSwitch(void)
{
	uqstl::this_thread::sleep_for(uqstl::chrono::milliseconds {1});
}

// Recursive, like the SDL mutexes that this replaces.
Mutex CreateMutex_Core(void)
{
	return new uqstl::recursive_mutex {};
}

void DestroyMutex(Mutex mutex)
{
	delete static_cast<uqstl::recursive_mutex*>(mutex);
}

void LockMutex(Mutex mutex)
{
	static_cast<uqstl::recursive_mutex*>(mutex)->lock();
}

void UnlockMutex(Mutex mutex)
{
	static_cast<uqstl::recursive_mutex*>(mutex)->unlock();
}

Semaphore CreateSemaphore_Core(uint32_t initial)
{
	return new CountingSemaphore {static_cast<ptrdiff_t>(initial)};
}

void DestroySemaphore(Semaphore sem)
{
	delete static_cast<CountingSemaphore*>(sem);
}

void SetSemaphore(Semaphore sem)
{
	static_cast<CountingSemaphore*>(sem)->acquire();
}

void ClearSemaphore(Semaphore sem)
{
	static_cast<CountingSemaphore*>(sem)->release();
}

RecursiveMutex CreateRecursiveMutex_Core(void)
{
	return new RecursiveMutexData {};
}

void DestroyRecursiveMutex(RecursiveMutex mutex)
{
	delete static_cast<RecursiveMutexData*>(mutex);
}

void LockRecursiveMutex(RecursiveMutex m)
{
	auto* mutex {static_cast<RecursiveMutexData*>(m)};
	mutex->mutex.lock();
	mutex->owner = uqstl::this_thread::get_id();
	++mutex->depth;
}

void UnlockRecursiveMutex(RecursiveMutex m)
{
	auto* mutex {static_cast<RecursiveMutexData*>(m)};
	if (mutex->owner != uqstl::this_thread::get_id())
	{
		uqm::log::debug("A thread attempted to unlock a recursive mutex that it does not hold.");
		return;
	}
	if (--mutex->depth == 0)
	{
		mutex->owner = uqstl::thread::id {};
	}
	mutex->mutex.unlock();
}

int GetRecursiveMutexDepth(RecursiveMutex m)
{
	auto* mutex {static_cast<RecursiveMutexData*>(m)};
	return mutex->owner == uqstl::this_thread::get_id() ? mutex->depth : 0;
}

CondVar CreateCondVar_Core(void)
{
	return new CondVarData {};
}

void DestroyCondVar(CondVar cv)
{
	delete static_cast<CondVarData*>(cv);
}

void WaitCondVar(CondVar c)
{
	auto* cv {static_cast<CondVarData*>(c)};
	uqstl::unique_lock lock {cv->mutex};
	cv->cond.wait(lock);
}

void SignalCondVar(CondVar cv)
{
	static_cast<CondVarData*>(cv)->cond.notify_one();
}

void BroadcastCondVar(CondVar cv)
{
	static_cast<CondVarData*>(cv)->cond.notify_all();
}
