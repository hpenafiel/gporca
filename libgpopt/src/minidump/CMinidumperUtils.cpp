//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CMinidumperUtils.cpp
//
//	@doc:
//		Implementation of minidump utility functions
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpos/common/CAutoTimer.h"
#include "gpos/common/CAutoRef.h"
#include "gpos/common/CBitSet.h"
#include "gpos/common/syslibwrapper.h"
#include "gpos/error/CAutoTrace.h"
#include "gpos/error/CErrorContext.h"
#include "gpos/error/CErrorHandlerStandard.h"
#include "gpos/io/COstreamFile.h"
#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/task/CAutoSuspendAbort.h"
#include "gpos/task/CAutoTraceFlag.h"
#include "gpos/task/CTask.h"
#include "gpos/task/CWorker.h"

#include "naucrates/traceflags/traceflags.h"
#include "naucrates/dxl/parser/CParseHandlerDXL.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/md/CMDProviderMemory.h"

#include "gpopt/base/CAutoOptCtxt.h"
#include "gpopt/cost/ICostModel.h"
#include "gpopt/engine/CEngine.h"
#include "gpopt/engine/CEnumeratorConfig.h"
#include "gpopt/engine/CStatisticsConfig.h"
#include "gpopt/mdcache/CMDAccessor.h"
#include "gpopt/mdcache/CMDCache.h"
#include "gpopt/minidump/CMetadataAccessorFactory.h"
#include "gpopt/minidump/CMinidumperUtils.h"
#include "gpopt/minidump/CMiniDumperDXL.h"
#include "gpopt/optimizer/COptimizer.h"
#include "gpopt/optimizer/COptimizerConfig.h"
#include "gpopt/translate/CTranslatorDXLToExpr.h"

#include <fstream>

using namespace gpos;
using namespace gpopt;
using namespace gpdxl;
using namespace std;

//---------------------------------------------------------------------------
//	@function:
//		CMinidumperUtils::PdxlmdLoad
//
//	@doc:
//		Load minidump file
//
//---------------------------------------------------------------------------
CDXLMinidump *
CMinidumperUtils::PdxlmdLoad
	(
	IMemoryPool *memory_pool,
	const CHAR *szFileName
	)
{
	CAutoTraceFlag atf1(EtraceSimulateAbort, false);
	CAutoTraceFlag atf2(EtraceSimulateOOM, false);

	{
		CAutoTrace at(memory_pool);
		at.Os() << "parsing DXL File " << szFileName;
	}
	
	CParseHandlerDXL *parse_handler_dxl = CDXLUtils::GetParseHandlerForDXLFile(memory_pool, szFileName, NULL /*xsd_file_path*/);

	CBitSet *pbs = parse_handler_dxl->Pbs();
	COptimizerConfig *optimizer_config = parse_handler_dxl->GetOptimizerConfig();
	CDXLNode *pdxlnQuery = parse_handler_dxl->GetQueryDXLRoot();
	DXLNodeArray *query_output_dxlnode_array = parse_handler_dxl->GetOutputColumnsDXLArray();
	DXLNodeArray *cte_dxlnode_array = parse_handler_dxl->GetCTEProducerDXLArray();
	DrgPimdobj *pdrgpmdobj = parse_handler_dxl->GetMdIdCachedObjArray();
	DrgPsysid *pdrgpsysid = parse_handler_dxl->GetSystemIdArray();
	CDXLNode *pdxlnPlan = parse_handler_dxl->PdxlnPlan();
	ULLONG plan_id = parse_handler_dxl->GetPlanId();
	ULLONG plan_space_size = parse_handler_dxl->GetPlanSpaceSize();

	if (NULL != pbs)
	{
		pbs->AddRef();
	}
	
	if (NULL != optimizer_config)
	{
		optimizer_config->AddRef();
	}

	if (NULL != pdxlnQuery)
	{
		pdxlnQuery->AddRef();
	}
	
	if (NULL != query_output_dxlnode_array)
	{
		query_output_dxlnode_array->AddRef();
	}

	if (NULL != cte_dxlnode_array)
	{
		cte_dxlnode_array->AddRef();
	}

	if (NULL != pdrgpmdobj)
	{
		pdrgpmdobj->AddRef();
	}
	
	if (NULL != pdrgpsysid)
	{
		pdrgpsysid->AddRef();
	}

	if (NULL != pdxlnPlan)
	{
		pdxlnPlan->AddRef();
	}
	
	// cleanup
	GPOS_DELETE(parse_handler_dxl);
	
	// reset time slice
#ifdef GPOS_DEBUG
    CWorker::Self()->ResetTimeSlice();
#endif // GPOS_DEBUG

	return GPOS_NEW(memory_pool) CDXLMinidump
				(
				pbs,
				optimizer_config,
				pdxlnQuery,
				query_output_dxlnode_array,
				cte_dxlnode_array,
				pdxlnPlan,
				pdrgpmdobj,
				pdrgpsysid,
				plan_id,
				plan_space_size
				);
}


//---------------------------------------------------------------------------
//	@function:
//		CMinidumperUtils::GenerateMinidumpFileName
//
//	@doc:
//		Generate a timestamp-based minidump filename in the provided buffer.
//
//---------------------------------------------------------------------------
void
CMinidumperUtils::GenerateMinidumpFileName
	(
	CHAR *buf,
	ULONG length,
	ULONG ulSessionId,
	ULONG ulCmdId,
	const CHAR *szMinidumpFileName // name of minidump file to be created,
									// if NULL, a time-based name is generated
	)
{
	if (!gpos::ioutils::PathExists("minidumps"))
	{
		GPOS_TRY
		{
			// create a minidumps folder
			const ULONG ulWrPerms = S_IRUSR  | S_IWUSR  | S_IXUSR;
			gpos::ioutils::CreateDir("minidumps", ulWrPerms);
		}
		GPOS_CATCH_EX(ex)
		{
			std::cerr << "[OPT]: Failed to create minidumps directory";

			// ignore exceptions during the creation of directory
			GPOS_RESET_EX;
		}
		GPOS_CATCH_END;
	}

	if (NULL == szMinidumpFileName)
	{
		// generate a time-based file name
		CUtils::GenerateFileName(buf, "minidumps/Minidump", "mdp", length, ulSessionId, ulCmdId);
	}
	else
	{
		// use provided file name
		const CHAR *szPrefix = "minidumps/";
		const ULONG ulPrefixLength = clib::StrLen(szPrefix);
		clib::StrNCpy(buf, szPrefix, ulPrefixLength);

		// remove directory path before file name, if any
		ULONG ulNameLength = clib::StrLen(szMinidumpFileName);
		ULONG ulNameStart  = ulNameLength - 1;
		while (ulNameStart > 0 &&
				szMinidumpFileName[ulNameStart - 1] != '\\' &&
				szMinidumpFileName[ulNameStart - 1] != '/')
		{
			ulNameStart --;
		}

		ulNameLength = clib::StrLen(szMinidumpFileName + ulNameStart);
		clib::StrNCpy(buf + ulPrefixLength, szMinidumpFileName + ulNameStart, ulNameLength);
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CMinidumperUtils::Finalize
//
//	@doc:
//		Finalize minidump and dump to file
//
//---------------------------------------------------------------------------
void
CMinidumperUtils::Finalize
	(
	CMiniDumperDXL *pmdmp,
	BOOL fSerializeErrCtx
	)
{
	CAutoTraceFlag atf1(EtraceSimulateAbort, false);
	CAutoTraceFlag atf2(EtraceSimulateOOM, false);
	CAutoTraceFlag atf3(EtraceSimulateNetError, false);
	CAutoTraceFlag atf4(EtraceSimulateIOError, false);

	if (fSerializeErrCtx)
	{
		CErrorContext *perrctxt = CTask::Self()->ConvertErrCtxt();
		perrctxt->Serialize();
	}
	
	pmdmp->Finalize();
}

//---------------------------------------------------------------------------
//	@function:
//		CMinidumperUtils::PdxlnExecuteMinidump
//
//	@doc:
//		Load and execute the minidump in the given file
//
//---------------------------------------------------------------------------
CDXLNode * 
CMinidumperUtils::PdxlnExecuteMinidump
	(
	IMemoryPool *memory_pool,
	const CHAR *szFileName,
	ULONG ulSegments,
	ULONG ulSessionId,
	ULONG ulCmdId,
	COptimizerConfig *optimizer_config,
	IConstExprEvaluator *pceeval
	)
{
	GPOS_ASSERT(NULL != szFileName);
	GPOS_ASSERT(NULL != optimizer_config);

	CAutoTimer at("Minidump", true /*fPrint*/);

	// load dump file
	CDXLMinidump *pdxlmd = CMinidumperUtils::PdxlmdLoad(memory_pool, szFileName);
	GPOS_CHECK_ABORT;

	CDXLNode *pdxlnPlan = PdxlnExecuteMinidump
							(
							memory_pool,
							pdxlmd,
							szFileName,
							ulSegments,
							ulSessionId,
							ulCmdId,
							optimizer_config,
							pceeval
							);

	// cleanup
	GPOS_DELETE(pdxlmd);
	
	// reset time slice
#ifdef GPOS_DEBUG
    CWorker::Self()->ResetTimeSlice();
#endif // GPOS_DEBUG
    
	return pdxlnPlan;
}


//---------------------------------------------------------------------------
//	@function:
//		CMinidumperUtils::PdxlnExecuteMinidump
//
//	@doc:
//		Execute the given minidump
//
//---------------------------------------------------------------------------
CDXLNode * 
CMinidumperUtils::PdxlnExecuteMinidump
	(
	IMemoryPool *memory_pool,
	CDXLMinidump *pdxlmd,
	const CHAR *szFileName,
	ULONG ulSegments, 
	ULONG ulSessionId,
	ULONG ulCmdId,
	COptimizerConfig *optimizer_config,
	IConstExprEvaluator *pceeval
	)
{
	GPOS_ASSERT(NULL != szFileName);

	// reset metadata ccache
	CMDCache::Reset();

	CMetadataAccessorFactory factory(memory_pool, pdxlmd, szFileName);

	CDXLNode *result = CMinidumperUtils::PdxlnExecuteMinidump(memory_pool, factory.Pmda(), pdxlmd, szFileName, ulSegments, ulSessionId, ulCmdId, optimizer_config, pceeval);

	return result;
}


//---------------------------------------------------------------------------
//	@function:
//		CMinidumperUtils::PdxlnExecuteMinidump
//
//	@doc:
//		Execute the given minidump using the given MD accessor
//
//---------------------------------------------------------------------------
CDXLNode *
CMinidumperUtils::PdxlnExecuteMinidump
	(
	IMemoryPool *memory_pool,
	CMDAccessor *pmda,
	CDXLMinidump *pdxlmd,
	const CHAR *szFileName,
	ULONG ulSegments,
	ULONG ulSessionId,
	ULONG ulCmdId,
	COptimizerConfig *optimizer_config,
	IConstExprEvaluator *pceeval
	)
{
	GPOS_ASSERT(NULL != pmda);
	GPOS_ASSERT(NULL != pdxlmd->GetQueryDXLRoot() &&
				NULL != pdxlmd->PdrgpdxlnQueryOutput() &&
				NULL != pdxlmd->GetCTEProducerDXLArray() &&
				"No query found in Minidump");
	GPOS_ASSERT(NULL != pdxlmd->GetMdIdCachedObjArray() && NULL != pdxlmd->GetSystemIdArray() && "No metadata found in Minidump");
	GPOS_ASSERT(NULL != optimizer_config);

	CDXLNode *pdxlnPlan = NULL;
	CAutoTimer at("Minidump", true /*fPrint*/);

	GPOS_CHECK_ABORT;

	// set trace flags
	CBitSet *pbsEnabled = NULL;
	CBitSet *pbsDisabled = NULL;
	SetTraceflags(memory_pool, pdxlmd->Pbs(), &pbsEnabled, &pbsDisabled);

	if (NULL == pceeval)
	{
		// disable constant expression evaluation when running minidump since
		// there no executor to compute the scalar expression
		GPOS_UNSET_TRACE(EopttraceEnableConstantExpressionEvaluation);
	}

	CErrorHandlerStandard errhdl;
	GPOS_TRY_HDL(&errhdl)
	{
		pdxlnPlan = COptimizer::PdxlnOptimize
								(
								memory_pool,
								pmda,
								pdxlmd->GetQueryDXLRoot(),
								pdxlmd->PdrgpdxlnQueryOutput(),
								pdxlmd->GetCTEProducerDXLArray(),
								pceeval,
								ulSegments,
								ulSessionId,
								ulCmdId,
								NULL, // search_stage_array
								optimizer_config,
								szFileName
								);
	}
	GPOS_CATCH_EX(ex)
	{
		// reset trace flags
		ResetTraceflags(pbsEnabled, pbsDisabled);

		CRefCount::SafeRelease(pbsEnabled);
		CRefCount::SafeRelease(pbsDisabled);

		GPOS_RETHROW(ex);
	}
	GPOS_CATCH_END;

	// reset trace flags
	ResetTraceflags(pbsEnabled, pbsDisabled);
	
	// clean up
	CRefCount::SafeRelease(pbsEnabled);
	CRefCount::SafeRelease(pbsDisabled);

	GPOS_CHECK_ABORT;

	return pdxlnPlan;
}

// EOF
