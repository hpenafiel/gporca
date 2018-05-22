//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		pthreadwrapper.h
//
//	@doc:
//		Wrapper for functions in pthread library
//
//---------------------------------------------------------------------------

#ifndef GPOS_pthreadwrapper_H
#define GPOS_pthreadwrapper_H

#include "gpos/types.h"
#include "gpos/common/clibtypes.h"
#include "gpos/common/pthreadtypes.h"

namespace gpos
{
	namespace pthread
	{

		// thread entry function declaration
		typedef void *(*PthreadExecFn)(void*);

		// return the specified type is a valid mutex type or not
		BOOL MutexTypeIsValid(INT type);

		// return the specified state is a valid detached state or not
		BOOL IsValidDetachedState(INT state);

		// initialize the thread attributes object
		INT PthreadAttrInit(PTHREAD_ATTR_T *attr);

		// destroy the thread attributes object
		void PthreadAttrDestroy(PTHREAD_ATTR_T *attr);

		// get the detachstate attribute
		INT PthreadAttrGetDetachState(const PTHREAD_ATTR_T *attr, INT *state);

		// set the detachstate attribute
		INT PthreadAttrSetDetachState(PTHREAD_ATTR_T *attr, INT state);

		// initialize the mutex attributes object
		INT PthreadMutexAttrInit(PTHREAD_MUTEXATTR_T *attr);

		// destroy the mutex attributes object
		void PthreadMutexAttrDestroy(PTHREAD_MUTEXATTR_T * attr);

		// get the mutex type attribute
		void PthreadMutexAttrGettype(const PTHREAD_MUTEXATTR_T *attr,INT *type);

		// set the mutex type attribute
		void PthreadMutexAttrSettype(PTHREAD_MUTEXATTR_T *attr, INT type);

		// initialize a mutex
		INT PthreadMutexInit(PTHREAD_MUTEX_T *mutex, const PTHREAD_MUTEXATTR_T *attr);

		// destroy a mutex
		void PthreadMutexDestroy(PTHREAD_MUTEX_T *mutex);

		// lock a mutex
		INT PthreadMutexLock(PTHREAD_MUTEX_T *mutex);

		// try lock a mutex
		INT PthreadMutexTryLock(PTHREAD_MUTEX_T *mutex);

		// unlock a mutex
		INT PthreadMutexUnlock(PTHREAD_MUTEX_T *mutex);

#ifndef GPOS_Darwin
		// lock the mutex object referenced by mutex with timeout
		INT PthreadMutexTimedlock(PTHREAD_MUTEX_T *mutex, const TIMESPEC *timeout);
#endif

		// initialize condition variables
		INT PthreadCondInit(PTHREAD_COND_T *__restrict cond, const PTHREAD_CONDATTR_T *__restrict attr);

		// destroy condition variables
		void PthreadCondDestroy(PTHREAD_COND_T *cond);

		// broadcast a condition
		INT PthreadCondBroadcast(PTHREAD_COND_T *cond);

		// signal a condition
		INT PthreadCondSignal(PTHREAD_COND_T *cond);

		// wait on a condition
		INT PthreadCondTimedWait(PTHREAD_COND_T *__restrict cond, PTHREAD_MUTEX_T *__restrict mutex,
			const TIMESPEC *__restrict abstime);

		// wait on a condition
		INT PthreadCondWait(PTHREAD_COND_T *__restrict cond, PTHREAD_MUTEX_T *__restrict mutex);

		// thread creation
		INT PthreadCreate(PTHREAD_T *__restrict thread, const PTHREAD_ATTR_T *__restrict attr,
			PthreadExecFn func, void *__restrict arg);

		// wait for thread termination
		INT PthreadJoin(PTHREAD_T thread, void **retval);

		// compare thread IDs
		BOOL PthreadEqual(PTHREAD_T pthread1, PTHREAD_T pthread2);

		// get the calling thread ID
		PTHREAD_T PthreadSelf();

		// set signal mask for thread
		void PthreadSigMask(INT mode, const SIGSET_T *set, SIGSET_T *oldset);

		// initialize signal set to empty
		void SigEmptySet(SIGSET_T *set);

		// add signal to set
		void SigAddSet(SIGSET_T *set, INT signum);

	} // namespace pthread
}

#endif

// EOF

