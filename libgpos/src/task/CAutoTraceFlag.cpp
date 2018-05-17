//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CAutoTraceFlag.cpp
//
//	@doc:
//		Auto object to toggle TF in scope
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpos/task/CAutoTraceFlag.h"

using namespace gpos;


//---------------------------------------------------------------------------
//	@function:
//		CAutoTraceFlag::CAutoTraceFlag
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CAutoTraceFlag::CAutoTraceFlag
	(
	ULONG trace,
	BOOL val
	)
	:
	m_trace(trace),
	m_orig(false)
{
	GPOS_ASSERT(NULL != ITask::TaskSelf());
	m_orig = ITask::TaskSelf()->Trace(m_trace, val);
}


//---------------------------------------------------------------------------
//	@function:
//		CAutoTraceFlag::~CAutoTraceFlag
//
//	@doc:
//		dtor
//
//---------------------------------------------------------------------------
CAutoTraceFlag::~CAutoTraceFlag()
{
	GPOS_ASSERT(NULL != ITask::TaskSelf());
		
	// reset original value
	ITask::TaskSelf()->Trace(m_trace, m_orig);
}


// EOF
