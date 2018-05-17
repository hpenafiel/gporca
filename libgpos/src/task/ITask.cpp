//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		ITask.cpp
//
//	@doc:
//		 Task abstraction
//---------------------------------------------------------------------------


#include "gpos/task/ITask.h"

using namespace gpos;


//---------------------------------------------------------------------------
//	@function:
//		ITask::TaskSelf
//
//	@doc:
//		Static function to lookup ones own worker in the pool manager
//
//---------------------------------------------------------------------------
ITask *
ITask::TaskSelf()
{
	IWorker *pwrk = IWorker::PwrkrSelf();
	if (NULL != pwrk)
	{
		return pwrk->Ptsk();
	}
	return NULL;
}

// EOF

