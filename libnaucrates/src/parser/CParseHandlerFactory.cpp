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
	IMemoryPool *memory_pool
	)
{
	m_phmPHCreators = GPOS_NEW(memory_pool) HMXMLStrPfPHCreator(memory_pool, HASH_MAP_SIZE);
	
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
	IMemoryPool *memory_pool,
	const XMLCh *xmlszName,
	CParseHandlerManager* parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	GPOS_ASSERT(NULL != m_phmPHCreators);

	PfParseHandlerOpCreator *phoc = m_phmPHCreators->Find(xmlszName);

	if (phoc != NULL)
	{
		return (*phoc) (memory_pool, parse_handler_mgr, parse_handler_root);
	}
	
	CDXLMemoryManager mm(memory_pool);

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
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerDXL(memory_pool, parse_handler_mgr);
}

// creates a parse handler for parsing a Plan
CParseHandlerBase *
CParseHandlerFactory::PphPlan
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerPlan(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing metadata
CParseHandlerBase *
CParseHandlerFactory::PphMetadata
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerMetadata(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a metadata request
CParseHandlerBase *
CParseHandlerFactory::PphMDRequest
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerMDRequest(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing trace flags
CParseHandlerBase *
CParseHandlerFactory::PphTraceFlags
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerTraceFlags(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing optimizer config
CParseHandlerBase *
CParseHandlerFactory::PphOptimizerConfig
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerOptimizerConfig(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing enumerator config
CParseHandlerBase *
CParseHandlerFactory::PphEnumeratorConfig
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerEnumeratorConfig(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing statistics configuration
CParseHandlerBase *
CParseHandlerFactory::PphStatisticsConfig
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerStatisticsConfig(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing CTE configuration
CParseHandlerBase *
CParseHandlerFactory::PphCTEConfig
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerCTEConfig(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing cost model configuration
CParseHandlerBase *
CParseHandlerFactory::PphCostModelConfig
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerCostModel(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing hint configuration
CParseHandlerBase *
CParseHandlerFactory::PphHint
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerHint(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing window oids configuration
CParseHandlerBase *
CParseHandlerFactory::PphWindowOids
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerWindowOids(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing relation metadata
CParseHandlerBase *
CParseHandlerFactory::PphMetadataRelation
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerMDRelation(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing external relation metadata
CParseHandlerBase *
CParseHandlerFactory::PphMetadataRelationExternal
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerMDRelationExternal(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing CTAS relation metadata
CParseHandlerBase *
CParseHandlerFactory::PphMetadataRelationCTAS
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerMDRelationCtas(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a MD index
CParseHandlerBase *
CParseHandlerFactory::PphMDIndex
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerMDIndex(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing relation stats
CParseHandlerBase *
CParseHandlerFactory::PphRelStats
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerRelStats(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing column stats
CParseHandlerBase *
CParseHandlerFactory::PphColStats
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerColStats(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing column stats bucket
CParseHandlerBase *
CParseHandlerFactory::PphColStatsBucket
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerColStatsBucket(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing GPDB type metadata
CParseHandlerBase *
CParseHandlerFactory::PphMDGPDBType
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerMDType(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing GPDB-specific operator metadata
CParseHandlerBase *
CParseHandlerFactory::PphMDGPDBScalarOp
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerMDGPDBScalarOp(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing GPDB-specific function metadata
CParseHandlerBase *
CParseHandlerFactory::PphMDGPDBFunc
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerMDGPDBFunc(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing GPDB-specific aggregate metadata
CParseHandlerBase *
CParseHandlerFactory::PphMDGPDBAgg
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerMDGPDBAgg(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing GPDB-specific trigger metadata
CParseHandlerBase *
CParseHandlerFactory::PphMDGPDBTrigger
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerMDGPDBTrigger(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing GPDB-specific cast metadata
CParseHandlerBase *
CParseHandlerFactory::PphMDCast
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerMDCast(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing GPDB-specific scalar comparison metadata
CParseHandlerBase *
CParseHandlerFactory::PphMDScCmp
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerMDScCmp(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a list of metadata identifiers
CParseHandlerBase *
CParseHandlerFactory::PphMetadataIdList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerMetadataIdList(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a list of column metadata info
CParseHandlerBase *
CParseHandlerFactory::PphMetadataColumns
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerMetadataColumns(memory_pool, parse_handler_mgr, parse_handler_root);
}

CParseHandlerBase *
CParseHandlerFactory::PphMDIndexInfoList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerMDIndexInfoList(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing column info
CParseHandlerBase *
CParseHandlerFactory::PphMetadataColumn
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerMetadataColumn(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a a default value for a column
CParseHandlerBase *
CParseHandlerFactory::PphColumnDefaultValueExpr
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerDefaultValueExpr(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a physical operator
CParseHandlerBase *
CParseHandlerFactory::PphPhysOp
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerPhysicalOp(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a scalar operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarOp
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarOp(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing the properties of a physical operator
CParseHandlerBase *
CParseHandlerFactory::PphProperties
	(
		IMemoryPool *memory_pool,
		CParseHandlerManager *parse_handler_mgr,
		CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerProperties(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a filter operator
CParseHandlerBase *
CParseHandlerFactory::PphFilter
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerFilter(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a table scan
CParseHandlerBase *
CParseHandlerFactory::PphTableScan
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerTableScan(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a bitmap table scan
CParseHandlerBase *
CParseHandlerFactory::PphBitmapTableScan
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerPhysicalBitmapTableScan(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a dynamic bitmap table scan
CParseHandlerBase *
CParseHandlerFactory::PphDynamicBitmapTableScan
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerPhysicalDynamicBitmapTableScan(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing an external scan
CParseHandlerBase *
CParseHandlerFactory::PphExternalScan
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerExternalScan(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a subquery scan
CParseHandlerBase *
CParseHandlerFactory::PphSubqScan
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerSubqueryScan(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a result node
CParseHandlerBase *
CParseHandlerFactory::PphResult
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerResult(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a hash join operator
CParseHandlerBase *
CParseHandlerFactory::PphHashJoin
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerHashJoin(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a nested loop join operator
CParseHandlerBase *
CParseHandlerFactory::PphNLJoin
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerNLJoin(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a merge join operator
CParseHandlerBase *
CParseHandlerFactory::PphMergeJoin
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerMergeJoin(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a sort operator
CParseHandlerBase *
CParseHandlerFactory::PphSort
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerSort(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing an append operator
CParseHandlerBase *
CParseHandlerFactory::PphAppend
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerAppend(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a materialize operator
CParseHandlerBase *
CParseHandlerFactory::PphMaterialize
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerMaterialize(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a dynamic table scan operator
CParseHandlerBase *
CParseHandlerFactory::PphDynamicTableScan
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerDynamicTableScan(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a dynamic index scan operator
CParseHandlerBase *
CParseHandlerFactory::PphDynamicIndexScan
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerDynamicIndexScan(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a partition selector operator
CParseHandlerBase *
CParseHandlerFactory::PphPartitionSelector
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerPartitionSelector(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a sequence operator
CParseHandlerBase *
CParseHandlerFactory::PphSequence
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerSequence(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a Limit operator
CParseHandlerBase *
CParseHandlerFactory::PphLimit
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerLimit(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a Limit Count operator
CParseHandlerBase *
CParseHandlerFactory::PphLimitcount
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarLimitCount(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a scalar subquery operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarSubquery
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarSubquery(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a scalar bitmap boolean operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarBitmapBoolOp
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarBitmapBoolOp(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a scalar array operator.
CParseHandlerBase *
CParseHandlerFactory::PphScalarArray
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerArray(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a scalar arrayref operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarArrayRef
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarArrayRef(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing an arrayref index list
CParseHandlerBase *
CParseHandlerFactory::PphScalarArrayRefIndexList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarArrayRefIndexList(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a scalar assert predicate operator.
CParseHandlerBase *
CParseHandlerFactory::PphScalarAssertConstraintList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarAssertConstraintList(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a scalar DML action operator.
CParseHandlerBase *
CParseHandlerFactory::PphScalarDMLAction
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarDMLAction(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a scalar operator list
CParseHandlerBase *
CParseHandlerFactory::PphScalarOpList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarOpList(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a scalar part OID
CParseHandlerBase *
CParseHandlerFactory::PphScalarPartOid
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarPartOid(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a scalar part default
CParseHandlerBase *
CParseHandlerFactory::PphScalarPartDefault
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarPartDefault(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a scalar part boundary
CParseHandlerBase *
CParseHandlerFactory::PphScalarPartBound
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarPartBound(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a scalar part bound inclusion
CParseHandlerBase *
CParseHandlerFactory::PphScalarPartBoundInclusion
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarPartBoundInclusion(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a scalar part bound openness
CParseHandlerBase *
CParseHandlerFactory::PphScalarPartBoundOpen
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarPartBoundOpen(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a scalar part list values
CParseHandlerBase *
CParseHandlerFactory::PphScalarPartListValues
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarPartListValues(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a scalar part list null test
CParseHandlerBase *
CParseHandlerFactory::PphScalarPartListNullTest
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarPartListNullTest(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing direct dispatch info
CParseHandlerBase *
CParseHandlerFactory::PphDirectDispatchInfo
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerDirectDispatchInfo(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a Limit Count operator
CParseHandlerBase *
CParseHandlerFactory::PphLimitoffset
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarLimitOffset(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a gather motion operator
CParseHandlerBase *
CParseHandlerFactory::PphGatherMotion
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerGatherMotion(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a broadcast motion operator
CParseHandlerBase *
CParseHandlerFactory::PphBroadcastMotion
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerBroadcastMotion(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a redistribute motion operator
CParseHandlerBase *
CParseHandlerFactory::PphRedistributeMotion
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerRedistributeMotion(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a routed motion operator
CParseHandlerBase *
CParseHandlerFactory::PphRoutedMotion
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerRoutedMotion(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a random motion operator
CParseHandlerBase *
CParseHandlerFactory::PphRandomMotion
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerRandomMotion(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a group by operator
CParseHandlerBase *
CParseHandlerFactory::PphAgg
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerAgg(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing aggref operator
CParseHandlerBase *
CParseHandlerFactory::PphAggref
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarAggref(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a grouping cols list in a group by
// operator
CParseHandlerBase *
CParseHandlerFactory::PphGroupingColList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerGroupingColList(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a scalar comparison operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarCmp
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarComp(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a distinct comparison operator
CParseHandlerBase *
CParseHandlerFactory::PphDistinctCmp
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerDistinctComp(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a scalar identifier operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarId
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarIdent(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a scalar FuncExpr
CParseHandlerBase *
CParseHandlerFactory::PphScalarFuncExpr
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarFuncExpr(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a scalar OpExpr
CParseHandlerBase *
CParseHandlerFactory::PphScalarOpexpr
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarOpExpr(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a scalar OpExpr
CParseHandlerBase *
CParseHandlerFactory::PphScalarArrayCmp
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarArrayComp(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a BoolExpr
CParseHandlerBase *
CParseHandlerFactory::PphScalarBoolExpr
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarBoolExpr(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a MinMax
CParseHandlerBase *
CParseHandlerFactory::PphScalarMinMax
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarMinMax(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a BooleanTest
CParseHandlerBase *
CParseHandlerFactory::PphBooleanTest
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarBooleanTest(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a NullTest
CParseHandlerBase *
CParseHandlerFactory::PphScalarNullTest
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarNullTest(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a NullIf
CParseHandlerBase *
CParseHandlerFactory::PphScalarNullIf
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarNullIf(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a cast
CParseHandlerBase *
CParseHandlerFactory::PphScalarCast
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarCast(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a CoerceToDomain operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarCoerceToDomain
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarCoerceToDomain(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a CoerceViaIO operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarCoerceViaIO
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarCoerceViaIO(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing an array coerce expression operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarArrayCoerceExpr
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarArrayCoerceExpr(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a SubPlan.
CParseHandlerBase *
CParseHandlerFactory::PphScalarSubPlan
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarSubPlan(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a SubPlan test expression
CParseHandlerBase *
CParseHandlerFactory::PphScalarSubPlanTestExpr
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarSubPlanTestExpr(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a SubPlan Params DXL node
CParseHandlerBase *
CParseHandlerFactory::PphScalarSubPlanParamList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarSubPlanParamList(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a single SubPlan Param
CParseHandlerBase *
CParseHandlerFactory::PphScalarSubPlanParam
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarSubPlanParam(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a logical TVF
CParseHandlerBase *
CParseHandlerFactory::PphLogicalTVF
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerLogicalTVF(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a physical TVF
CParseHandlerBase *
CParseHandlerFactory::PphPhysicalTVF
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerPhysicalTVF(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a coalesce operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarCoalesce
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarCoalesce(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a Switch operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarSwitch
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarSwitch(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a SwitchCase operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarSwitchCase
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarSwitchCase(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a case test
CParseHandlerBase *
CParseHandlerFactory::PphScalarCaseTest
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarCaseTest(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a Const
CParseHandlerBase *
CParseHandlerFactory::PphScalarConstValue
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarConstValue(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing an if statement
CParseHandlerBase *
CParseHandlerFactory::PphIfStmt
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarIfStmt(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a projection list
CParseHandlerBase *
CParseHandlerFactory::PphProjList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerProjList(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a projection element
CParseHandlerBase *
CParseHandlerFactory::PphProjElem
	(
		IMemoryPool *memory_pool,
		CParseHandlerManager *parse_handler_mgr,
		CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerProjElem(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a hash expr list
CParseHandlerBase *
CParseHandlerFactory::PphHashExprList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,	
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerHashExprList(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a hash expression in a redistribute
// motion node
CParseHandlerBase *
CParseHandlerFactory::PphHashExpr
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerHashExpr(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a condition list in a hash join or
// merge join node
CParseHandlerBase *
CParseHandlerFactory::PphCondList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerCondList(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a sorting column list in a sort node
CParseHandlerBase *
CParseHandlerFactory::PphSortColList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerSortColList(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a sorting column in a sort node
CParseHandlerBase *
CParseHandlerFactory::PphSortCol
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerSortCol(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing the cost estimates of a physical
// operator
CParseHandlerBase *
CParseHandlerFactory::PphCost
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerCost(memory_pool, parse_handler_mgr, parse_handler_root);	
}

// creates a parse handler for parsing a table descriptor
CParseHandlerBase *
CParseHandlerFactory::PphTableDesc
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerTableDescr(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a column descriptor
CParseHandlerBase *
CParseHandlerFactory::PphColDesc				
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerColDescr(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing an index scan node
CParseHandlerBase *
CParseHandlerFactory::PphIndexScan
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerIndexScan(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing an index only scan node
CParseHandlerBase *
CParseHandlerFactory::PphIndexOnlyScan
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerIndexOnlyScan(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a bitmap index scan node
CParseHandlerBase *
CParseHandlerFactory::PphBitmapIndexProbe
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarBitmapIndexProbe(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing an index descriptor of an
// index scan node
CParseHandlerBase *
CParseHandlerFactory::PphIndexDescr
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerIndexDescr(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing the list of index condition in a
// index scan node
CParseHandlerBase *
CParseHandlerFactory::PphIndexCondList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerIndexCondList(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a query
CParseHandlerBase *
CParseHandlerFactory::PphQuery
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerQuery(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a logical operator
CParseHandlerBase *
CParseHandlerFactory::PphLgOp
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerLogicalOp(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a logical get operator
CParseHandlerBase *
CParseHandlerFactory::PphLgGet
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerLogicalGet(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a logical external get operator
CParseHandlerBase *
CParseHandlerFactory::PphLgExternalGet
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerLogicalExternalGet(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a logical project operator
CParseHandlerBase *
CParseHandlerFactory::PphLgProject
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerLogicalProject(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a logical CTE producer operator
CParseHandlerBase *
CParseHandlerFactory::PphLgCTEProducer
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerLogicalCTEProducer(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a logical CTE consumer operator
CParseHandlerBase *
CParseHandlerFactory::PphLgCTEConsumer
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerLogicalCTEConsumer(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a logical CTE anchor operator
CParseHandlerBase *
CParseHandlerFactory::PphLgCTEAnchor
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerLogicalCTEAnchor(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a CTE list
CParseHandlerBase *
CParseHandlerFactory::PphCTEList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerCTEList(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a logical set operator
CParseHandlerBase *
CParseHandlerFactory::PphLgSetOp
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerLogicalSetOp(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a logical select operator
CParseHandlerBase *
CParseHandlerFactory::PphLgSelect
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerLogicalSelect(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a logical join operator
CParseHandlerBase *
CParseHandlerFactory::PphLgJoin
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerLogicalJoin(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing dxl representing query output
CParseHandlerBase *
CParseHandlerFactory::PphQueryOutput
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerQueryOutput(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a logical group by operator
CParseHandlerBase *
CParseHandlerFactory::PphLgGrpBy
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerLogicalGroupBy(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a logical limit operator
CParseHandlerBase *
CParseHandlerFactory::PphLgLimit
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerLogicalLimit(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a logical constant table operator
CParseHandlerBase *
CParseHandlerFactory::PphLgConstTable
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerLogicalConstTable(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing ALL/ANY subquery operators
CParseHandlerBase *
CParseHandlerFactory::PphScSubqueryQuantified
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarSubqueryQuantified(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing an EXISTS/NOT EXISTS subquery operator
CParseHandlerBase *
CParseHandlerFactory::PphScSubqueryExists
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarSubqueryExists(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing relation statistics
CParseHandlerBase *
CParseHandlerFactory::PphStats
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerStatistics(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a pass-through parse handler
CParseHandlerBase *
CParseHandlerFactory::PphStacktrace
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerStacktrace(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing relation statistics
CParseHandlerBase *
CParseHandlerFactory::PphStatsDerivedRelation
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerStatsDerivedRelation(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing derived column statistics
CParseHandlerBase *
CParseHandlerFactory::PphStatsDerivedColumn
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerStatsDerivedColumn(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing bucket bound in a histogram
CParseHandlerBase *
CParseHandlerFactory::PphStatsBucketBound
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerStatsBound(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a window node
CParseHandlerBase *
CParseHandlerFactory::PphWindow
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerPhysicalWindow(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing WindowRef operator
CParseHandlerBase *
CParseHandlerFactory::PphWindowRef
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarWindowRef(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing window frame node
CParseHandlerBase *
CParseHandlerFactory::PphWindowFrame
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerWindowFrame(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing window key node
CParseHandlerBase *
CParseHandlerFactory::PphWindowKey
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerWindowKey(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a list of window keys
CParseHandlerBase *
CParseHandlerFactory::PphWindowKeyList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerWindowKeyList(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing window specification node
CParseHandlerBase *
CParseHandlerFactory::PphWindowSpec
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerWindowSpec(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a list of window specifications
CParseHandlerBase *
CParseHandlerFactory::PphWindowSpecList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerWindowSpecList(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a logical window operator
CParseHandlerBase *
CParseHandlerFactory::PphLgWindow
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerLogicalWindow(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a logical insert operator
CParseHandlerBase *
CParseHandlerFactory::PphLgInsert
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerLogicalInsert(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a logical delete operator
CParseHandlerBase *
CParseHandlerFactory::PphLgDelete
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerLogicalDelete(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a logical update operator
CParseHandlerBase *
CParseHandlerFactory::PphLgUpdate
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerLogicalUpdate(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a logical CTAS operator
CParseHandlerBase *
CParseHandlerFactory::PphLgCTAS
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerLogicalCTAS(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a physical CTAS operator
CParseHandlerBase *
CParseHandlerFactory::PphPhCTAS
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerPhysicalCTAS(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing CTAS storage options
CParseHandlerBase *
CParseHandlerFactory::PphCTASOptions
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerCtasStorageOptions(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a physical CTE producer operator
CParseHandlerBase *
CParseHandlerFactory::PphPhCTEProducer
(
 IMemoryPool *memory_pool,
 CParseHandlerManager *parse_handler_mgr,
 CParseHandlerBase *parse_handler_root
 )
{
	return GPOS_NEW(memory_pool) CParseHandlerPhysicalCTEProducer(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a physical CTE consumer operator
CParseHandlerBase *
CParseHandlerFactory::PphPhCTEConsumer
(
 IMemoryPool *memory_pool,
 CParseHandlerManager *parse_handler_mgr,
 CParseHandlerBase *parse_handler_root
 )
{
	return GPOS_NEW(memory_pool) CParseHandlerPhysicalCTEConsumer(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a physical DML operator
CParseHandlerBase *
CParseHandlerFactory::PphPhDML
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerPhysicalDML(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a physical split operator
CParseHandlerBase *
CParseHandlerFactory::PphPhSplit
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerPhysicalSplit(memory_pool, parse_handler_mgr, parse_handler_root);
}

//	creates a parse handler for parsing a physical row trigger operator
CParseHandlerBase *
CParseHandlerFactory::PphPhRowTrigger
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerPhysicalRowTrigger(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a physical assert operator
CParseHandlerBase *
CParseHandlerFactory::PphPhAssert
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerAssert(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a trailing window frame edge parser
CParseHandlerBase *
CParseHandlerFactory::PphWindowFrameTrailingEdge
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarWindowFrameEdge(memory_pool, parse_handler_mgr, parse_handler_root, false /*fLeading*/);
}

// creates a leading window frame edge parser
CParseHandlerBase *
CParseHandlerFactory::PphWindowFrameLeadingEdge
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarWindowFrameEdge(memory_pool, parse_handler_mgr, parse_handler_root, true /*fLeading*/);
}

// creates a parse handler for parsing search strategy
CParseHandlerBase *
CParseHandlerFactory::PphSearchStrategy
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerSearchStrategy(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing search stage
CParseHandlerBase *
CParseHandlerFactory::PphSearchStage
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerSearchStage(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing xform
CParseHandlerBase *
CParseHandlerFactory::PphXform
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerXform(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates cost params parse handler
CParseHandlerBase *
CParseHandlerFactory::PphCostParams
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerCostParams(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates cost param parse handler
CParseHandlerBase *
CParseHandlerFactory::PphCostParam
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerCostParam(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for top level scalar expressions
CParseHandlerBase *
CParseHandlerFactory::PphScalarExpr
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarExpr(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing GPDB-specific check constraint
CParseHandlerBase *
CParseHandlerFactory::PphMDGPDBCheckConstraint
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerMDGPDBCheckConstraint(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a Values List operator
CParseHandlerBase *
CParseHandlerFactory::PphScalarValuesList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerScalarValuesList(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing a Values Scan operator
CParseHandlerBase *
CParseHandlerFactory::PphValuesScan
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerValuesScan(memory_pool, parse_handler_mgr, parse_handler_root);
}

// creates a parse handler for parsing GPDB-specific array coerce cast metadata
CParseHandlerBase *
CParseHandlerFactory::PphMDArrayCoerceCast
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
{
	return GPOS_NEW(memory_pool) CParseHandlerMDArrayCoerceCast(memory_pool, parse_handler_mgr, parse_handler_root);
}

// EOF
