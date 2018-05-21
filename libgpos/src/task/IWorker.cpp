//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2008 Greenplum, Inc.
//
//	@filename:
//		CWorker.cpp
//
//	@doc:
//		Worker abstraction, e.g. thread
//---------------------------------------------------------------------------


#include "gpos/memory/CMemoryPoolManager.h"
#include "gpos/task/CWorkerPoolManager.h"

#include "gpos/task/IWorker.h"

using namespace gpos;

#ifdef GPOS_DEBUG
BOOL IWorker::m_enforce_time_slices(false);
#endif // GPOS_DEBUG

//---------------------------------------------------------------------------
//	@function:
//		IWorker::Self
//
//	@doc:
//		static function to lookup ones own worker in the pool manager
//
//---------------------------------------------------------------------------
IWorker *
IWorker::Self()
{
	IWorker *pwrkr = NULL;
	
	if (NULL != CWorkerPoolManager::WorkerPoolManager())
	{
		pwrkr = CWorkerPoolManager::WorkerPoolManager()->Self();
	}
	
	return pwrkr;
}


//---------------------------------------------------------------------------
//	@function:
//		IWorker::CheckForAbort
//
//	@doc:
//		Check for aborts
//
//---------------------------------------------------------------------------
void
IWorker::CheckAbort
	(
	const CHAR *file,
	ULONG line_num
	)
{
	IWorker *pwrkr = Self();
	if (NULL != pwrkr)
	{
		pwrkr->CheckForAbort(file, line_num);
	}
}

// EOF

