//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		pthreadwrapper.cpp
//
//	@doc:
//		Wrapper for functions in pthread library
//
//---------------------------------------------------------------------------

#include <errno.h>

#include "gpos/assert.h"
#include "gpos/utils.h"
#include "gpos/common/pthreadwrapper.h"
#include "gpos/task/IWorker.h"

using namespace gpos;


//---------------------------------------------------------------------------
//	@function:
//		MutexTypeIsValid
//
//	@doc:
//		Return the specified type is a valid mutex type or not
//
//---------------------------------------------------------------------------
BOOL
gpos::pthread::MutexTypeIsValid
	(
	INT type
	)
{
	return
		PTHREAD_MUTEX_NORMAL == (type) ||
		PTHREAD_MUTEX_ERRORCHECK == (type) ||
		PTHREAD_MUTEX_RECURSIVE == (type) ||
		PTHREAD_MUTEX_DEFAULT == (type) ;
}


//---------------------------------------------------------------------------
//	@function:
//		IsValidDetachedState
//
//	@doc:
//		Return the specified state is a valid detached state or not
//
//---------------------------------------------------------------------------
BOOL
gpos::pthread::IsValidDetachedState
	(
	INT state
	)
{
	return
		PTHREAD_CREATE_DETACHED == (state) ||
		PTHREAD_CREATE_JOINABLE == (state);
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadAttrInit
//
//	@doc:
//		Initialize the thread attributes object
//
//---------------------------------------------------------------------------
INT
gpos::pthread::PthreadAttrInit
	(
	PTHREAD_ATTR_T *attr
	)
{
	GPOS_ASSERT_NO_SPINLOCK;
	GPOS_ASSERT(NULL != attr);

	INT res = pthread_attr_init(attr);

	GPOS_ASSERT
	(
		0 == res ||
		(EINTR != res && "Unexpected Error")
	);

	return res;
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadAttrDestroy
//
//	@doc:
//		Destroy the thread attributes object
//
//---------------------------------------------------------------------------
void
gpos::pthread::PthreadAttrDestroy
	(
	PTHREAD_ATTR_T *attr
	)
{
	GPOS_ASSERT_NO_SPINLOCK;
	GPOS_ASSERT(NULL != attr);

#ifdef GPOS_DEBUG
	INT res =
#endif // GPOS_DEBUG
	pthread_attr_destroy(attr);

	GPOS_ASSERT(0 == res && "function pthread_attr_destroy() failed");

}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadAttrGetDetachState
//
//	@doc:
//		Get the detachstate attribute
//
//---------------------------------------------------------------------------
INT
gpos::pthread::PthreadAttrGetDetachState
	(
	const PTHREAD_ATTR_T *attr,
	INT *state
	)
{
	GPOS_ASSERT_NO_SPINLOCK;
	GPOS_ASSERT(NULL != attr);
	GPOS_ASSERT(NULL != state);

	INT res = pthread_attr_getdetachstate(attr, state);

	GPOS_ASSERT
	(
		(0 == res && IsValidDetachedState(*state)) ||
		(0 != res && EINTR != res && "Unexpected Error")
	);

	return res;
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadAttrSetDetachState
//
//	@doc:
//		Set the detachstate attribute
//
//---------------------------------------------------------------------------
INT
gpos::pthread::PthreadAttrSetDetachState
	(
	PTHREAD_ATTR_T *attr,
	INT state
	)
{
	GPOS_ASSERT_NO_SPINLOCK;
	GPOS_ASSERT(NULL != attr);
	GPOS_ASSERT(IsValidDetachedState(state));

	INT res = pthread_attr_setdetachstate(attr, state);

	GPOS_ASSERT
	(
		0 == res ||
		(EINTR != res && "Unexpected Error")
	);

	return res;
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadMutexAttrInit
//
//	@doc:
//		Initialize the mutex attributes object
//
//---------------------------------------------------------------------------
INT
gpos::pthread::PthreadMutexAttrInit
	(
	PTHREAD_MUTEXATTR_T *attr
	)
{
	GPOS_ASSERT_NO_SPINLOCK;
	GPOS_ASSERT(NULL != attr);

	INT res = pthread_mutexattr_init(attr);

	GPOS_ASSERT(0 == res || ENOMEM == res);

	return res;
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadMutexAttrDestroy
//
//	@doc:
//		Destroy the mutex attributes object
//
//---------------------------------------------------------------------------
void
gpos::pthread::PthreadMutexAttrDestroy
	(
	PTHREAD_MUTEXATTR_T *attr
	)
{
	GPOS_ASSERT_NO_SPINLOCK;
	GPOS_ASSERT(NULL != attr);

#ifdef GPOS_DEBUG
	INT res =
#endif // GPOS_DEBUG
	pthread_mutexattr_destroy(attr);

	GPOS_ASSERT(0 == res && "function pthread_mutexattr_destroy() failed");
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadMutexAttrGettype
//
//	@doc:
//		Get the mutex type attribute
//
//---------------------------------------------------------------------------
void
gpos::pthread::PthreadMutexAttrGettype
	(
	const PTHREAD_MUTEXATTR_T *attr,
	INT *type
	)
{
	GPOS_ASSERT_NO_SPINLOCK;
	GPOS_ASSERT(NULL != attr);
	GPOS_ASSERT(NULL != type);

#ifdef GPOS_DEBUG
	INT res =
#endif // GPOS_DEBUG
#ifdef GPOS_FreeBSD
	pthread_mutexattr_gettype(const_cast<PTHREAD_MUTEXATTR_T*>(attr), type);
#else  // !GPOS_FreeBSD
	pthread_mutexattr_gettype(attr, type);
#endif

	GPOS_ASSERT(0 == res && MutexTypeIsValid(*type));

}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadMutexAttrSettype
//
//	@doc:
//		Set the mutex type attribute
//
//---------------------------------------------------------------------------
void
gpos::pthread::PthreadMutexAttrSettype
	(
	PTHREAD_MUTEXATTR_T *attr,
	INT type
	)
{
	GPOS_ASSERT_NO_SPINLOCK;
	GPOS_ASSERT(NULL != attr);
	GPOS_ASSERT(MutexTypeIsValid(type));

#ifdef GPOS_DEBUG
	INT res =
#endif // GPOS_DEBUG
	pthread_mutexattr_settype(attr, type);

	GPOS_ASSERT(0 == res);
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadMutexInit
//
//	@doc:
//		Initialize a mutex
//
//---------------------------------------------------------------------------
INT
gpos::pthread::PthreadMutexInit
	(
	PTHREAD_MUTEX_T *mutex,
	const PTHREAD_MUTEXATTR_T *attr
	)
{
	GPOS_ASSERT_NO_SPINLOCK;
	GPOS_ASSERT(NULL != attr);
	GPOS_ASSERT(NULL != mutex);

	INT res = pthread_mutex_init(mutex, attr);

	GPOS_ASSERT
	(
		0 == res ||
		(EINVAL != res && "Invalid mutex attr") ||
		(EBUSY != res && "Attempt to reinitialize") ||
		(EINTR != res && "Unexpected Error")
	);

	return res;
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadMutexDestroy
//
//	@doc:
//		Destroy a mutex
//
//---------------------------------------------------------------------------
void
gpos::pthread::PthreadMutexDestroy
	(
	PTHREAD_MUTEX_T *mutex
	)
{
	GPOS_ASSERT_NO_SPINLOCK;
	GPOS_ASSERT(NULL != mutex);

#ifdef GPOS_DEBUG
	INT res =
#endif // GPOS_DEBUG
	pthread_mutex_destroy(mutex);

	GPOS_ASSERT(0 == res && "function pthread_mutex_destroy() failed");
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadMutexLock
//
//	@doc:
//		Lock a mutex
//
//---------------------------------------------------------------------------
INT
gpos::pthread::PthreadMutexLock
	(
	PTHREAD_MUTEX_T *mutex
	)
{
	GPOS_ASSERT_NO_SPINLOCK;
	GPOS_ASSERT(NULL != mutex);

	INT res = pthread_mutex_lock(mutex);

	GPOS_ASSERT
	(
		res == 0 ||
		(EINVAL != res && "Uninitialized mutex structure") ||
		(EDEADLK != res && "The thread already owned the mutex") ||
		(EINTR != res && "Unexpected Error")
	);

	return res;
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadMutexTryLock
//
//	@doc:
//	        Try lock a mutex
//
//---------------------------------------------------------------------------
INT
gpos::pthread::PthreadMutexTryLock
	(
	PTHREAD_MUTEX_T *mutex
	)
{
	GPOS_ASSERT_NO_SPINLOCK;
	GPOS_ASSERT(NULL != mutex);

	INT res = pthread_mutex_trylock(mutex);

	GPOS_ASSERT
	(
		0 == res ||
		(EINVAL != res && "Uninitialized mutex structure") ||
		(EINTR != res && "Unexpected Error")
	);

	return res;
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadMutexUnlock
//
//	@doc:
//		Unlock a mutex
//
//---------------------------------------------------------------------------
INT
gpos::pthread::PthreadMutexUnlock
	(
	PTHREAD_MUTEX_T *mutex
	)
{
	GPOS_ASSERT_NO_SPINLOCK;
	GPOS_ASSERT(NULL != mutex);

	INT res = pthread_mutex_unlock(mutex);

	GPOS_ASSERT
	(
		0 == res ||
		(EINVAL != res && "Uninitialized mutex structure") ||
		(EPERM != res && "Mutex was not owned by thread") ||
		(EINTR != res && "Unexpected Error")
	);

	return res;
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadMutexTimedlock
//
//	@doc:
//		Lock the mutex object referenced by mutex with timeout
//
//---------------------------------------------------------------------------
#ifndef GPOS_Darwin
INT
gpos::pthread::PthreadMutexTimedlock
	(
	PTHREAD_MUTEX_T *mutex,
	const TIMESPEC *timeout
	)
{
	GPOS_ASSERT_NO_SPINLOCK;
	GPOS_ASSERT(NULL != mutex);

	INT res = pthread_mutex_timedlock(mutex, timeout);
	GPOS_ASSERT
	(
		0 == res ||
		(EINVAL != res && "Invalid mutex structure") ||
		(EINTR != res && "Unexpected Error")
	);

	return res;
}
#endif


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadCondInit
//
//	@doc:
//		Initialize condition variables
//
//---------------------------------------------------------------------------
INT
gpos::pthread::PthreadCondInit
	(
	PTHREAD_COND_T *__restrict cond,
	const PTHREAD_CONDATTR_T *__restrict attr
	)
{
	GPOS_ASSERT_NO_SPINLOCK;
	GPOS_ASSERT(NULL != cond);

	INT res = pthread_cond_init(cond, attr);

	GPOS_ASSERT
	(
		0 == res ||
		(EINVAL != res && "Invalid condition attr") ||
		(EBUSY != res && "Attempt to reinitialize") ||
		(EINTR != res && "Unexpected Error")
	);

	return res;
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadCondDestroy
//
//	@doc:
//		Destroy condition variables
//
//---------------------------------------------------------------------------
void
gpos::pthread::PthreadCondDestroy
	(
	PTHREAD_COND_T *cond
	)
{
	GPOS_ASSERT_NO_SPINLOCK;
	GPOS_ASSERT(NULL != cond);

#ifdef GPOS_DEBUG
	INT res =
#endif // GPOS_DEBUG
	pthread_cond_destroy(cond);

	GPOS_ASSERT(0 == res && "function pthread_attr_destroy() failed");
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadCondBroadcast
//
//	@doc:
//		Broadcast a condition
//
//---------------------------------------------------------------------------
INT
gpos::pthread::PthreadCondBroadcast
	(
	PTHREAD_COND_T *cond
	)
{
	GPOS_ASSERT_NO_SPINLOCK;
	GPOS_ASSERT(NULL != cond);

	INT res = pthread_cond_broadcast(cond);

	GPOS_ASSERT
	(
		0 == res ||
		(EINVAL != res && "Uninitialized condition structure") ||
		(EINTR != res && "Unexpected Error")
	);

	return res;
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadCondSignal
//
//	@doc:
//		Signal a condition
//
//---------------------------------------------------------------------------
INT
gpos::pthread::PthreadCondSignal
	(
	PTHREAD_COND_T *cond
	)
{
	GPOS_ASSERT_NO_SPINLOCK;
	GPOS_ASSERT(NULL != cond);

	INT res = pthread_cond_signal(cond);

	GPOS_ASSERT
	(
		0 == res ||
		(EINVAL != res && "Uninitialized condition structure") ||
		(EINTR != res && "Unexpected Error")
	);

	return res;
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadCondTimedWait
//
//	@doc:
//		Wait on a condition
//
//---------------------------------------------------------------------------
INT
gpos::pthread::PthreadCondTimedWait
	(
	PTHREAD_COND_T *__restrict cond,
	PTHREAD_MUTEX_T *__restrict mutex,
	const TIMESPEC *__restrict abstime
	)
{
	GPOS_ASSERT_NO_SPINLOCK;
	GPOS_ASSERT(NULL != cond);

	INT res = pthread_cond_timedwait(cond, mutex, abstime);

	GPOS_ASSERT
	(
		0 == res ||
		(EINVAL != res && "Invalid parameters") ||
		(EPERM != res && "Mutex was not owned by thread") ||
		(EINTR != res && "Unexpected Error")
	);

	return res;
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadCondWait
//
//	@doc:
//		Signal a condition
//
//---------------------------------------------------------------------------
INT
gpos::pthread::PthreadCondWait
	(
	PTHREAD_COND_T *__restrict cond,
	PTHREAD_MUTEX_T *__restrict mutex
	)
{
	GPOS_ASSERT_NO_SPINLOCK;
	GPOS_ASSERT(NULL != cond);

	INT res = pthread_cond_wait(cond, mutex);

	GPOS_ASSERT
	(
		0 == res ||
		(EINVAL != res && "Invalid parameters") ||
		(EPERM != res && "Mutex was not owned by thread") ||
		(EINTR != res && "Unexpected Error")
	);

	return res;
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadCreate
//
//	@doc:
//		thread creation
//
//---------------------------------------------------------------------------
INT
gpos::pthread::PthreadCreate
	(
	PTHREAD_T *__restrict thread,
	const PTHREAD_ATTR_T *__restrict attr,
	PthreadExecFn func,
	void *__restrict arg
	)
{
	GPOS_ASSERT_NO_SPINLOCK;
	GPOS_ASSERT(NULL != thread);

	INT res = pthread_create(thread, attr, func, arg);

	GPOS_ASSERT
	(
		0 == res ||
		(EINVAL != res && "Invalid pthread attr") ||
		(EINTR != res && "Unexpected Error")
	);

	return res;
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadJoin
//
//	@doc:
//		Wait for thread termination
//
//---------------------------------------------------------------------------
INT
gpos::pthread::PthreadJoin
	(
	PTHREAD_T thread,
	void **retval
	)
{
	GPOS_ASSERT_NO_SPINLOCK;

	INT res = pthread_join(thread, retval);

	GPOS_ASSERT
	(
		0 == res ||
		(EINVAL != res && "Thread is not joinable") ||
		(ESRCH != res && "No such thread") ||
		(EINTR != res && "Unexpected Error")
	);

	return res;
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadEqual
//
//	@doc:
//		Compare thread IDs
//
//---------------------------------------------------------------------------
BOOL
gpos::pthread::PthreadEqual
	(
	PTHREAD_T pthread1,
	PTHREAD_T pthread2
	)
{
	INT res = pthread_equal(pthread1, pthread2);

	GPOS_ASSERT(EINTR != res && "Unexpected Error");

	return 0 != res;
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadSelf
//
//	@doc:
//		Get the calling thread ID
//
//---------------------------------------------------------------------------
PTHREAD_T
gpos::pthread::PthreadSelf
	(
	)
{
	return pthread_self();
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::PthreadSigMask
//
//	@doc:
//		Set signal mask for thread
//
//---------------------------------------------------------------------------
void
gpos::pthread::PthreadSigMask
	(
	INT mode,
	const SIGSET_T *set,
	SIGSET_T *oldset
	)
{
#ifdef GPOS_DEBUG
	INT res =
#endif // GPOS_DEBUG
	pthread_sigmask(mode, set, oldset);

	GPOS_ASSERT(0 == res);
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::SigEmptySet
//
//	@doc:
//		Initialize signal set to empty
//
//---------------------------------------------------------------------------
void
gpos::pthread::SigEmptySet
	(
	SIGSET_T *set
	)
{
#ifdef GPOS_DEBUG
	INT res =
#endif // GPOS_DEBUG
	sigemptyset(set);

	GPOS_ASSERT(0 == res);
}


//---------------------------------------------------------------------------
//	@function:
//		pthread::SigAddSet
//
//	@doc:
//		Add signal to set
//
//---------------------------------------------------------------------------
void
gpos::pthread::SigAddSet
	(
	SIGSET_T *set,
	INT signum
	)
{
#ifdef GPOS_DEBUG
	INT res =
#endif // GPOS_DEBUG
	sigaddset(set, signum);

	GPOS_ASSERT(0 == res);
}


// EOF

