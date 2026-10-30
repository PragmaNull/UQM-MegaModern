// Threads, locks and sleep functions for the game, on top of the C++ standard library.
#ifndef LIBS_THREADLIB_H_
#define LIBS_THREADLIB_H_

#include "libs/time/timelib.h"

#include <cstdint>

void InitThreadSystem(void);
void UnInitThreadSystem(void);

typedef int (*ThreadFunction)(void*);

typedef void* Thread;
typedef void* Mutex;
typedef void* Semaphore;
typedef void* RecursiveMutex;
typedef void* CondVar;

/* Local data associated with each thread */
typedef struct _threadLocal
{
	Semaphore flushSem;
} ThreadLocal;

/* The classes of synchronization objects */
enum
{
	SYNC_CLASS_TOPLEVEL = (1 << 0), /* Exposed to the game logic */
	SYNC_CLASS_AUDIO = (1 << 1),	/* Involves the audio system */
	SYNC_CLASS_VIDEO = (1 << 2),	/* Involves the video system. */
	SYNC_CLASS_RESOURCE = (1 << 3)	/* Involves system resources */
};

Thread CreateThread_Core(ThreadFunction func, void* data, const char* name);
Semaphore CreateSemaphore_Core(uint32_t initial);
Mutex CreateMutex_Core(void);
RecursiveMutex CreateRecursiveMutex_Core(void);
CondVar CreateCondVar_Core(void);

// The stack size, the names of synchronization objects and the sync classes are not used.
#define CREATETHREAD(func, data, stackSize, name) \
	CreateThread_Core((func), (data), (name))
#define STARTTHREAD(func, data, stackSize, name) \
	(void)CreateThread_Core((func), (data), (name))
#define CREATESEMAPHORE(initial, name, syncClass) \
	CreateSemaphore_Core((initial))
#define CREATEMUTEX(name, syncClass) \
	CreateMutex_Core()
#define CREATERECURSIVEMUTEX(name, syncClass) \
	CreateRecursiveMutex_Core()
#define CREATECONDVAR(name, syncClass) \
	CreateCondVar_Core()

// Returns the data of the calling thread. Each thread, the main thread too, has its own.
ThreadLocal* GetMyThreadLocal(void);

void HibernateThread(TimePeriod timePeriod);
void HibernateThreadUntil(TimeCount wakeTime);
void SleepThread(TimePeriod timePeriod);
void SleepThreadUntil(TimeCount wakeTime);
void TaskSwitch(void);

// Joins the threads that have ended. Call it from the main thread.
void ProcessThreadLifecycles(void);

void DestroySemaphore(Semaphore sem);
void SetSemaphore(Semaphore sem);
void ClearSemaphore(Semaphore sem);

void DestroyMutex(Mutex sem);
void LockMutex(Mutex sem);
void UnlockMutex(Mutex sem);

void DestroyRecursiveMutex(RecursiveMutex m);
void LockRecursiveMutex(RecursiveMutex m);
void UnlockRecursiveMutex(RecursiveMutex m);
int GetRecursiveMutexDepth(RecursiveMutex m);

void DestroyCondVar(CondVar);
void WaitCondVar(CondVar);
void SignalCondVar(CondVar);
void BroadcastCondVar(CondVar);

#endif /* LIBS_THREADLIB_H_ */
