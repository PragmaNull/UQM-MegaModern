// Mutexes and lock guards that the Clang thread safety analysis can check.
#pragma once
#ifndef UQM_CORE_THREAD_MUTEX_H_
#define UQM_CORE_THREAD_MUTEX_H_

#include "core/stl/stl.h"
#include "core/thread/ThreadSafety.h"

#include <mutex>

namespace uqm
{

class UQM_CAPABILITY("mutex") Mutex
{
public:
	Mutex() = default;
	Mutex(const Mutex&) = delete;
	Mutex& operator=(const Mutex&) = delete;

	void lock() UQM_ACQUIRE()
	{
		m_mutex.lock();
	}

	void unlock() UQM_RELEASE()
	{
		m_mutex.unlock();
	}

	bool try_lock() UQM_TRY_ACQUIRE(true)
	{
		return m_mutex.try_lock();
	}

private:
	uqstl::mutex m_mutex {};
};

// A thread that holds the mutex can lock it again.
class UQM_CAPABILITY("mutex") UQM_REENTRANT_CAPABILITY RecursiveMutex
{
public:
	RecursiveMutex() = default;
	RecursiveMutex(const RecursiveMutex&) = delete;
	RecursiveMutex& operator=(const RecursiveMutex&) = delete;

	void lock() UQM_ACQUIRE()
	{
		m_mutex.lock();
	}

	void unlock() UQM_RELEASE()
	{
		m_mutex.unlock();
	}

	bool try_lock() UQM_TRY_ACQUIRE(true)
	{
		return m_mutex.try_lock();
	}

private:
	uqstl::recursive_mutex m_mutex {};
};

// Holds the lock from construction to destruction.
template <typename MutexType>
class UQM_SCOPED_CAPABILITY LockGuard
{
public:
	explicit LockGuard(MutexType& mutex) UQM_ACQUIRE(mutex)
		: m_mutex {mutex}
	{
		m_mutex.lock();
	}

	~LockGuard() UQM_RELEASE()
	{
		m_mutex.unlock();
	}

	LockGuard(const LockGuard&) = delete;
	LockGuard& operator=(const LockGuard&) = delete;

private:
	MutexType& m_mutex;
};

} // namespace uqm

#endif // UQM_CORE_THREAD_MUTEX_H_
