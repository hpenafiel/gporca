//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010-2011 EMC Corp.
//
//	@filename:
// CParseHandlerFactory.cpp
//
//	@doc:
// Implementation of the factory methods for creating parse handlers
//
//	@owner: 
//
//
//	@test:
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/parsehandlers.h"
#include "naucrates/dxl/xml/CDXLMemoryManager.h"
#include "naucrates/dxl/xml/dxltokens.h"

using namespace gpdxl;

XERCES_CPP_NAMESPACE_USE

CParseHandlerFactory::HMXMLStrPfPHCreator *
CParseHandlerFactory::m_phmPHCreators = NULL;

// adds a new mapping of token to corresponding parse handler
void
CParseHandlerFactory::AddMapping
	(
	Edxltoken edxltok,
	PfParseHandlerOpCreator *pfphopc
	)
{
	GPOS_ASSERT(NULL != m_phmPHCreators);
	const XMLCh *xmlszTok = CDXLTokens::XmlstrToken(edxltok);
	GPOS_ASSERT(NULL != xmlszTok);
	
#ifdef GPOS_DEBUG
	BOOL fInserted = 
#endif
	m_phmPHCreators->Insert(xmlszTok, pfphopc);
	
	GPOS_ASSERT(fInserted);
}

// initialize mapping of tokens to parse handlers
void
CParseHandlerFactory::Init
	(
	IMemoryPool *pmp
	)
{
	m_phmPHCreators = GPOS_NEW(pmp) HMXMLStrPfPHCreator(pmp, ulHashMapSize);
	
	// array mapping XML Token -> Parse Handler Creator mappings to hashmap
	SParseHandlerMapping rgParseHandlers[] =
	{
			{EdxltokenPlan, &PphPlan},
			{EdxltokenMetadata, &PphMetadata},
			{EdxltokenMDRequest, &PphMDRequest},
			{EdxltokenTraceFlags, &PphTraceFlags},
			{EdxltokenOptimizerConfig, &PphOptimizerConfig},
			{EdxltokenRelationExternal, &PphMetadataRelationExternal},
			{EdxltokenRelationCTAS, &PphMetadataRelationCTAS},
			{EdxltokenEnumeratorConfig, &PphEnumeratorConfig},
			{EdxltokenStatisticsConfig, &PphStatisticsConfig},
			{EdxltokenCTEConfig, &PphCTEConfig},
			{EdxltokenCostModelConfig, &PphCostModelConfig},
			{EdxltokenHint, &PphHint},
			{EdxltokenWindowOids, &PphWindowOids},

			{EdxltokenRelation, &PphMetadataRelation},
			{EdxltokenIndex, &PphMDIndex},
			{EdxltokenMDType, &PphMDGPDBType},
			{EdxltokenGPDBScalarOp, &PphMDGPDBScalarOp},
			{EdxltokenGPDBFunc, &PphMDGPDBFunc},
			{EdxltokenGPDBAgg, &PphMDGPDBAgg},
			{EdxltokenGPDBTrigger, &PphMDGPDBTrigger},
			{EdxltokenCheckConstraint, &PphMDGPDBCheckConstraint},
			{EdxltokenRelationStats, &PphRelStats},
			{EdxltokenColumnStats, &PphColStats},
			{EdxltokenMetadataIdList, &PphMetadataIdList},
			{EdxltokenIndexInfoList, &PphMDIndexInfoList},
			{EdxltokenMetadataColumns, &PphMetadataColumns},
			{EdxltokenMetadataColumn, &PphMetadataColumn},
			{EdxltokenColumnDefaultValue, &PphColumnDefaultValueExpr},
			{EdxltokenColumnStatsBucket, &PphColStatsBucket},
			{EdxltokenGPDBCast, &PphMDCast},
			{EdxltokenGPDBMDScCmp, &PphMDScCmp},
			{EdxltokenGPDBArrayCoerceCast, &PphMDArrayCoerceCast},

			{EdxltokenPhysical, &PphPhysOp},

			{EdxltokenPhysicalAggregate, &PphAgg},
			{EdxltokenPhysicalTableScan, &PphTableScan},
			{EdxltokenPhysicalBitmapTableScan, &PphBitmapTableScan},
			{EdxltokenPhysicalDynamicBitmapTableScan, &PphDynamicBitmapTableScan},
			{EdxltokenPhysicalExternalScan, &PphExternalScan},
			{EdxltokenPhysicalHashJoin, &PphHashJoin},
			{EdxltokenPhysicalNLJoin, &PphNLJoin},
			{EdxltokenPhysicalMergeJoin, &PphMergeJoin},
			{EdxltokenPhysicalGatherMotion, &PphGatherMotion},
			{EdxltokenPhysicalBroadcastMotion, &PphBroadcastMotion},
			{EdxltokenPhysicalRedistributeMotion, &PphRedistributeMotion},
			{EdxltokenPhysicalRoutedDistributeMotion, &PphRoutedMotion},
			{EdxltokenPhysicalRandomMotion, &PphRandomMotion},
			{EdxltokenPhysicalSubqueryScan, &PphSubqScan},
			{EdxltokenPhysicalResult, &PphResult},
			{EdxltokenPhysicalLimit, &PphLimit},
			{EdxltokenPhysicalSort, &PphSort},
			{EdxltokenPhysicalAppend, &PphAppend},
			{EdxltokenPhysicalMaterialize, &PphMaterialize},
		 	{EdxltokenPhysicalDynamicTableScan, &PphDynamicTableScan},
		 	{EdxltokenPhysicalDynamicIndexScan, &PphDynamicIndexScan},
		 	{EdxltokenPhysicalPartitionSelector, &PphPartitionSelector},
			{EdxltokenPhysicalSequence, &PphSequence},
			{EdxltokenPhysicalIndexScan, &PphIndexScan},
			{EdxltokenPhysicalIndexOnlyScan, &PphIndexOnlyScan},
			{EdxltokenScalarBitmapIndexProbe, &PphBitmapIndexProbe},
			{EdxltokenIndexDescr, &PphIndexDescr},

			{EdxltokenPhysicalWindow, &PphWindow},
			{EdxltokenScalarWindowref, &PphWindowRef},
			{EdxltokenWindowFrame, &PphWindowFrame},
			{EdxltokenScalarWindowFrameLeadingEdge, &PphWindowFrameLeadingEdge},
			{EdxltokenScalarWindowFrameTrailingEdge, &PphWindowFrameTrailingEdge},
			{EdxltokenWindowKey, &PphWindowKey},
			{EdxltokenWindowKeyList, &PphWindowKeyList},

			{EdxltokenScalarIndexCondList, &PphIndexCondList},
 
			{EdxltokenScalar, &PphScalarOp},

			{EdxltokenScalarFilter, &PphFilter},
			{EdxltokenScalarOneTimeFilter, &PphFilter},
			{EdxltokenScalarRecheckCondFilter, &PphFilter},
			{EdxltokenScalarProjList, &PphProjList},
			{EdxltokenScalarProjElem, &PphProjElem},
			{EdxltokenScalarAggref, &PphAggref},
			{EdxltokenScalarSortColList, &PphSortColList},
			{EdxltokenScalarSortCol, &PphSortCol},
			{EdxltokenScalarCoalesce, &PphScalarCoalesce},
			{EdxltokenScalarComp, &PphScalarCmp},
			{EdxltokenScalarDistinctComp, &PphDistinctCmp},
			{EdxltokenScalarIdent, &PphScalarId},
			{EdxltokenScalarOpExpr, &PphScalarOpexpr},
			{EdxltokenScalarArrayComp, &PphScalarArrayCmp},
			{EdxltokenScalarBoolOr, &PphScalarBoolExpr},
			{EdxltokenScalarBoolNot, &PphScalarBoolExpr},
			{EdxltokenScalarBoolAnd, &PphScalarBoolExpr},
			{EdxltokenScalarMin, &PphScalarMinMax},
			{EdxltokenScalarMax, &PphScalarMinMax},
			{EdxltokenScalarBoolTestIsTrue, &PphBooleanTest},
			{EdxltokenScalarBoolTestIsNotTrue, &PphBooleanTest},
			{EdxltokenScalarBoolTestIsFalse, &PphBooleanTest},
			{EdxltokenScalarBoolTestIsNotFalse, &PphBooleanTest},
			{EdxltokenScalarBoolTestIsUnknown, &PphBooleanTest},
			{EdxltokenScalarBoolTestIsNotUnknown, &PphBooleanTest},
			{EdxltokenScalarSubPlan, &PphScalarSubPlan},
			{EdxltokenScalarConstValue, &PphScalarConstValue},
			{EdxltokenScalarIfStmt, &PphIfStmt},
			{EdxltokenScalarSwitch, &PphScalarSwitch},
			{EdxltokenScalarSwitchCase, &PphScalarSwitchCase},
			{EdxltokenScalarCaseTest, &PphScalarCaseTest},
			{EdxltokenScalarFuncExpr, &PphScalarFuncExpr},
			{EdxltokenScalarIsNull, &PphScalarNullTest},
			{EdxltokenScalarIsNotNull, &PphScalarNullTest},
			{EdxltokenScalarNullIf, &PphScalarNullIf},
			{EdxltokenScalarCast, &PphScalarCast},
			{EdxltokenScalarCoerceToDomain, PphScalarCoerceToDomain},
			{EdxltokenScalarCoerceViaIO, PphScalarCoerceViaIO},
			{EdxltokenScalarArrayCoerceExpr, PphScalarArrayCoerceExpr},
			{EdxltokenScalarHashExpr, &PphHashExpr},
			{EdxltokenScalarHashCondList, &PphCondList},
			{EdxltokenScalarMergeCondList, &PphCondList},
			{EdxltokenScalarHashExprList, &PphHashExprList},
			{EdxltokenScalarGroupingColList, &PphGroupingColList},
			{EdxltokenScalarLimitOffset, &PphLimitoffset},
			{EdxltokenScalarLimitCount, &PphLimitcount},
			{EdxltokenScalarSubPlanTestExpr, &PphScalarSubPlanTestExpr},
			{EdxltokenScalarSubPlanParamList, &PphScalarSubPlanParamList},
			{EdxltokenScalarSubPlanParam, &PphScalarSubPlanParam},
			{EdxltokenScalarOpList, &PphScalarOpList},
			{EdxltokenScalarPartOid, &PphScalarPartOid},
			{EdxltokenScalarPartDefault, &PphScalarPartDefault},
			{EdxltokenScalarPartBound, &PphScalarPartBound},
			{EdxltokenScalarPartBoundInclusion, &PphScalarPartBoundInclusion},
			{EdxltokenScalarPartBoundOpen, &PphScalarPartBoundOpen},
			{EdxltokenScalarPartListValues, &PphScalarPartListValues},
			{EdxltokenScalarPartListNullTest, &PphScalarPartListNullTest},

			{EdxltokenScalarSubquery, &PphScalarSubquery},
			{EdxltokenScalarBitmapAnd, &PphScalarBitmapBoolOp},
			{EdxltokenScalarBitmapOr, &PphScalarBitmapBoolOp},

			{EdxltokenScalarArray, &PphScalarArray},
			{EdxltokenScalarArrayRef, &PphScalarArrayRef},
			{EdxltokenScalarArrayRefIndexList, &PphScalarArrayRefIndexList},
			
			{EdxltokenScalarAssertConstraintList, &PphScalarAssertConstraintList},
			
			{EdxltokenScalarDMLAction, &PphScalarDMLAction},
			{EdxltokenDirectDispatchInfo, &PphDirectDispatchInfo},

			{EdxltokenQueryOutput, &PphQueryOutput},

			{EdxltokenCost, &PphCost},
			{EdxltokenTableDescr, &PphTableDesc},
			{EdxltokenColumns, &PphColDesc},
			{EdxltokenProperties, &PphProperties},
			{EdxltokenPhysicalTVF, &PphPhysicalTVF},
			{EdxltokenLogicalTVF, &PphLogicalTVF},

			{EdxltokenQuery, &PphQuery},
			{EdxltokenLogicalGet, &PphLgGet},
			{EdxltokenLogicalExternalGet, &PphLgExternalGet},
			{EdxltokenLogical, &PphLgOp},
			{EdxltokenLogicalProject, &PphLgProject},
			{EdxltokenLogicalSelect, &PphLgSelect},
			{EdxltokenLogicalJoin, &PphLgJoin},
			{EdxltokenLogicalGrpBy, &PphLgGrpBy},
			{EdxltokenLogicalLimit, &PphLgLimit},
			{EdxltokenLogicalConstTable, &PphLgConstTable},
			{EdxltokenLogicalCTEProducer, &PphLgCTEProducer},
			{EdxltokenLogicalCTEConsumer, &PphLgCTEConsumer},
			{EdxltokenLogicalCTEAnchor, &PphLgCTEAnchor},
			{EdxltokenCTEList, &PphCTEList},

			{EdxltokenLogicalWindow, &PphLgWindow},
			{EdxltokenWindowSpec, &PphWindowSpec},
			{EdxltokenWindowSpecList, &PphWindowSpecList},

			{EdxltokenLogicalInsert, &PphLgInsert},
			{EdxltokenLogicalDelete, &PphLgDelete},
			{EdxltokenLogicalUpdate, &PphLgUpdate},
			{EdxltokenPhysicalDMLInsert, &PphPhDML},
			{EdxltokenPhysicalDMLDelete, &PphPhDML},
			{EdxltokenPhysicalDMLUpdate, &PphPhDML},
			{EdxltokenPhysicalSplit, &PphPhSplit},
			{EdxltokenPhysicalRowTrigger, &PphPhRowTrigger},
			{EdxltokenPhysicalAssert, &PphPhAssert},
			{EdxltokenPhysicalCTEProducer, &PphPhCTEProducer},
			{EdxltokenPhysicalCTEConsumer, &PphPhCTEConsumer},
			{EdxltokenLogicalCTAS, &PphLgCTAS},
			{EdxltokenPhysicalCTAS, &PphPhCTAS},
			{EdxltokenCTASOptions, &PphCTASOptions},
		
			{EdxltokenScalarSubqueryAny, &PphScSubqueryQuantified},
			{EdxltokenScalarSubqueryAll, &PphScSubqueryQuantified},
 			{EdxltokenScalarSubqueryExists, &PphScSubqueryExists},
			{EdxltokenScalarSubqueryNotExists, &PphScSubqueryExists},

			{EdxltokenStackTrace, &PphStacktrace},
			{EdxltokenLogicalUnion, &PphLgSetOp},
			{EdxltokenLogicalUnionAll, &PphLgSetOp},
			{EdxltokenLogicalIntersect, &PphLgSetOp},
			{EdxltokenLogicalIntersectAll, &PphLgSetOp},
			{EdxltokenLogicalDifference, &PphLgSetOp},
			{EdxltokenLogicalDifferenceAll, &PphLgSetOp},

			{EdxltokenStatistics, &PphStats},
			{EdxltokenStatsDerivedColumn, &PphStatsDerivedColumn},
			{EdxltokenStatsDerivedRelation, &PphStatsDerivedRelation},
			{EdxltokenStatsBucketLowerBound, &PphStatsBucketBound},
			{EdxltokenStatsBucketUpperBound, &PphStatsBucketBound},

			{EdxltokenSearchStrategy, &PphSearchStrategy},
			{EdxltokenSearchStage, &PphSearchStage},
			{EdxltokenXform, &PphXform},

			{EdxltokenCostParams, &PphCostParams},
			{EdxltokenCostParam, &PphCostParam},

			{EdxltokenScalarExpr, &PphScalarExpr},
			{EdxltokenScalarValuesList, &PphScalarValuesList},
			{EdxltokenPhysicalValuesScan, &PphValuesScan}

	};
	
	const ULONG ulParsehandlers = GPOS_ARRAY_SIZE(rgParseHandlers);

	for (ULONG ul = 0; ul < ulParsehandlers; ul++)
	{
		SParseHandlerMapping elem = rgParseHandlers[ul];
		AddMapping(elem.edxltoken, elem.pfphopc);
	}
}

// creates a parse handler instance given an xml tag
CParseHandlerBase *
CParseHandlerFactory::Pph
	(
	IMemoryPool *pmp,
	const XMLCh *xmlszName,
	CParseHandlerManager* parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	GPOS_ASSERT(NULL != m_phmPHCreators);

	PfParseHandlerOpCreator *phoc = m_phmPHCreators->Find(xmlszName);

	if (phoc != NULL)
	{
		return (*phoc) (pmp, parse_handler_mgr, pphRoot);
	}
	
	CDXLMemoryManager mm(pmp);

	// did not find the physical operator in the table
	CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(&mm, xmlszName);;

	GPOS_RAISE
	(
	gpdxl::ExmaDXL,
	gpdxl::ExmiDXLUnrecognizedOperator,
	pstr->GetBuffer()
	);

	return NULL;
}

// creates a parse handler for parsing a DXL document.
CParseHandlerDXL *
CParseHandlerFactory::Pphdxl
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr
	)
{
	return GPOS_NEW(pmp) CParseHandlerDXL(pmp, parse_handler_mgr);
}

// creates a parse handler for parsing a Plan
CParseHandlerBase *
CParseHandlerFactory::PphPlan
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerPlan(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing metadata
CParseHandlerBase *
CParseHandlerFactory::PphMetadata
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerMetadata(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a metadata request
CParseHandlerBase *
CParseHandlerFactory::PphMDRequest
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerMDRequest(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing trace flags
CParseHandlerBase *
CParseHandlerFactory::PphTraceFlags
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerTraceFlags(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing optimizer config
CParseHandlerBase *
CParseHandlerFactory::PphOptimizerConfig
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerOptimizerConfig(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing enumerator config
CParseHandlerBase *
CParseHandlerFactory::PphEnumeratorConfig
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerEnumeratorConfig(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing statistics configuration
CParseHandlerBase *
CParseHandlerFactory::PphStatisticsConfig
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerStatisticsConfig(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing CTE configuration
CParseHandlerBase *
CParseHandlerFactory::PphCTEConfig
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerCTEConfig(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing cost model configuration
CParseHandlerBase *
CParseHandlerFactory::PphCostModelConfig
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerCostModel(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing hint configuration
CParseHandlerBase *
CParseHandlerFactory::PphHint
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerHint(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing window oids configuration
CParseHandlerBase *
CParseHandlerFactory::PphWindowOids
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerWindowOids(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing relation metadata
CParseHandlerBase *
CParseHandlerFactory::PphMetadataRelation
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerMDRelation(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing external relation metadata
CParseHandlerBase *
CParseHandlerFactory::PphMetadataRelationExternal
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerMDRelationExternal(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing CTAS relation metadata
CParseHandlerBase *
CParseHandlerFactory::PphMetadataRelationCTAS
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerMDRelationCtas(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a MD index
CParseHandlerBase *
CParseHandlerFactory::PphMDIndex
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerMDIndex(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing relation stats
CParseHandlerBase *
CParseHandlerFactory::PphRelStats
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerRelStats(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing column stats
CParseHandlerBase *
CParseHandlerFactory::PphColStats
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerColStats(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing column stats bucket
CParseHandlerBase *
CParseHandlerFactory::PphColStatsBucket
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerColStatsBucket(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing GPDB type metadata
CParseHandlerBase *
CParseHandlerFactory::PphMDGPDBType
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerMDType(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing GPDB-specific operator metadata
CParseHandlerBase *
CParseHandlerFactory::PphMDGPDBScalarOp
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerMDGPDBScalarOp(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing GPDB-specific function metadata
CParseHandlerBase *
CParseHandlerFactory::PphMDGPDBFunc
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerMDGPDBFunc(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing GPDB-specific aggregate metadata
CParseHandlerBase *
CParseHandlerFactory::PphMDGPDBAgg
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerMDGPDBAgg(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing GPDB-specific trigger metadata
CParseHandlerBase *
CParseHandlerFactory::PphMDGPDBTrigger
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerMDGPDBTrigger(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing GPDB-specific cast metadata
CParseHandlerBase *
CParseHandlerFactory::PphMDCast
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerMDCast(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing GPDB-specific scalar comparison metadata
CParseHandlerBase *
CParseHandlerFactory::PphMDScCmp
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerMDScCmp(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a list of metadata identifiers
CParseHandlerBase *
CParseHandlerFactory::PphMetadataIdList
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerMetadataIdList(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a list of column metadata info
CParseHandlerBase *
CParseHandlerFactory::PphMetadataColumns
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerMetadataColumns(pmp, parse_handler_mgr, pphRoot);
}

CParseHandlerBase *
CParseHandlerFactory::PphMDIndexInfoList
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerMDIndexInfoList(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing column info
CParseHandlerBase *
CParseHandlerFactory::PphMetadataColumn
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerMetadataColumn(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a a default value for a column
CParseHandlerBase *
CParseHandlerFactory::PphColumnDefaultValueExpr
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerDefaultValueExpr(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a physical operator
CParseHandlerBase *
CParseHandlerFactory::PphPhysOp
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerPhysicalOp(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a scalar operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarOp
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarOp(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing the properties of a physical operator
CParseHandlerBase *
CParseHandlerFactory::PphProperties
	(
		IMemoryPool *pmp,
		CParseHandlerManager *parse_handler_mgr,
		CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerProperties(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a filter operator
CParseHandlerBase *
CParseHandlerFactory::PphFilter
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerFilter(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a table scan
CParseHandlerBase *
CParseHandlerFactory::PphTableScan
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerTableScan(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a bitmap table scan
CParseHandlerBase *
CParseHandlerFactory::PphBitmapTableScan
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerPhysicalBitmapTableScan(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a dynamic bitmap table scan
CParseHandlerBase *
CParseHandlerFactory::PphDynamicBitmapTableScan
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerPhysicalDynamicBitmapTableScan(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing an external scan
CParseHandlerBase *
CParseHandlerFactory::PphExternalScan
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerExternalScan(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a subquery scan
CParseHandlerBase *
CParseHandlerFactory::PphSubqScan
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerSubqueryScan(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a result node
CParseHandlerBase *
CParseHandlerFactory::PphResult
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerResult(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a hash join operator
CParseHandlerBase *
CParseHandlerFactory::PphHashJoin
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerHashJoin(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a nested loop join operator
CParseHandlerBase *
CParseHandlerFactory::PphNLJoin
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerNLJoin(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a merge join operator
CParseHandlerBase *
CParseHandlerFactory::PphMergeJoin
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerMergeJoin(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a sort operator
CParseHandlerBase *
CParseHandlerFactory::PphSort
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerSort(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing an append operator
CParseHandlerBase *
CParseHandlerFactory::PphAppend
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerAppend(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a materialize operator
CParseHandlerBase *
CParseHandlerFactory::PphMaterialize
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerMaterialize(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a dynamic table scan operator
CParseHandlerBase *
CParseHandlerFactory::PphDynamicTableScan
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerDynamicTableScan(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a dynamic index scan operator
CParseHandlerBase *
CParseHandlerFactory::PphDynamicIndexScan
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerDynamicIndexScan(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a partition selector operator
CParseHandlerBase *
CParseHandlerFactory::PphPartitionSelector
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerPartitionSelector(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a sequence operator
CParseHandlerBase *
CParseHandlerFactory::PphSequence
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerSequence(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a Limit operator
CParseHandlerBase *
CParseHandlerFactory::PphLimit
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerLimit(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a Limit Count operator
CParseHandlerBase *
CParseHandlerFactory::PphLimitcount
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarLimitCount(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a scalar subquery operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarSubquery
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarSubquery(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a scalar bitmap boolean operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarBitmapBoolOp
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarBitmapBoolOp(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a scalar array operator.
CParseHandlerBase *
CParseHandlerFactory::PphScalarArray
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerArray(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a scalar arrayref operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarArrayRef
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarArrayRef(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing an arrayref index list
CParseHandlerBase *
CParseHandlerFactory::PphScalarArrayRefIndexList
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarArrayRefIndexList(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a scalar assert predicate operator.
CParseHandlerBase *
CParseHandlerFactory::PphScalarAssertConstraintList
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarAssertConstraintList(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a scalar DML action operator.
CParseHandlerBase *
CParseHandlerFactory::PphScalarDMLAction
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarDMLAction(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a scalar operator list
CParseHandlerBase *
CParseHandlerFactory::PphScalarOpList
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarOpList(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a scalar part OID
CParseHandlerBase *
CParseHandlerFactory::PphScalarPartOid
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarPartOid(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a scalar part default
CParseHandlerBase *
CParseHandlerFactory::PphScalarPartDefault
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarPartDefault(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a scalar part boundary
CParseHandlerBase *
CParseHandlerFactory::PphScalarPartBound
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarPartBound(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a scalar part bound inclusion
CParseHandlerBase *
CParseHandlerFactory::PphScalarPartBoundInclusion
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarPartBoundInclusion(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a scalar part bound openness
CParseHandlerBase *
CParseHandlerFactory::PphScalarPartBoundOpen
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarPartBoundOpen(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a scalar part list values
CParseHandlerBase *
CParseHandlerFactory::PphScalarPartListValues
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarPartListValues(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a scalar part list null test
CParseHandlerBase *
CParseHandlerFactory::PphScalarPartListNullTest
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarPartListNullTest(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing direct dispatch info
CParseHandlerBase *
CParseHandlerFactory::PphDirectDispatchInfo
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerDirectDispatchInfo(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a Limit Count operator
CParseHandlerBase *
CParseHandlerFactory::PphLimitoffset
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarLimitOffset(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a gather motion operator
CParseHandlerBase *
CParseHandlerFactory::PphGatherMotion
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerGatherMotion(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a broadcast motion operator
CParseHandlerBase *
CParseHandlerFactory::PphBroadcastMotion
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerBroadcastMotion(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a redistribute motion operator
CParseHandlerBase *
CParseHandlerFactory::PphRedistributeMotion
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerRedistributeMotion(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a routed motion operator
CParseHandlerBase *
CParseHandlerFactory::PphRoutedMotion
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerRoutedMotion(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a random motion operator
CParseHandlerBase *
CParseHandlerFactory::PphRandomMotion
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerRandomMotion(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a group by operator
CParseHandlerBase *
CParseHandlerFactory::PphAgg
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerAgg(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing aggref operator
CParseHandlerBase *
CParseHandlerFactory::PphAggref
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarAggref(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a grouping cols list in a group by
// operator
CParseHandlerBase *
CParseHandlerFactory::PphGroupingColList
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerGroupingColList(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a scalar comparison operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarCmp
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarComp(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a distinct comparison operator
CParseHandlerBase *
CParseHandlerFactory::PphDistinctCmp
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerDistinctComp(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a scalar identifier operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarId
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarIdent(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a scalar FuncExpr
CParseHandlerBase *
CParseHandlerFactory::PphScalarFuncExpr
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarFuncExpr(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a scalar OpExpr
CParseHandlerBase *
CParseHandlerFactory::PphScalarOpexpr
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarOpExpr(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a scalar OpExpr
CParseHandlerBase *
CParseHandlerFactory::PphScalarArrayCmp
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarArrayComp(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a BoolExpr
CParseHandlerBase *
CParseHandlerFactory::PphScalarBoolExpr
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarBoolExpr(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a MinMax
CParseHandlerBase *
CParseHandlerFactory::PphScalarMinMax
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarMinMax(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a BooleanTest
CParseHandlerBase *
CParseHandlerFactory::PphBooleanTest
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarBooleanTest(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a NullTest
CParseHandlerBase *
CParseHandlerFactory::PphScalarNullTest
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarNullTest(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a NullIf
CParseHandlerBase *
CParseHandlerFactory::PphScalarNullIf
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarNullIf(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a cast
CParseHandlerBase *
CParseHandlerFactory::PphScalarCast
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarCast(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a CoerceToDomain operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarCoerceToDomain
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarCoerceToDomain(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a CoerceViaIO operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarCoerceViaIO
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarCoerceViaIO(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing an array coerce expression operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarArrayCoerceExpr
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarArrayCoerceExpr(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a SubPlan.
CParseHandlerBase *
CParseHandlerFactory::PphScalarSubPlan
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarSubPlan(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a SubPlan test expression
CParseHandlerBase *
CParseHandlerFactory::PphScalarSubPlanTestExpr
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarSubPlanTestExpr(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a SubPlan Params DXL node
CParseHandlerBase *
CParseHandlerFactory::PphScalarSubPlanParamList
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarSubPlanParamList(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a single SubPlan Param
CParseHandlerBase *
CParseHandlerFactory::PphScalarSubPlanParam
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarSubPlanParam(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a logical TVF
CParseHandlerBase *
CParseHandlerFactory::PphLogicalTVF
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerLogicalTVF(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a physical TVF
CParseHandlerBase *
CParseHandlerFactory::PphPhysicalTVF
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerPhysicalTVF(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a coalesce operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarCoalesce
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarCoalesce(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a Switch operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarSwitch
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarSwitch(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a SwitchCase operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarSwitchCase
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarSwitchCase(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a case test
CParseHandlerBase *
CParseHandlerFactory::PphScalarCaseTest
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarCaseTest(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a Const
CParseHandlerBase *
CParseHandlerFactory::PphScalarConstValue
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarConstValue(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing an if statement
CParseHandlerBase *
CParseHandlerFactory::PphIfStmt
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarIfStmt(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a projection list
CParseHandlerBase *
CParseHandlerFactory::PphProjList
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerProjList(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a projection element
CParseHandlerBase *
CParseHandlerFactory::PphProjElem
	(
		IMemoryPool *pmp,
		CParseHandlerManager *parse_handler_mgr,
		CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerProjElem(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a hash expr list
CParseHandlerBase *
CParseHandlerFactory::PphHashExprList
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,	
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerHashExprList(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a hash expression in a redistribute
// motion node
CParseHandlerBase *
CParseHandlerFactory::PphHashExpr
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerHashExpr(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a condition list in a hash join or
// merge join node
CParseHandlerBase *
CParseHandlerFactory::PphCondList
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerCondList(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a sorting column list in a sort node
CParseHandlerBase *
CParseHandlerFactory::PphSortColList
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerSortColList(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a sorting column in a sort node
CParseHandlerBase *
CParseHandlerFactory::PphSortCol
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerSortCol(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing the cost estimates of a physical
// operator
CParseHandlerBase *
CParseHandlerFactory::PphCost
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerCost(pmp, parse_handler_mgr, pphRoot);	
}

// creates a parse handler for parsing a table descriptor
CParseHandlerBase *
CParseHandlerFactory::PphTableDesc
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerTableDescr(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a column descriptor
CParseHandlerBase *
CParseHandlerFactory::PphColDesc				
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerColDescr(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing an index scan node
CParseHandlerBase *
CParseHandlerFactory::PphIndexScan
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerIndexScan(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing an index only scan node
CParseHandlerBase *
CParseHandlerFactory::PphIndexOnlyScan
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerIndexOnlyScan(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a bitmap index scan node
CParseHandlerBase *
CParseHandlerFactory::PphBitmapIndexProbe
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarBitmapIndexProbe(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing an index descriptor of an
// index scan node
CParseHandlerBase *
CParseHandlerFactory::PphIndexDescr
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerIndexDescr(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing the list of index condition in a
// index scan node
CParseHandlerBase *
CParseHandlerFactory::PphIndexCondList
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerIndexCondList(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a query
CParseHandlerBase *
CParseHandlerFactory::PphQuery
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerQuery(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a logical operator
CParseHandlerBase *
CParseHandlerFactory::PphLgOp
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerLogicalOp(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a logical get operator
CParseHandlerBase *
CParseHandlerFactory::PphLgGet
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerLogicalGet(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a logical external get operator
CParseHandlerBase *
CParseHandlerFactory::PphLgExternalGet
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerLogicalExternalGet(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a logical project operator
CParseHandlerBase *
CParseHandlerFactory::PphLgProject
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerLogicalProject(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a logical CTE producer operator
CParseHandlerBase *
CParseHandlerFactory::PphLgCTEProducer
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerLogicalCTEProducer(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a logical CTE consumer operator
CParseHandlerBase *
CParseHandlerFactory::PphLgCTEConsumer
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerLogicalCTEConsumer(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a logical CTE anchor operator
CParseHandlerBase *
CParseHandlerFactory::PphLgCTEAnchor
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerLogicalCTEAnchor(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a CTE list
CParseHandlerBase *
CParseHandlerFactory::PphCTEList
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerCTEList(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a logical set operator
CParseHandlerBase *
CParseHandlerFactory::PphLgSetOp
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerLogicalSetOp(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a logical select operator
CParseHandlerBase *
CParseHandlerFactory::PphLgSelect
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerLogicalSelect(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a logical join operator
CParseHandlerBase *
CParseHandlerFactory::PphLgJoin
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerLogicalJoin(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing dxl representing query output
CParseHandlerBase *
CParseHandlerFactory::PphQueryOutput
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerQueryOutput(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a logical group by operator
CParseHandlerBase *
CParseHandlerFactory::PphLgGrpBy
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerLogicalGroupBy(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a logical limit operator
CParseHandlerBase *
CParseHandlerFactory::PphLgLimit
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerLogicalLimit(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a logical constant table operator
CParseHandlerBase *
CParseHandlerFactory::PphLgConstTable
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerLogicalConstTable(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing ALL/ANY subquery operators
CParseHandlerBase *
CParseHandlerFactory::PphScSubqueryQuantified
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarSubqueryQuantified(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing an EXISTS/NOT EXISTS subquery operator
CParseHandlerBase *
CParseHandlerFactory::PphScSubqueryExists
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarSubqueryExists(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing relation statistics
CParseHandlerBase *
CParseHandlerFactory::PphStats
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerStatistics(pmp, parse_handler_mgr, pphRoot);
}

// creates a pass-through parse handler
CParseHandlerBase *
CParseHandlerFactory::PphStacktrace
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerStacktrace(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing relation statistics
CParseHandlerBase *
CParseHandlerFactory::PphStatsDerivedRelation
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerStatsDerivedRelation(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing derived column statistics
CParseHandlerBase *
CParseHandlerFactory::PphStatsDerivedColumn
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerStatsDerivedColumn(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing bucket bound in a histogram
CParseHandlerBase *
CParseHandlerFactory::PphStatsBucketBound
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerStatsBound(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a window node
CParseHandlerBase *
CParseHandlerFactory::PphWindow
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerPhysicalWindow(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing WindowRef operator
CParseHandlerBase *
CParseHandlerFactory::PphWindowRef
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarWindowRef(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing window frame node
CParseHandlerBase *
CParseHandlerFactory::PphWindowFrame
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerWindowFrame(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing window key node
CParseHandlerBase *
CParseHandlerFactory::PphWindowKey
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerWindowKey(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a list of window keys
CParseHandlerBase *
CParseHandlerFactory::PphWindowKeyList
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerWindowKeyList(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing window specification node
CParseHandlerBase *
CParseHandlerFactory::PphWindowSpec
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerWindowSpec(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a list of window specifications
CParseHandlerBase *
CParseHandlerFactory::PphWindowSpecList
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerWindowSpecList(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a logical window operator
CParseHandlerBase *
CParseHandlerFactory::PphLgWindow
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerLogicalWindow(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a logical insert operator
CParseHandlerBase *
CParseHandlerFactory::PphLgInsert
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerLogicalInsert(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a logical delete operator
CParseHandlerBase *
CParseHandlerFactory::PphLgDelete
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerLogicalDelete(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a logical update operator
CParseHandlerBase *
CParseHandlerFactory::PphLgUpdate
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerLogicalUpdate(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a logical CTAS operator
CParseHandlerBase *
CParseHandlerFactory::PphLgCTAS
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerLogicalCTAS(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a physical CTAS operator
CParseHandlerBase *
CParseHandlerFactory::PphPhCTAS
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerPhysicalCTAS(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing CTAS storage options
CParseHandlerBase *
CParseHandlerFactory::PphCTASOptions
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerCtasStorageOptions(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a physical CTE producer operator
CParseHandlerBase *
CParseHandlerFactory::PphPhCTEProducer
(
 IMemoryPool *pmp,
 CParseHandlerManager *parse_handler_mgr,
 CParseHandlerBase *pphRoot
 )
{
	return GPOS_NEW(pmp) CParseHandlerPhysicalCTEProducer(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a physical CTE consumer operator
CParseHandlerBase *
CParseHandlerFactory::PphPhCTEConsumer
(
 IMemoryPool *pmp,
 CParseHandlerManager *parse_handler_mgr,
 CParseHandlerBase *pphRoot
 )
{
	return GPOS_NEW(pmp) CParseHandlerPhysicalCTEConsumer(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a physical DML operator
CParseHandlerBase *
CParseHandlerFactory::PphPhDML
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerPhysicalDML(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a physical split operator
CParseHandlerBase *
CParseHandlerFactory::PphPhSplit
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerPhysicalSplit(pmp, parse_handler_mgr, pphRoot);
}

//	creates a parse handler for parsing a physical row trigger operator
CParseHandlerBase *
CParseHandlerFactory::PphPhRowTrigger
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerPhysicalRowTrigger(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a physical assert operator
CParseHandlerBase *
CParseHandlerFactory::PphPhAssert
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerAssert(pmp, parse_handler_mgr, pphRoot);
}

// creates a trailing window frame edge parser
CParseHandlerBase *
CParseHandlerFactory::PphWindowFrameTrailingEdge
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarWindowFrameEdge(pmp, parse_handler_mgr, pphRoot, false /*fLeading*/);
}

// creates a leading window frame edge parser
CParseHandlerBase *
CParseHandlerFactory::PphWindowFrameLeadingEdge
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarWindowFrameEdge(pmp, parse_handler_mgr, pphRoot, true /*fLeading*/);
}

// creates a parse handler for parsing search strategy
CParseHandlerBase *
CParseHandlerFactory::PphSearchStrategy
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerSearchStrategy(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing search stage
CParseHandlerBase *
CParseHandlerFactory::PphSearchStage
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerSearchStage(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing xform
CParseHandlerBase *
CParseHandlerFactory::PphXform
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerXform(pmp, parse_handler_mgr, pphRoot);
}

// creates cost params parse handler
CParseHandlerBase *
CParseHandlerFactory::PphCostParams
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerCostParams(pmp, parse_handler_mgr, pphRoot);
}

// creates cost param parse handler
CParseHandlerBase *
CParseHandlerFactory::PphCostParam
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerCostParam(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for top level scalar expressions
CParseHandlerBase *
CParseHandlerFactory::PphScalarExpr
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarExpr(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing GPDB-specific check constraint
CParseHandlerBase *
CParseHandlerFactory::PphMDGPDBCheckConstraint
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerMDGPDBCheckConstraint(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a Values List operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarValuesList
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerScalarValuesList(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing a Values Scan operator
CParseHandlerBase *
CParseHandlerFactory::PphValuesScan
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerValuesScan(pmp, parse_handler_mgr, pphRoot);
}

// creates a parse handler for parsing GPDB-specific array coerce cast metadata
CParseHandlerBase *
CParseHandlerFactory::PphMDArrayCoerceCast
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
{
	return GPOS_NEW(pmp) CParseHandlerMDArrayCoerceCast(pmp, parse_handler_mgr, pphRoot);
}

// EOF
