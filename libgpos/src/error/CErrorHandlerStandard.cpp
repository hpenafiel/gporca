//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2008 - 2010 Greenplum, Inc.
//
//	@filename:
//		CErrorHandlerStandardStandard.cpp
//
//	@doc:
//		Implements standard error handler
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpos/error/CErrorContext.h"
#include "gpos/error/CErrorHandlerStandard.h"
#include "gpos/error/CLogger.h"
#include "gpos/string/CWStringStatic.h"
#include "gpos/task/CAutoSuspendAbort.h"
#include "gpos/task/CTask.h"

using namespace gpos;

//---------------------------------------------------------------------------
//	@function:
//		CErrorHandlerStandard::Process
//
//	@doc:
//		Process pending error context;
//
//---------------------------------------------------------------------------
void
CErrorHandlerStandard::Process
	(
	CException exception
	)
{
	CTask *task = CTask::Self();

	GPOS_ASSERT(NULL != task && "No task in current context");

	IErrorContext *err_ctxt = task->ErrCtxt();
	CLogger *log = dynamic_cast<CLogger*>(task->LogErr());
	
	GPOS_ASSERT(err_ctxt->FPending() && "No error to process");
	GPOS_ASSERT(err_ctxt->Exc() == exception &&
			"Exception processed different from pending");

	// print error stack trace
	if (CException::ExmaSystem == exception.UlMajor() && !err_ctxt->FRethrow())
	{
		if ((CException::ExmiIOError == exception.UlMinor() ||
		    CException::ExmiNetError == exception.UlMinor() ) &&
			0 < errno)
		{
			err_ctxt->AppendErrnoMsg();
		}

		if (ILogger::EeilMsgHeaderStack <= log->Eil())
		{
			err_ctxt->AppendStackTrace();
		}
	}

	// scope for suspending cancellation
	{
		// suspend cancellation
		CAutoSuspendAbort asa;

		// log error message
		log->Log(err_ctxt->WszMsg(), err_ctxt->UlSev(), __FILE__, __LINE__);
	}
}

// EOF

