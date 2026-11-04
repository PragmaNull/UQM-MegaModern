// Macros for the Clang thread safety analysis. Other compilers ignore them.
#pragma once
#ifndef UQM_CORE_THREAD_THREADSAFETY_H_
#define UQM_CORE_THREAD_THREADSAFETY_H_

#if defined(__clang__)
#define UQM_THREAD_ANNOTATION(x) __attribute__((x))
#else
#define UQM_THREAD_ANNOTATION(x)
#endif

#if defined(__clang__) && __has_attribute(reentrant_capability)
#define UQM_REENTRANT_CAPABILITY UQM_THREAD_ANNOTATION(reentrant_capability)
#else
#define UQM_REENTRANT_CAPABILITY
#endif

// A type that is a lock.
#define UQM_CAPABILITY(name) UQM_THREAD_ANNOTATION(capability(name))
// A type that holds a lock for its lifetime.
#define UQM_SCOPED_CAPABILITY UQM_THREAD_ANNOTATION(scoped_lockable)

// Data that a thread can use only while it holds the lock.
#define UQM_GUARDED_BY(lock) UQM_THREAD_ANNOTATION(guarded_by(lock))
// Data, through a pointer, that a thread can use only while it holds the lock.
#define UQM_PT_GUARDED_BY(lock) UQM_THREAD_ANNOTATION(pt_guarded_by(lock))

// The caller must hold the locks.
#define UQM_REQUIRES(...) UQM_THREAD_ANNOTATION(requires_capability(__VA_ARGS__))
// The caller must not hold the locks.
#define UQM_EXCLUDES(...) UQM_THREAD_ANNOTATION(locks_excluded(__VA_ARGS__))

// The function locks or unlocks the locks.
#define UQM_ACQUIRE(...) UQM_THREAD_ANNOTATION(acquire_capability(__VA_ARGS__))
#define UQM_RELEASE(...) UQM_THREAD_ANNOTATION(release_capability(__VA_ARGS__))
#define UQM_TRY_ACQUIRE(...) UQM_THREAD_ANNOTATION(try_acquire_capability(__VA_ARGS__))

// The function returns a reference to the lock.
#define UQM_RETURN_CAPABILITY(lock) UQM_THREAD_ANNOTATION(lock_returned(lock))

// The analysis does not check the function.
#define UQM_NO_THREAD_SAFETY_ANALYSIS UQM_THREAD_ANNOTATION(no_thread_safety_analysis)

#endif // UQM_CORE_THREAD_THREADSAFETY_H_
