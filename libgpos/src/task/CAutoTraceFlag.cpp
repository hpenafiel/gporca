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
	GPOS_ASSERT(NULL != ITask::PtskSelf());
	m_orig = ITask::PtskSelf()->FTrace(m_trace, val);
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
	GPOS_ASSERT(NULL != ITask::PtskSelf());
		
	// reset original value
	ITask::PtskSelf()->FTrace(m_trace, m_orig);
}


// EOF
