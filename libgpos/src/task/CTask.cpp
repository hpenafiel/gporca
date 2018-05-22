//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2008 - 2010 Greenplum, Inc.
//
//	@filename:
//		CTask.cpp
//
//	@doc:
//		Task implementation
//---------------------------------------------------------------------------

#include "gpos/error/CErrorContext.h"
#include "gpos/error/CErrorHandlerStandard.h"
#include "gpos/task/CAutoSuspendAbort.h"
#include "gpos/task/CTask.h"
#include "gpos/task/CWorker.h"

using namespace gpos;

// init CTaskId's atomic counter
CAtomicULONG_PTR CTaskId::m_counter(0);

const CTaskId CTaskId::m_invalid_tid;

//---------------------------------------------------------------------------
//	@function:
//		CTask::~CTask
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CTask::CTask
	(
	IMemoryPool *pmp,
	CTaskContext *ptskctxt,
	IErrorContext *perrctxt,
	CEvent *pevent,
	volatile BOOL *pfCancel
	)
	:
	m_pmp(pmp),
	m_task_ctxt(ptskctxt),
	m_err_ctxt(perrctxt),
	m_err_handle(NULL),
	m_func(NULL),
	m_arg(NULL),
	m_res(NULL),
	m_mutex(pevent->Mutex()),
	m_event(pevent),
	m_status(EtsInit),
	m_cancel(pfCancel),
	m_cancel_local(false),
	m_abort_suspend_count(false),
	m_reported(false)
{
	GPOS_ASSERT(NULL != pmp);
	GPOS_ASSERT(NULL != ptskctxt);
	GPOS_ASSERT(NULL != perrctxt);

	if (NULL == pfCancel)
	{
		m_cancel = &m_cancel_local;
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CTask::~CTask
//
//	@doc:
//		dtor
//
//---------------------------------------------------------------------------
CTask::~CTask()
{
	GPOS_ASSERT(0 == m_abort_suspend_count);

	// suspend cancellation
	CAutoSuspendAbort asa;

	GPOS_DELETE(m_task_ctxt);
	GPOS_DELETE(m_err_ctxt);

	CMemoryPoolManager::Pmpm()->Destroy(m_pmp);
}


//---------------------------------------------------------------------------
//	@function:
//		CTask::Bind
//
//	@doc:
//		Bind task to function and arguments
//
//---------------------------------------------------------------------------
void 
CTask::Bind
	(
	void *(*pfunc)(void*),
	void *pvArg
	)
{
	GPOS_ASSERT(NULL != pfunc);
	
	m_func = pfunc;
	m_arg = pvArg;
}


//---------------------------------------------------------------------------
//	@function:
//		CTask::Execute
//
//	@doc:
//		Execution of task function; wrapped in asserts to prevent leaks
//
//---------------------------------------------------------------------------
void 
CTask::Execute()
{
	GPOS_ASSERT(EtsDequeued == m_status);

	// final task status
	ETaskStatus ets = m_status;

	// check for cancel
	if (*m_cancel)
	{
		ets = EtsError;
	}
	else
	{
		CErrorHandlerStandard errhdl;
		GPOS_TRY_HDL(&errhdl)
		{
			// mark task as running
			SetStatus(EtsRunning);

			// call executable function
			m_res = m_func(m_arg);

#ifdef GPOS_DEBUG
			// check interval since last CFA
			GPOS_CHECK_ABORT;
#endif // GPOS_DEBUG

			// task completed
			ets = EtsCompleted;
		}
		GPOS_CATCH_EX(ex)
		{
			// not reset error context with error propagation
			ets = EtsError;
		}
		GPOS_CATCH_END;
	}
	
	// signal end of task execution
	Signal(ets);
}


//---------------------------------------------------------------------------
//	@function:
//		CTask::SetStatus
//
//	@doc:
//		Set task status;
//		Locking is required if updating more than one variable;
//
//---------------------------------------------------------------------------
void
CTask::SetStatus(ETaskStatus ets)
{
	// status changes are monotonic
	GPOS_ASSERT(ets >= m_status && "Invalid task status transition");

	m_status = ets;
}


//---------------------------------------------------------------------------
//	@function:
//  	CTask::Scheduled
//
//	@doc:
//		Check if task has been scheduled
//
//---------------------------------------------------------------------------
BOOL
CTask::Scheduled() const
{
	switch (m_status)
	{
		case EtsInit:
			return false;
			break;
		case EtsQueued:
		case EtsDequeued:
		case EtsRunning:
		case EtsCompleted:
		case EtsError:
			return true;
			break;
		default:
			GPOS_ASSERT(!"Invalid task status");
			return false;
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CTask::Finished
//
//	@doc:
//		Check if task finished executing
//
//---------------------------------------------------------------------------
BOOL
CTask::Finished() const
{
	switch (m_status)
	{
		case EtsInit:
		case EtsQueued:
		case EtsDequeued:
		case EtsRunning:
			return false;
			break;
		case EtsCompleted:
		case EtsError:
			return true;
			break;
		default:
			GPOS_ASSERT(!"Invalid task status");
			return false;
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CTask::Signal
//
//	@doc:
//		Signal task completion or error
//
//---------------------------------------------------------------------------
void
CTask::Signal
	(
	ETaskStatus Status
	)
	throw()
{
	GPOS_ASSERT(Scheduled() && !Finished() && "Invalid task status after execution");

	// scope for locking mutex
	{
		// suspend cancellation, or else the mutex may throw an Abort exception
		SuspendAbort();

		// use lock to prevent a waiting worker from missing a signal
		CAutoMutex am(*m_mutex);
		am.Lock();

		// update task status
		SetStatus(Status);

		m_event->Signal();

		// resume cancellation
		ResumeAbort();
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CTask::ResumeAbort
//
//	@doc:
//		Decrement counter for requests to suspend abort
//
//---------------------------------------------------------------------------
void
CTask::ResumeAbort()
{
	GPOS_ASSERT(0 < m_abort_suspend_count);

	m_abort_suspend_count--;

#ifdef GPOS_DEBUG
	CWorker *pwrkr = CWorker::Self();

	GPOS_ASSERT(NULL != pwrkr);
	pwrkr->ResetTimeSlice();
#endif
}


#ifdef GPOS_DEBUG


//---------------------------------------------------------------------------
//	@function:
//		CTask::CheckStatus
//
//	@doc:
//		Check if task has expected status
//
//---------------------------------------------------------------------------
BOOL
CTask::CheckStatus
	(
	BOOL fCompleted
	)
{
	GPOS_ASSERT(!Canceled());
	if (fCompleted)
	{
		// task must have completed without an error
		return (CTask::EtsCompleted == Status());
	}
	else
	{
		// task must still be running
		return (Scheduled() && !Finished());
	}
}

#endif // GPOS_DEBUG

// EOF

