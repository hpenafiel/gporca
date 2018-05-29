//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CAutoOptCtxt.cpp
//
//	@doc:
//		Implementation of auto opt context
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpopt/base/CAutoOptCtxt.h"
#include "gpopt/cost/ICostModel.h"
#include "gpopt/eval/CConstExprEvaluatorDefault.h"
#include "gpopt/optimizer/COptimizerConfig.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CAutoOptCtxt::CAutoOptCtxt
//
//	@doc:
//		ctor
//		Create and install default optimizer context
//
//---------------------------------------------------------------------------
CAutoOptCtxt::CAutoOptCtxt
	(
	IMemoryPool *pmp,
	CMDAccessor *pmda,
	IConstExprEvaluator *pceeval,
	COptimizerConfig *optimizer_config
	)
{
	if (NULL == optimizer_config)
	{
		// create default statistics configuration
		optimizer_config = COptimizerConfig::PoconfDefault(pmp);
	}
	if (NULL == pceeval)
	{
		// use the default constant expression evaluator which cannot evaluate any expression
		pceeval = GPOS_NEW(pmp) CConstExprEvaluatorDefault();
	}

	COptCtxt *poctxt = COptCtxt::PoctxtCreate(pmp, pmda, pceeval, optimizer_config);
	ITask::Self()->GetTls().Store(poctxt);
}

//---------------------------------------------------------------------------
//	@function:
//		CAutoOptCtxt::CAutoOptCtxt
//
//	@doc:
//		ctor
//		Create and install default optimizer context with the given cost model
//
//---------------------------------------------------------------------------
CAutoOptCtxt::CAutoOptCtxt
	(
	IMemoryPool *pmp,
	CMDAccessor *pmda,
	IConstExprEvaluator *pceeval,
	ICostModel *pcm
	)
{
	GPOS_ASSERT(NULL != pcm);
	
	// create default statistics configuration
	COptimizerConfig *optimizer_config = COptimizerConfig::PoconfDefault(pmp, pcm);
	
	if (NULL == pceeval)
	{
		// use the default constant expression evaluator which cannot evaluate any expression
		pceeval = GPOS_NEW(pmp) CConstExprEvaluatorDefault();
	}

	COptCtxt *poctxt = COptCtxt::PoctxtCreate(pmp, pmda, pceeval, optimizer_config);
	ITask::Self()->GetTls().Store(poctxt);
}

//---------------------------------------------------------------------------
//	@function:
//		CAutoOptCtxt::~CAutoOptCtxt
//
//	@doc:
//		dtor
//		uninstall optimizer context and destroy
//
//---------------------------------------------------------------------------
CAutoOptCtxt::~CAutoOptCtxt()
{
	CTaskLocalStorageObject *ptlsobj = ITask::Self()->GetTls().Get(CTaskLocalStorage::EtlsidxOptCtxt);
	ITask::Self()->GetTls().Remove(ptlsobj);
	
	GPOS_DELETE(ptlsobj);
}

// EOF

