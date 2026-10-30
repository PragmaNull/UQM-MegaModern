// Tests for the mutexes, lock guards and thread utilities.
#include <gtest/gtest.h>

#include "core/thread/Mutex.h"
#include "core/thread/Thread.h"

#include <atomic>
#include <vector>

namespace uqm
{

namespace
{

struct Counter
{
	Mutex mutex {};
	int value UQM_GUARDED_BY(mutex) {};
};

} // namespace

TEST(ThreadTest, LockGuardProtectsCounter)
{
	Counter counter {};
	{
		uqstl::vector<uqstl::jthread> threads {};
		for (int i {}; i < 4; ++i)
		{
			threads.push_back(makeThread("counter", [&](uqstl::stop_token) {
				for (int j {}; j < 1000; ++j)
				{
					const LockGuard lock {counter.mutex};
					++counter.value;
				}
			}));
		}
	}
	const LockGuard lock {counter.mutex};
	EXPECT_EQ(counter.value, 4000);
}

TEST(ThreadTest, RecursiveMutexLocksTwice)
{
	RecursiveMutex mutex {};
	const LockGuard outer {mutex};
	const LockGuard inner {mutex};
	SUCCEED();
}

TEST(ThreadTest, TryLockFailsWhileOtherThreadHoldsMutex)
{
	Mutex mutex {};
	const LockGuard lock {mutex};
	bool locked {true};
	makeThread("try", [&](uqstl::stop_token) {
		locked = false;
		if (mutex.try_lock())
		{
			locked = true;
			mutex.unlock();
		}
	}).join();
	EXPECT_FALSE(locked);
}

TEST(ThreadTest, MakeThreadStopsOnDestruction)
{
	uqstl::atomic<bool> stopped {false};
	{
		const uqstl::jthread thread {makeThread("stopper", [&](uqstl::stop_token stopToken) {
			while (!stopToken.stop_requested())
			{
				taskSwitch();
			}
			stopped = true;
		})};
	}
	EXPECT_TRUE(stopped);
}

} // namespace uqm
