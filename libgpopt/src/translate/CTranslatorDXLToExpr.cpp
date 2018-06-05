//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CTranslatorDXLToExpr.cpp
//
//	@doc:
//		Implementation of the methods used to translate a DXL tree into Expr tree.
//		All translator methods allocate memory in the provided memory pool, and
//		the caller is responsible for freeing it.
//---------------------------------------------------------------------------

#include "gpos/common/CAutoTimer.h"

#include "naucrates/md/IMDId.h"
#include "naucrates/md/IMDRelation.h"
#include "naucrates/md/IMDFunction.h"
#include "naucrates/md/IMDScalarOp.h"
#include "naucrates/md/IMDAggregate.h"
#include "naucrates/md/IMDCast.h"
#include "naucrates/md/CMDArrayCoerceCastGPDB.h"
#include "naucrates/md/CMDRelationCtasGPDB.h"
#include "naucrates/md/CMDProviderMemory.h"

#include "naucrates/dxl/operators/dxlops.h"

#include "gpopt/base/CAutoOptCtxt.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColumnFactory.h"
#include "gpopt/base/CEnfdOrder.h"
#include "gpopt/base/CEnfdDistribution.h"
#include "gpopt/base/CDistributionSpecAny.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/mdcache/CMDAccessorUtils.h"
#include "gpopt/metadata/CColumnDescriptor.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/ops.h"
#include "gpopt/translate/CTranslatorDXLToExpr.h"
#include "gpopt/translate/CTranslatorExprToDXLUtils.h"
#include "gpopt/exception.h"

#include "naucrates/traceflags/traceflags.h"

#define GPDB_DENSE_RANK_OID 7002
#define GPDB_PERCENT_RANK_OID 7003
#define GPDB_CUME_DIST_OID 7004
#define GPDB_NTILE_INT4_OID 7005
#define GPDB_NTILE_INT8_OID 7006
#define GPDB_NTILE_NUMERIC_OID 7007
#define GPOPT_ACTION_INSERT 0
#define GPOPT_ACTION_DELETE 1

using namespace gpos;
using namespace gpnaucrates;
using namespace gpmd;
using namespace gpdxl;
using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::CTranslatorDXLToExpr
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CTranslatorDXLToExpr::CTranslatorDXLToExpr
	(
	IMemoryPool *memory_pool,
	CMDAccessor *pmda,
	BOOL fInitColumnFactory
	)
	:
	m_memory_pool(memory_pool),
	m_sysid(IMDId::EmdidGPDB, GPMD_GPDB_SYSID),
	m_pmda(pmda),
	m_phmulcr(NULL),
	m_phmululCTE(NULL),
	m_pdrgpulOutputColRefs(NULL),
	m_pdrgpmdname(NULL),
	m_phmulpdxlnCTEProducer(NULL),
	m_ulCTEId(ULONG_MAX),
	m_pcf(NULL)
{
	// initialize hash tables
	m_phmulcr = GPOS_NEW(m_memory_pool) HMUlCr(m_memory_pool);

	// initialize hash tables
	m_phmululCTE = GPOS_NEW(m_memory_pool) HMUlUl(m_memory_pool);

	const ULONG ulSize = GPOS_ARRAY_SIZE(m_rgpfTranslators);
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		m_rgpfTranslators[ul] = NULL;
	}

	InitTranslators();

	if (fInitColumnFactory)
	{
		// get column factory from optimizer context object
		m_pcf = COptCtxt::PoctxtFromTLS()->Pcf();

		GPOS_ASSERT(NULL != m_pcf);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::~CTranslatorDXLToExpr
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CTranslatorDXLToExpr::~CTranslatorDXLToExpr()
{
	m_phmulcr->Release();
	m_phmululCTE->Release();
	CRefCount::SafeRelease(m_pdrgpulOutputColRefs);
	CRefCount::SafeRelease(m_pdrgpmdname);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::InitTranslators
//
//	@doc:
//		Initialize index of scalar translators
//
//---------------------------------------------------------------------------
void
CTranslatorDXLToExpr::InitTranslators()
{
	// array mapping operator type to translator function
	STranslatorMapping rgTranslators[] = 
	{
		{EdxlopLogicalGet,			&gpopt::CTranslatorDXLToExpr::PexprLogicalGet},
		{EdxlopLogicalExternalGet,	&gpopt::CTranslatorDXLToExpr::PexprLogicalGet},
		{EdxlopLogicalTVF,			&gpopt::CTranslatorDXLToExpr::PexprLogicalTVF},
		{EdxlopLogicalSelect, 		&gpopt::CTranslatorDXLToExpr::PexprLogicalSelect},
		{EdxlopLogicalProject, 		&gpopt::CTranslatorDXLToExpr::PexprLogicalProject},
		{EdxlopLogicalCTEAnchor,	&gpopt::CTranslatorDXLToExpr::PexprLogicalCTEAnchor},
		{EdxlopLogicalCTEProducer,	&gpopt::CTranslatorDXLToExpr::PexprLogicalCTEProducer},
		{EdxlopLogicalCTEConsumer,	&gpopt::CTranslatorDXLToExpr::PexprLogicalCTEConsumer},
		{EdxlopLogicalGrpBy, 		&gpopt::CTranslatorDXLToExpr::PexprLogicalGroupBy},
		{EdxlopLogicalLimit, 		&gpopt::CTranslatorDXLToExpr::PexprLogicalLimit},
		{EdxlopLogicalJoin, 		&gpopt::CTranslatorDXLToExpr::PexprLogicalJoin},
		{EdxlopLogicalConstTable, 	&gpopt::CTranslatorDXLToExpr::PexprLogicalConstTableGet},
		{EdxlopLogicalSetOp, 		&gpopt::CTranslatorDXLToExpr::PexprLogicalSetOp},
		{EdxlopLogicalWindow, 		&gpopt::CTranslatorDXLToExpr::PexprLogicalSeqPr},
		{EdxlopLogicalInsert, 		&gpopt::CTranslatorDXLToExpr::PexprLogicalInsert},
		{EdxlopLogicalDelete, 		&gpopt::CTranslatorDXLToExpr::PexprLogicalDelete},
		{EdxlopLogicalUpdate, 		&gpopt::CTranslatorDXLToExpr::PexprLogicalUpdate},
		{EdxlopLogicalCTAS, 		&gpopt::CTranslatorDXLToExpr::PexprLogicalCTAS},
		{EdxlopScalarIdent,			&gpopt::CTranslatorDXLToExpr::PexprScalarIdent},
		{EdxlopScalarCmp, 			&gpopt::CTranslatorDXLToExpr::PexprScalarCmp},
		{EdxlopScalarOpExpr, 		&gpopt::CTranslatorDXLToExpr::PexprScalarOp},
		{EdxlopScalarDistinct, 		&gpopt::CTranslatorDXLToExpr::PexprScalarIsDistinctFrom},
		{EdxlopScalarConstValue, 	&gpopt::CTranslatorDXLToExpr::PexprScalarConst},
		{EdxlopScalarBoolExpr, 		&gpopt::CTranslatorDXLToExpr::PexprScalarBoolOp},
		{EdxlopScalarFuncExpr, 		&gpopt::CTranslatorDXLToExpr::PexprScalarFunc},
		{EdxlopScalarMinMax, 		&gpopt::CTranslatorDXLToExpr::PexprScalarMinMax},
		{EdxlopScalarAggref, 		&gpopt::CTranslatorDXLToExpr::PexprAggFunc},
		{EdxlopScalarWindowRef, 	&gpopt::CTranslatorDXLToExpr::PexprWindowFunc},
		{EdxlopScalarNullTest, 		&gpopt::CTranslatorDXLToExpr::PexprScalarNullTest},
		{EdxlopScalarNullIf, 		&gpopt::CTranslatorDXLToExpr::PexprScalarNullIf},
		{EdxlopScalarBooleanTest, 	&gpopt::CTranslatorDXLToExpr::PexprScalarBooleanTest},
		{EdxlopScalarIfStmt, 		&gpopt::CTranslatorDXLToExpr::PexprScalarIf},
		{EdxlopScalarSwitch, 		&gpopt::CTranslatorDXLToExpr::PexprScalarSwitch},
		{EdxlopScalarSwitchCase,	&gpopt::CTranslatorDXLToExpr::PexprScalarSwitchCase},
		{EdxlopScalarCaseTest,		&gpopt::CTranslatorDXLToExpr::PexprScalarCaseTest},
		{EdxlopScalarCoalesce, 		&gpopt::CTranslatorDXLToExpr::PexprScalarCoalesce},
		{EdxlopScalarArrayCoerceExpr,	&gpopt::CTranslatorDXLToExpr::PexprScalarArrayCoerceExpr},
		{EdxlopScalarCast, 			&gpopt::CTranslatorDXLToExpr::PexprScalarCast},
		{EdxlopScalarCoerceToDomain,&gpopt::CTranslatorDXLToExpr::PexprScalarCoerceToDomain},
		{EdxlopScalarCoerceViaIO,	&gpopt::CTranslatorDXLToExpr::PexprScalarCoerceViaIO},
		{EdxlopScalarSubquery, 		&gpopt::CTranslatorDXLToExpr::PexprScalarSubquery},
		{EdxlopScalarSubqueryAny, 	&gpopt::CTranslatorDXLToExpr::PexprScalarSubqueryQuantified},
		{EdxlopScalarSubqueryAll, 	&gpopt::CTranslatorDXLToExpr::PexprScalarSubqueryQuantified},
		{EdxlopScalarArray, 		&gpopt::CTranslatorDXLToExpr::PexprArray},
		{EdxlopScalarArrayComp, 	&gpopt::CTranslatorDXLToExpr::PexprArrayCmp},
		{EdxlopScalarArrayRef, 		&gpopt::CTranslatorDXLToExpr::PexprArrayRef},
		{EdxlopScalarArrayRefIndexList, &gpopt::CTranslatorDXLToExpr::PexprArrayRefIndexList},
	};

	const ULONG ulTranslators = GPOS_ARRAY_SIZE(rgTranslators);
	
	for (ULONG ul = 0; ul < ulTranslators; ul++)
	{
		STranslatorMapping elem = rgTranslators[ul];
		m_rgpfTranslators[elem.edxlopid] = elem.pf;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PdrgpulOutputColRefs
//
//	@doc:
// 		Return the array of query output column reference id
//
//---------------------------------------------------------------------------
ULongPtrArray *
CTranslatorDXLToExpr::PdrgpulOutputColRefs()
{
	GPOS_ASSERT(NULL != m_pdrgpulOutputColRefs);
	return m_pdrgpulOutputColRefs;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::Pexpr
//
//	@doc:
//		Translate a DXL tree into an Expr Tree
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::Pexpr
	(
	const CDXLNode *pdxln,
	const DXLNodeArray *query_output_dxlnode_array,
	const DXLNodeArray *cte_dxlnode_array
	)
{
	GPOS_ASSERT(NULL == m_pdrgpulOutputColRefs);
	GPOS_ASSERT(NULL == m_phmulpdxlnCTEProducer);
	GPOS_ASSERT(NULL != pdxln && NULL != pdxln->GetOperator());
	GPOS_ASSERT(NULL != query_output_dxlnode_array);
	
	m_phmulpdxlnCTEProducer = GPOS_NEW(m_memory_pool) IdToDXLNodeMap(m_memory_pool);
	const ULONG ulCTEs = cte_dxlnode_array->Size();
	for (ULONG ul = 0; ul < ulCTEs; ul++)
	{
		CDXLNode *pdxlnCTE = (*cte_dxlnode_array)[ul];
		CDXLLogicalCTEProducer *pdxlopCTEProducer = CDXLLogicalCTEProducer::Cast(pdxlnCTE->GetOperator());

		pdxlnCTE->AddRef();
#ifdef GPOS_DEBUG
		BOOL fres =
#endif // GPOS_DEBUG
				m_phmulpdxlnCTEProducer->Insert(GPOS_NEW(m_memory_pool) ULONG(pdxlopCTEProducer->UlId()), pdxlnCTE);
		GPOS_ASSERT(fres);
	}

	// translate main DXL tree
	CExpression *pexpr = Pexpr(pdxln);
	GPOS_ASSERT(NULL != pexpr);

	m_phmulpdxlnCTEProducer->Release();
	m_phmulpdxlnCTEProducer = NULL;

	// generate the array of output column reference ids and column names
	m_pdrgpulOutputColRefs = GPOS_NEW(m_memory_pool) ULongPtrArray(m_memory_pool);
	m_pdrgpmdname = GPOS_NEW(m_memory_pool) DrgPmdname(m_memory_pool);

	BOOL fGenerateRequiredColumns = COperator::EopLogicalUpdate != pexpr->Pop()->Eopid();
	
	const ULONG ulLen = query_output_dxlnode_array->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		CDXLNode *pdxlnIdent = (*query_output_dxlnode_array)[ul];

		// get dxl scalar identifier
		CDXLScalarIdent *pdxlopIdent = CDXLScalarIdent::Cast(pdxlnIdent->GetOperator());

		// get the dxl column reference
		const CDXLColRef *pdxlcr = pdxlopIdent->Pdxlcr();
		GPOS_ASSERT(NULL != pdxlcr);
		const ULONG col_id = pdxlcr->Id();

		// get its column reference from the hash map
		const CColRef *pcr = PcrLookup(m_phmulcr, col_id);
		
		if (fGenerateRequiredColumns)
		{
			const ULONG ulColRefId =  pcr->UlId();
			ULONG *pulCopy = GPOS_NEW(m_memory_pool) ULONG(ulColRefId);
			// add to the array of output column reference ids
			m_pdrgpulOutputColRefs->Append(pulCopy);
	
			// get the column names and add it to the array of output column names
			CMDName *mdname = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pdxlcr->MdName()->Pstr());
			m_pdrgpmdname->Append(mdname);
		}
	}

	return pexpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::Pexpr
//
//	@doc:
//		Translates a DXL tree into a Expr Tree
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::Pexpr
	(
	const CDXLNode *pdxln
	)
{
	// recursive function - check stack
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(NULL != pdxln && NULL != pdxln->GetOperator());

	CDXLOperator *dxl_op = pdxln->GetOperator();

	CExpression *pexpr = NULL;
	switch (dxl_op->GetDXLOperatorType())
	{
		case EdxloptypeLogical:
				pexpr = PexprLogical(pdxln);
				break;

		case EdxloptypeScalar:
				pexpr = PexprScalar(pdxln);
				break;

		default:
			GPOS_RAISE(gpopt::ExmaGPOPT, gpopt::ExmiUnsupportedOp, pdxln->GetOperator()->GetOpNameStr()->GetBuffer());
	}

	return pexpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprTranslateQuery
//
//	@doc:
// 		 Main driver for translating dxl query with its associated output
//		columns and CTEs
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprTranslateQuery
	(
	const CDXLNode *pdxln,
	const DXLNodeArray *query_output_dxlnode_array,
	const DXLNodeArray *cte_dxlnode_array
	)
{
	CAutoTimer at("\n[OPT]: DXL To Expr Translation Time", GPOS_FTRACE(EopttracePrintOptimizationStatistics));

	return Pexpr(pdxln, query_output_dxlnode_array, cte_dxlnode_array);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprTranslateScalar
//
//	@doc:
// 		 Translate a dxl scalar expression
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprTranslateScalar
	(
	const CDXLNode *pdxln,
	DrgPcr *pdrgpcr,
	ULongPtrArray *pdrgpul
	)
{
	GPOS_ASSERT_IMP(NULL != pdrgpul, NULL != pdrgpcr);
	GPOS_ASSERT_IMP(NULL != pdrgpul, pdrgpul->Size() == pdrgpcr->Size());

	if (EdxloptypeScalar != pdxln->GetOperator()->GetDXLOperatorType())
	{
		GPOS_RAISE(gpopt::ExmaGPOPT, gpopt::ExmiUnexpectedOp, pdxln->GetOperator()->GetOpNameStr()->GetBuffer());
	}
	
	if (NULL != pdrgpcr)
	{
		const ULONG ulLen = pdrgpcr->Size();
		for (ULONG ul = 0; ul < ulLen; ul++)
		{
			CColRef *pcr = (*pdrgpcr)[ul];
			// copy key
			ULONG *pulKey = NULL;
			if (NULL == pdrgpul)
			{
				pulKey = GPOS_NEW(m_memory_pool) ULONG(ul+1);
			}
			else
			{
				pulKey = GPOS_NEW(m_memory_pool) ULONG(*((*pdrgpul)[ul]) + 1);
			}
	#ifdef GPOS_DEBUG
			BOOL fres =
	#endif // GPOS_DEBUG
					m_phmulcr->Insert(pulKey, pcr);
			GPOS_ASSERT(fres);
		}
	}

	return PexprScalar(pdxln);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprLogical
//
//	@doc:
//		Translates a DXL Logical Op into a Expr
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprLogical
	(
	const CDXLNode *pdxln
	)
{
	// recursive function - check stack
	GPOS_CHECK_STACK_SIZE;

	GPOS_ASSERT(NULL != pdxln);
	GPOS_ASSERT(EdxloptypeLogical == pdxln->GetOperator()->GetDXLOperatorType());
	CDXLOperator *dxl_op = pdxln->GetOperator();

	ULONG ulOpId =  (ULONG) dxl_op->GetDXLOperator();
	PfPexpr pf = m_rgpfTranslators[ulOpId];

	if (NULL == pf)
	{
		GPOS_RAISE(gpopt::ExmaGPOPT, gpopt::ExmiUnsupportedOp, dxl_op->GetOpNameStr()->GetBuffer());
	}
	
	return (this->* pf)(pdxln);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprLogicalTVF
//
//	@doc:
// 		Create a logical TVF expression from its DXL representation
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprLogicalTVF
	(
	const CDXLNode *pdxln
	)
{
	CDXLLogicalTVF *dxl_op = CDXLLogicalTVF::Cast(pdxln->GetOperator());
	GPOS_ASSERT(NULL != dxl_op->MdName()->Pstr());

	// populate column information
	const ULONG ulColumns = dxl_op->Arity();
	GPOS_ASSERT(0 < ulColumns);

	DrgPcoldesc *pdrgpcoldesc = GPOS_NEW(m_memory_pool) DrgPcoldesc(m_memory_pool);

	for (ULONG ul = 0; ul < ulColumns; ul++)
	{
		const CDXLColDescr *pdxlcoldesc = dxl_op->GetColumnDescrAt(ul);
		GPOS_ASSERT(pdxlcoldesc->MDIdType()->IsValid());

		const IMDType *pmdtype = m_pmda->Pmdtype(pdxlcoldesc->MDIdType());

		GPOS_ASSERT(NULL != pdxlcoldesc->MdName()->Pstr()->GetBuffer());
		CWStringConst strColName(m_memory_pool, pdxlcoldesc->MdName()->Pstr()->GetBuffer());

		INT iAttNo = pdxlcoldesc->AttrNum();
		CColumnDescriptor *pcoldesc = GPOS_NEW(m_memory_pool) CColumnDescriptor
													(
													m_memory_pool,
													pmdtype,
													pdxlcoldesc->TypeModifier(),
													CName(m_memory_pool, &strColName),
													iAttNo,
													true, // fNullable
													pdxlcoldesc->Width()
													);
		pdrgpcoldesc->Append(pcoldesc);
	}

	// create a logical TVF operator
	IMDId *pmdidFunc = dxl_op->FuncMdId();
	pmdidFunc->AddRef();

	IMDId *pmdidRetType = dxl_op->ReturnTypeMdId();
	pmdidRetType->AddRef();
	CLogicalTVF *popTVF = GPOS_NEW(m_memory_pool) CLogicalTVF
										(
										m_memory_pool,
										pmdidFunc,
										pmdidRetType,
										GPOS_NEW(m_memory_pool) CWStringConst(m_memory_pool, dxl_op->MdName()->Pstr()->GetBuffer()),
										pdrgpcoldesc
										);

	// create expression containing the logical TVF operator
	CExpression *pexpr = NULL;
	const ULONG ulArity = pdxln->Arity();
	if (0 < ulArity)
	{
		// translate function arguments
		DrgPexpr *pdrgpexprArgs = PdrgpexprChildren(pdxln);

		pexpr = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popTVF, pdrgpexprArgs);
	}
	else
	{
		// function has no arguments
		pexpr = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popTVF);
	}

	// construct the mapping between the DXL ColId and CColRef
	ConstructDXLColId2ColRefMapping(dxl_op->GetColumnDescrDXLArray(), popTVF->PdrgpcrOutput());

	return pexpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprLogicalGet
//
//	@doc:
// 		Create a Expr logical get from a DXL logical get
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprLogicalGet
	(
	const CDXLNode *pdxln
	)
{
	CDXLOperator *dxl_op = pdxln->GetOperator();
	Edxlopid edxlopid = dxl_op->GetDXLOperator();

	// translate the table descriptor
	CDXLTableDescr *table_descr = CDXLLogicalGet::Cast(dxl_op)->GetTableDescr();

	GPOS_ASSERT(NULL != table_descr);
	GPOS_ASSERT(NULL != table_descr->MdName()->Pstr());

	CTableDescriptor *ptabdesc = Ptabdesc(table_descr);

	CWStringConst strAlias(m_memory_pool, table_descr->MdName()->Pstr()->GetBuffer());

	// create a logical get or dynamic get operator
	CName *pname = GPOS_NEW(m_memory_pool) CName(m_memory_pool, CName(&strAlias));
	CLogical *popGet = NULL;
	DrgPcr *pdrgpcr = NULL; 

	const IMDRelation *pmdrel = m_pmda->Pmdrel(table_descr->MDId());
	if (pmdrel->FPartitioned())
	{
		GPOS_ASSERT(EdxlopLogicalGet == edxlopid);

		// generate a part index id
		ULONG ulPartIndexId = COptCtxt::PoctxtFromTLS()->UlPartIndexNextVal();
		popGet = GPOS_NEW(m_memory_pool) CLogicalDynamicGet(m_memory_pool, pname, ptabdesc, ulPartIndexId);	
		CLogicalDynamicGet *popDynamicGet = CLogicalDynamicGet::PopConvert(popGet);

		// get the output column references from the dynamic get
		pdrgpcr = popDynamicGet->PdrgpcrOutput();

		// if there are no indices, we only generate a dummy partition constraint because
		// the constraint might be expensive to compute and it is not needed
		BOOL fDummyConstraint = 0 == pmdrel->UlIndices();
		CPartConstraint *ppartcnstr = CUtils::PpartcnstrFromMDPartCnstr
												(
												m_memory_pool,
												m_pmda,
												popDynamicGet->PdrgpdrgpcrPart(),
												pmdrel->Pmdpartcnstr(),
												pdrgpcr,
												fDummyConstraint
												);
		popDynamicGet->SetPartConstraint(ppartcnstr);
	}
	else
	{
		if (EdxlopLogicalGet == edxlopid)
		{
			popGet = GPOS_NEW(m_memory_pool) CLogicalGet(m_memory_pool, pname, ptabdesc);
		}
		else
		{
			GPOS_ASSERT(EdxlopLogicalExternalGet == edxlopid);
			popGet = GPOS_NEW(m_memory_pool) CLogicalExternalGet(m_memory_pool, pname, ptabdesc);
		}

		// get the output column references
		pdrgpcr = CLogicalGet::PopConvert(popGet)->PdrgpcrOutput();
	}

	CExpression *pexpr = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popGet);

	GPOS_ASSERT(NULL != pdrgpcr);
	GPOS_ASSERT(pdrgpcr->Size() == table_descr->Arity());

	const ULONG ulColumns = pdrgpcr->Size();
	// construct the mapping between the DXL ColId and CColRef
	for(ULONG ul = 0; ul < ulColumns ; ul++)
	{
		CColRef *pcr = (*pdrgpcr)[ul];
		const CDXLColDescr *pdxlcd = table_descr->GetColumnDescrAt(ul);
		GPOS_ASSERT(NULL != pcr);
		GPOS_ASSERT(NULL != pdxlcd && !pdxlcd-> IsDropped());

		// copy key
		ULONG *pulKey = GPOS_NEW(m_memory_pool) ULONG(pdxlcd->Id());
		BOOL fres = m_phmulcr->Insert(pulKey, pcr);

		if (!fres)
		{
			GPOS_DELETE(pulKey);
		}
	}

	return pexpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprLogicalSetOp
//
//	@doc:
// 		Create a logical set operator from a DXL set operator
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprLogicalSetOp
	(
	const CDXLNode *pdxln
	)
{
	GPOS_ASSERT(NULL != pdxln);

	CDXLLogicalSetOp *dxl_op = CDXLLogicalSetOp::Cast(pdxln->GetOperator());

#ifdef GPOS_DEBUG
	const ULONG ulArity = pdxln->Arity();
#endif // GPOS_DEBUG

	GPOS_ASSERT(2 <= ulArity);
	GPOS_ASSERT(ulArity == dxl_op->ChildCount());

	// array of input column reference
	DrgDrgPcr *pdrgdrgpcrInput = GPOS_NEW(m_memory_pool) DrgDrgPcr(m_memory_pool);
		// array of output column descriptors
	ULongPtrArray *pdrgpulOutput = GPOS_NEW(m_memory_pool) ULongPtrArray(m_memory_pool);
	
	DrgPexpr *pdrgpexpr = PdrgpexprPreprocessSetOpInputs(pdxln, pdrgdrgpcrInput, pdrgpulOutput);

	// create an array of output column references
	DrgPcr *pdrgpcrOutput = CTranslatorDXLToExprUtils::Pdrgpcr(m_memory_pool, m_phmulcr, pdrgpulOutput /*array of colids of the first child*/);

	pdrgpulOutput->Release();

	CLogicalSetOp *pop = NULL;
	switch (dxl_op->GetSetOpType())
	{
		case EdxlsetopUnion:
				{
					pop = GPOS_NEW(m_memory_pool) CLogicalUnion(m_memory_pool, pdrgpcrOutput, pdrgdrgpcrInput);
					break;
				}

		case EdxlsetopUnionAll:
				{
					pop = GPOS_NEW(m_memory_pool) CLogicalUnionAll(m_memory_pool, pdrgpcrOutput, pdrgdrgpcrInput);
					break;
				}

		case EdxlsetopDifference:
				{
					pop = GPOS_NEW(m_memory_pool) CLogicalDifference(m_memory_pool, pdrgpcrOutput, pdrgdrgpcrInput);
					break;
				}

		case EdxlsetopIntersect:
				{
					pop = GPOS_NEW(m_memory_pool) CLogicalIntersect(m_memory_pool, pdrgpcrOutput, pdrgdrgpcrInput);
					break;
				}

		case EdxlsetopDifferenceAll:
				{
					pop = GPOS_NEW(m_memory_pool) CLogicalDifferenceAll(m_memory_pool, pdrgpcrOutput, pdrgdrgpcrInput);
					break;
				}

		case EdxlsetopIntersectAll:
				{
					pop = GPOS_NEW(m_memory_pool) CLogicalIntersectAll(m_memory_pool, pdrgpcrOutput, pdrgdrgpcrInput);
					break;
				}

		default:
			GPOS_RAISE(gpopt::ExmaGPOPT, gpopt::ExmiUnsupportedOp, dxl_op->GetOpNameStr()->GetBuffer());
	}

	GPOS_ASSERT(NULL != pop);

	return GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, pop, pdrgpexpr);
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprCastPrjElem
//
//	@doc:
//		Return a project element on a cast expression
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprCastPrjElem
	(
	IMDId *pmdidSource,
	IMDId *pmdidDest,
	const CColRef *pcrToCast,
	CColRef *pcrToReturn
	)
{
	const IMDCast *pmdcast = m_pmda->Pmdcast(pmdidSource, pmdidDest);
	pmdidDest->AddRef();
	pmdcast->PmdidCastFunc()->AddRef();
	CExpression *pexprCast;

	if (pmdcast->EmdPathType() == IMDCast::EmdtArrayCoerce)
	{
		CMDArrayCoerceCastGPDB *parrayCoerceCast = (CMDArrayCoerceCastGPDB *) pmdcast;
		pexprCast =
		GPOS_NEW(m_memory_pool) CExpression
		(
			m_memory_pool,
			GPOS_NEW(m_memory_pool) CScalarArrayCoerceExpr
							(
							m_memory_pool,
							parrayCoerceCast->PmdidCastFunc(),
							pmdidDest,
							parrayCoerceCast->TypeModifier(),
							parrayCoerceCast->FIsExplicit(),
							(COperator::ECoercionForm) parrayCoerceCast->Ecf(),
							parrayCoerceCast->ILoc()
							),
			GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, GPOS_NEW(m_memory_pool) CScalarIdent(m_memory_pool, pcrToCast))
		);
	}
	else
	{
		pexprCast =
			GPOS_NEW(m_memory_pool) CExpression
			(
				m_memory_pool,
				GPOS_NEW(m_memory_pool) CScalarCast(m_memory_pool, pmdidDest, pmdcast->PmdidCastFunc(), pmdcast->FBinaryCoercible()),
				GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, GPOS_NEW(m_memory_pool) CScalarIdent(m_memory_pool, pcrToCast))
			);
	}

	return
		GPOS_NEW(m_memory_pool) CExpression
				(
				m_memory_pool,
				GPOS_NEW(m_memory_pool) CScalarProjectElement(m_memory_pool, pcrToReturn),
				pexprCast
				);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::BuildSetOpChild
//
//	@doc:
//		Build expression and input columns of SetOp Child
//
//---------------------------------------------------------------------------
void
CTranslatorDXLToExpr::BuildSetOpChild
	(
	const CDXLNode *pdxlnSetOp,
	ULONG ulChildIndex,
	CExpression **ppexprChild, // output: generated child expression
	DrgPcr **ppdrgpcrChild, // output: generated child input columns
	DrgPexpr **ppdrgpexprChildProjElems // output: project elements to remap child input columns
	)
{
	GPOS_ASSERT(NULL != pdxlnSetOp);
	GPOS_ASSERT(NULL != ppexprChild);
	GPOS_ASSERT(NULL != ppdrgpcrChild);
	GPOS_ASSERT(NULL != ppdrgpexprChildProjElems);
	GPOS_ASSERT(NULL == *ppdrgpexprChildProjElems);

	const CDXLLogicalSetOp *dxl_op = CDXLLogicalSetOp::Cast(pdxlnSetOp->GetOperator());
	const CDXLNode *child_dxlnode = (*pdxlnSetOp)[ulChildIndex];

	// array of project elements to remap child input columns
	*ppdrgpexprChildProjElems = GPOS_NEW(m_memory_pool) DrgPexpr(m_memory_pool);

	// array of child input column
	*ppdrgpcrChild = GPOS_NEW(m_memory_pool) DrgPcr(m_memory_pool);

	// translate child
	*ppexprChild = PexprLogical(child_dxlnode);

	const ULongPtrArray *pdrgpulInput = dxl_op->GetInputColIdArrayAt(ulChildIndex);
	const ULONG ulInputCols = pdrgpulInput->Size();
	CColRefSet *pcrsChildOutput = CDrvdPropRelational::Pdprel((*ppexprChild)->PdpDerive())->PcrsOutput();
	for (ULONG ulColPos = 0; ulColPos < ulInputCols; ulColPos++)
	{
		// column identifier of the input column
		ULONG col_id = *(*pdrgpulInput)[ulColPos];
		const CColRef *pcr = PcrLookup(m_phmulcr, col_id);

		// corresponding output column descriptor
		const CDXLColDescr *pdxlcdOutput = dxl_op->GetColumnDescrAt(ulColPos);

		// check if a cast function needs to be introduced
		IMDId *pmdidSource = pcr->Pmdtype()->MDId();
		IMDId *pmdidDest = pdxlcdOutput->MDIdType();

		if (FCastingUnknownType(pmdidSource, pmdidDest))
		{
			GPOS_RAISE(gpopt::ExmaGPOPT, gpopt::ExmiUnsupportedOp, GPOS_WSZ_LIT("Casting of columns of unknown data type"));
		}

		const IMDType *pmdtype = m_pmda->Pmdtype(pmdidDest);
		INT type_modifier = pdxlcdOutput->TypeModifier();

		BOOL fEqualTypes = IMDId::FEqualMDId(pmdidSource, pmdidDest);
		BOOL fFirstChild = (0 == ulChildIndex);
		BOOL fUnionOrUnionAll = ((EdxlsetopUnionAll == dxl_op->GetSetOpType()) || (EdxlsetopUnion == dxl_op->GetSetOpType()));

		if (!pcrsChildOutput->FMember(pcr))
		{
			// input column is an outer reference, add a project element for input column

			// add the colref to the hash map between DXL ColId and colref as they can used above the setop
			CColRef *pcrNew = PcrCreate(pcr, pmdtype, type_modifier, fFirstChild, pdxlcdOutput->Id());
			(*ppdrgpcrChild)->Append(pcrNew);

			CExpression *pexprChildProjElem = NULL;
			if (fEqualTypes)
			{
				// project child input column
				pexprChildProjElem =
						GPOS_NEW(m_memory_pool) CExpression
						(
						m_memory_pool,
						GPOS_NEW(m_memory_pool) CScalarProjectElement(m_memory_pool, pcrNew),
						GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, GPOS_NEW(m_memory_pool) CScalarIdent(m_memory_pool, pcr))
						);
			}
			else
			{
				// introduce cast expression
				pexprChildProjElem = PexprCastPrjElem(pmdidSource, pmdidDest, pcr, pcrNew);
			}

			(*ppdrgpexprChildProjElems)->Append(pexprChildProjElem);
			continue;
		}

		if (fEqualTypes)
		{
			// no cast function needed, add the colref to the array of input colrefs
			(*ppdrgpcrChild)->Append(const_cast<CColRef*>(pcr));
			continue;
		}

		if (fUnionOrUnionAll || fFirstChild)
		{
			// add the colref to the hash map between DXL ColId and colref as they can used above the setop
			CColRef *pcrNew = PcrCreate(pcr, pmdtype, type_modifier, fFirstChild, pdxlcdOutput->Id());
			(*ppdrgpcrChild)->Append(pcrNew);

			// introduce cast expression for input column
			CExpression *pexprChildProjElem = PexprCastPrjElem(pmdidSource, pmdidDest, pcr, pcrNew);
			(*ppdrgpexprChildProjElems)->Append(pexprChildProjElem);
		}
		else
		{
			(*ppdrgpcrChild)->Append(const_cast<CColRef*>(pcr));
		}
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PdrgpexprPreprocessSetOpInputs
//
//	@doc:
//		Pre-process inputs to the set operator and add casting when needed
//
//---------------------------------------------------------------------------
DrgPexpr *
CTranslatorDXLToExpr::PdrgpexprPreprocessSetOpInputs
	(
	const CDXLNode *pdxln,
	DrgDrgPcr *pdrgdrgpcrInput,
	ULongPtrArray *pdrgpulOutput
	)
{
	GPOS_ASSERT(NULL != pdxln);
	GPOS_ASSERT(NULL != pdrgdrgpcrInput);
	GPOS_ASSERT(NULL != pdrgpulOutput);
	
	// array of child expression
	DrgPexpr *pdrgpexpr = GPOS_NEW(m_memory_pool) DrgPexpr(m_memory_pool);

	CDXLLogicalSetOp *dxl_op = CDXLLogicalSetOp::Cast(pdxln->GetOperator());

	const ULONG ulArity = pdxln->Arity();
	GPOS_ASSERT(2 <= ulArity);
	GPOS_ASSERT(ulArity == dxl_op->ChildCount());

	const ULONG ulOutputCols = dxl_op->Arity();

	for (ULONG ul = 0; ul < ulArity; ul++)
	{
		CExpression *pexprChild = NULL;
		DrgPcr *pdrgpcrInput = NULL;
		DrgPexpr *pdrgpexprChildProjElems = NULL;
		BuildSetOpChild(pdxln, ul, &pexprChild, &pdrgpcrInput, &pdrgpexprChildProjElems);
		GPOS_ASSERT(ulOutputCols == pdrgpcrInput->Size());
		GPOS_ASSERT(NULL != pexprChild);

		pdrgdrgpcrInput->Append(pdrgpcrInput);

		if (0 < pdrgpexprChildProjElems->Size())
		{
			CExpression *pexprChildProject = GPOS_NEW(m_memory_pool) CExpression
															(
															m_memory_pool,
															GPOS_NEW(m_memory_pool) CLogicalProject(m_memory_pool),
															pexprChild,
															GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, GPOS_NEW(m_memory_pool) CScalarProjectList(m_memory_pool), pdrgpexprChildProjElems)
															);
			pdrgpexpr->Append(pexprChildProject);
		}
		else
		{
			pdrgpexpr->Append(pexprChild);
			pdrgpexprChildProjElems->Release();
		}
	}

	// create the set operation's array of output column identifiers
	for (ULONG ulOutputColPos = 0; ulOutputColPos < ulOutputCols; ulOutputColPos++)
	{
		const CDXLColDescr *pdxlcdOutput = dxl_op->GetColumnDescrAt(ulOutputColPos);
		pdrgpulOutput->Append(GPOS_NEW(m_memory_pool) ULONG (pdxlcdOutput->Id()));
	}
	
	return pdrgpexpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::FCastingUnknownType
//
//	@doc:
//		Check if we currently support the casting of such column types
//---------------------------------------------------------------------------
BOOL
CTranslatorDXLToExpr::FCastingUnknownType
	(
	IMDId *pmdidSource,
	IMDId *pmdidDest
	)
{
	return ((pmdidSource->Equals(&CMDIdGPDB::m_mdidUnknown) || pmdidDest->Equals(&CMDIdGPDB::m_mdidUnknown)));
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PcrLookup
//
//	@doc:
// 		Look up the column reference in the hash map. We raise an exception if
//		the column is not found
//---------------------------------------------------------------------------
CColRef *
CTranslatorDXLToExpr::PcrLookup
	(
	HMUlCr *phmulcr,
	ULONG col_id
	)
{
	GPOS_ASSERT(NULL != phmulcr);
	GPOS_ASSERT(ULONG_MAX != col_id);

	// get its column reference from the hash map
	CColRef *pcr = phmulcr->Find(&col_id);
    if (NULL == pcr)
    {
    	GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXL2ExprAttributeNotFound, col_id);
    }

	return pcr;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PcrCreate
//
//	@doc:
// 		Create new column reference and add to the hashmap maintaining
//		the mapping between DXL ColIds and column reference.
//
//---------------------------------------------------------------------------
CColRef *
CTranslatorDXLToExpr::PcrCreate
	(
	const CColRef *pcr,
	const IMDType *pmdtype,
	INT type_modifier,
	BOOL fStoreMapping,
	ULONG col_id
	)
{
	// generate a new column reference
	CName name(pcr->Name().Pstr());
	CColRef *pcrNew = m_pcf->PcrCreate(pmdtype, type_modifier, name);

	if (fStoreMapping)
	{
#ifdef GPOS_DEBUG
		BOOL fResult =
#endif // GPOS_DEBUG
				m_phmulcr->Insert(GPOS_NEW(m_memory_pool) ULONG(col_id), pcrNew);

		GPOS_ASSERT(fResult);
	}

	return pcrNew;
}
//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::pdrgpcrOutput
//
//	@doc:
// 		Construct an array of new column references from the array of
//		DXL column descriptors
//
//---------------------------------------------------------------------------
DrgPcr *
CTranslatorDXLToExpr::Pdrgpcr
	(
	const ColumnDescrDXLArray *pdrgpdxlcd
	)
{
	GPOS_ASSERT(NULL != pdrgpdxlcd);
	DrgPcr *pdrgpcrOutput = GPOS_NEW(m_memory_pool) DrgPcr(m_memory_pool);
	ULONG ulOutputCols = pdrgpdxlcd->Size();
	for (ULONG ul = 0; ul < ulOutputCols; ul++)
	{
		CDXLColDescr *pdxlcd = (*pdrgpdxlcd)[ul];
		IMDId *pmdid = pdxlcd->MDIdType();
		const IMDType *pmdtype = m_pmda->Pmdtype(pmdid);

		CName name(pdxlcd->MdName()->Pstr());
		// generate a new column reference
		CColRef *pcr = m_pcf->PcrCreate(pmdtype, pdxlcd->TypeModifier(), name);
		pdrgpcrOutput->Append(pcr);
	}

	return pdrgpcrOutput;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::ConstructDXLColId2ColRefMapping
//
//	@doc:
// 		Construct the mapping between the DXL ColId and CColRef
//
//---------------------------------------------------------------------------
void
CTranslatorDXLToExpr::ConstructDXLColId2ColRefMapping
	(
	const ColumnDescrDXLArray *pdrgpdxlcd,
	const DrgPcr *pdrgpcr
	)
{
	GPOS_ASSERT(NULL != pdrgpdxlcd);
	GPOS_ASSERT(NULL != pdrgpcr);

	const ULONG ulColumns = pdrgpdxlcd->Size();
	GPOS_ASSERT(pdrgpcr->Size() == ulColumns);

	// construct the mapping between the DXL ColId and CColRef
	for (ULONG ul = 0; ul < ulColumns ; ul++)
	{
		CColRef *pcr = (*pdrgpcr)[ul];
		GPOS_ASSERT(NULL != pcr);

		const CDXLColDescr *pdxlcd = (*pdrgpdxlcd)[ul];
		GPOS_ASSERT(NULL != pdxlcd && !pdxlcd->IsDropped());

		// copy key
		ULONG *pulKey = GPOS_NEW(m_memory_pool) ULONG(pdxlcd->Id());
#ifdef GPOS_DEBUG
		BOOL fResult =
#endif // GPOS_DEBUG
		m_phmulcr->Insert(pulKey, pcr);

		GPOS_ASSERT(fResult);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprLogicalSelect
//
//	@doc:
// 		Create a logical select expr from a DXL logical select
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprLogicalSelect
	(
	const CDXLNode *pdxln
	)
{
	GPOS_ASSERT(NULL != pdxln);

	// translate the child dxl node
	CDXLNode *child_dxlnode = (*pdxln)[1];
	CExpression *pexprChild = PexprLogical(child_dxlnode);

	// translate scalar condition
	CDXLNode *pdxlnCond = (*pdxln)[0];
	CExpression *pexprCond = PexprScalar(pdxlnCond);
	CLogicalSelect *plgselect = GPOS_NEW(m_memory_pool) CLogicalSelect(m_memory_pool);
	CExpression *pexprSelect = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, plgselect, pexprChild,	pexprCond);

	return pexprSelect;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprLogicalProject
//
//	@doc:
// 		Create a logical project expr from a DXL logical project
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprLogicalProject
	(
	const CDXLNode *pdxln
	)
{
	GPOS_ASSERT(NULL != pdxln);

	// translate the child dxl node
	CDXLNode *child_dxlnode = (*pdxln)[1];
	CExpression *pexprChild = PexprLogical(child_dxlnode);

	// translate the project list
	CDXLNode *pdxlnPrL = (*pdxln)[0];
	GPOS_ASSERT(EdxlopScalarProjectList == pdxlnPrL->GetOperator()->GetDXLOperator());
	CExpression *pexprProjList = PexprScalarProjList(pdxlnPrL);

	CLogicalProject *popProject = GPOS_NEW(m_memory_pool) CLogicalProject(m_memory_pool);
	CExpression *pexprProject = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popProject, pexprChild,	pexprProjList);

	return pexprProject;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprLogicalCTEAnchor
//
//	@doc:
// 		Create a logical CTE anchor expr from a DXL logical CTE anchor
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprLogicalCTEAnchor
	(
	const CDXLNode *pdxln
	)
{
	GPOS_ASSERT(NULL != pdxln);
	CDXLLogicalCTEAnchor *pdxlopCTEAnchor = CDXLLogicalCTEAnchor::Cast(pdxln->GetOperator());
	ULONG ulCTEId = pdxlopCTEAnchor->UlId();

	CDXLNode *pdxlnCTEProducer = m_phmulpdxlnCTEProducer->Find(&ulCTEId);
	GPOS_ASSERT(NULL != pdxlnCTEProducer);

	ULONG ulId = UlMapCTEId(ulCTEId);
	// mark that we are about to start processing this new CTE and keep track
	// of the previous one
	ULONG ulCTEPrevious = m_ulCTEId;
	m_ulCTEId = ulId;
	CExpression *pexprProducer = Pexpr(pdxlnCTEProducer);
	GPOS_ASSERT(NULL != pexprProducer);
	m_ulCTEId = ulCTEPrevious;
	
	CColRefSet *pcrsProducerOuter = CDrvdPropRelational::Pdprel(pexprProducer->PdpDerive())->PcrsOuter();
	if (0 < pcrsProducerOuter->Size())
	{
		GPOS_RAISE
				(
				gpopt::ExmaGPOPT,
				gpopt::ExmiUnsupportedOp,
				GPOS_WSZ_LIT("CTE with outer references")
				);
	}

	COptCtxt::PoctxtFromTLS()->Pcteinfo()->AddCTEProducer(pexprProducer);
	pexprProducer->Release();

	// translate the child dxl node
	CExpression *pexprChild = PexprLogical((*pdxln)[0]);

	return GPOS_NEW(m_memory_pool) CExpression
						(
						m_memory_pool,
						GPOS_NEW(m_memory_pool) CLogicalCTEAnchor(m_memory_pool, ulId),
						pexprChild
						);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprLogicalCTEProducer
//
//	@doc:
// 		Create a logical CTE producer expr from a DXL logical CTE producer
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprLogicalCTEProducer
	(
	const CDXLNode *pdxln
	)
{
	GPOS_ASSERT(NULL != pdxln);
	CDXLLogicalCTEProducer *pdxlopCTEProducer = CDXLLogicalCTEProducer::Cast(pdxln->GetOperator());
	ULONG ulId = UlMapCTEId(pdxlopCTEProducer->UlId());

	// translate the child dxl node
	CExpression *pexprChild = PexprLogical((*pdxln)[0]);

	// a column of the cte producer's child may be used in CTE producer output multiple times;
	// CTE consumer maintains a hash map between the cte producer columns to cte consumer columns.
	// To avoid losing mapping information of duplicate producer columns, we introduce a relabel
	// node (project element) for each duplicate entry of the producer column.

	DrgPexpr *pdrgpexprPrEl = GPOS_NEW(m_memory_pool) DrgPexpr(m_memory_pool);
	CColRefSet *pcrsProducer = GPOS_NEW(m_memory_pool) CColRefSet(m_memory_pool);
	DrgPcr *pdrgpcr = GPOS_NEW(m_memory_pool) DrgPcr(m_memory_pool);

	ULongPtrArray *pdrgpulCols = pdxlopCTEProducer->PdrgpulColIds();
	const ULONG ulLen = pdrgpulCols->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		ULONG *pulColId = (*pdrgpulCols)[ul];
		const CColRef *pcr = m_phmulcr->Find(pulColId);
		GPOS_ASSERT(NULL != pcr);

		if (pcrsProducer->FMember(pcr))
		{
			// the column was previously used, so introduce a project node to relabel
			// the next use of the column reference
			CColRef *pcrNew = m_pcf->PcrCreate(pcr);
			CExpression *pexprPrEl = CUtils::PexprScalarProjectElement
												(
												m_memory_pool,
												pcrNew,
												GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, GPOS_NEW(m_memory_pool) CScalarIdent(m_memory_pool, pcr))
												);
			pdrgpexprPrEl->Append(pexprPrEl);
			pcr = pcrNew;
		}

		pdrgpcr->Append(const_cast<CColRef*>(pcr));
		pcrsProducer->Include(pcr);
	}

	GPOS_ASSERT(ulLen == pdrgpcr->Size());

	if (0 < pdrgpexprPrEl->Size())
	{
		pdrgpexprPrEl->AddRef();
		CExpression *pexprPr = GPOS_NEW(m_memory_pool) CExpression
											(
											m_memory_pool,
											GPOS_NEW(m_memory_pool) CLogicalProject(m_memory_pool),
											pexprChild,
											GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, GPOS_NEW(m_memory_pool) CScalarProjectList(m_memory_pool), pdrgpexprPrEl)
											);
		pexprChild = pexprPr;
	}

	// clean up
	pdrgpexprPrEl->Release();
	pcrsProducer->Release();

	return GPOS_NEW(m_memory_pool) CExpression
						(
						m_memory_pool,
						GPOS_NEW(m_memory_pool) CLogicalCTEProducer(m_memory_pool, ulId, pdrgpcr),
						pexprChild
						);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprLogicalCTEConsumer
//
//	@doc:
// 		Create a logical CTE consumer expr from a DXL logical CTE consumer
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprLogicalCTEConsumer
	(
	const CDXLNode *pdxln
	)
{
	GPOS_ASSERT(NULL != pdxln);

	CDXLLogicalCTEConsumer *pdxlopCTEConsumer = CDXLLogicalCTEConsumer::Cast(pdxln->GetOperator());
	ULONG ulId = UlMapCTEId(pdxlopCTEConsumer->UlId());

	ULongPtrArray *pdrgpulCols = pdxlopCTEConsumer->PdrgpulColIds();

	// create new col refs
	CCTEInfo *pcteinfo = COptCtxt::PoctxtFromTLS()->Pcteinfo();
	CExpression *pexprProducer = pcteinfo->PexprCTEProducer(ulId);
	GPOS_ASSERT(NULL != pexprProducer);

	DrgPcr *pdrgpcrProducer = CLogicalCTEProducer::PopConvert(pexprProducer->Pop())->Pdrgpcr();
	DrgPcr *pdrgpcrConsumer = CUtils::PdrgpcrCopy(m_memory_pool, pdrgpcrProducer);

	// add new colrefs to mapping
	const ULONG ulCols = pdrgpcrConsumer->Size();
	GPOS_ASSERT(pdrgpulCols->Size() == ulCols);
	for (ULONG ul = 0; ul < ulCols; ul++)
	{
		ULONG *pulColId = GPOS_NEW(m_memory_pool) ULONG(*(*pdrgpulCols)[ul]);
		CColRef *pcr = (*pdrgpcrConsumer)[ul];

#ifdef GPOS_DEBUG
		BOOL fResult =
#endif // GPOS_DEBUG
		m_phmulcr->Insert(pulColId, pcr);
		GPOS_ASSERT(fResult);
	}

	pcteinfo->IncrementConsumers(ulId, m_ulCTEId);
	return GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, GPOS_NEW(m_memory_pool) CLogicalCTEConsumer(m_memory_pool, ulId, pdrgpcrConsumer));
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::UlMapCTEId
//
//	@doc:
// 		Return an new CTE id based on the CTE id used in DXL, since we may
//		introduce new CTEs during translation that did not exist in DXL
//
//---------------------------------------------------------------------------
ULONG
CTranslatorDXLToExpr::UlMapCTEId
	(
	const ULONG ulIdOld
	)
{
	ULONG *pulNewId =  m_phmululCTE->Find(&ulIdOld);
	if (NULL == pulNewId)
	{
		pulNewId = GPOS_NEW(m_memory_pool) ULONG(COptCtxt::PoctxtFromTLS()->Pcteinfo()->next_id());

#ifdef GPOS_DEBUG
		BOOL fInserted =
#endif
		m_phmululCTE->Insert(GPOS_NEW(m_memory_pool) ULONG(ulIdOld), pulNewId);
		GPOS_ASSERT(fInserted);
	}

	return *pulNewId;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprLogicalInsert
//
//	@doc:
// 		Create a logical DML on top of a project from a DXL logical insert
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprLogicalInsert
	(
	const CDXLNode *pdxln
	)
{
	GPOS_ASSERT(NULL != pdxln);
	CDXLLogicalInsert *pdxlopInsert = CDXLLogicalInsert::Cast(pdxln->GetOperator());

	// translate the child dxl node
	CDXLNode *child_dxlnode = (*pdxln)[0];
	CExpression *pexprChild = PexprLogical(child_dxlnode);

	CTableDescriptor *ptabdesc = Ptabdesc(pdxlopInsert->GetTableDescr());

	ULongPtrArray *pdrgpulSourceCols = pdxlopInsert->GetSrcColIdsArray();
	DrgPcr *pdrgpcr = CTranslatorDXLToExprUtils::Pdrgpcr(m_memory_pool, m_phmulcr, pdrgpulSourceCols);

	return GPOS_NEW(m_memory_pool) CExpression
						(
						m_memory_pool,
						GPOS_NEW(m_memory_pool) CLogicalInsert(m_memory_pool, ptabdesc, pdrgpcr),
						pexprChild
						);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprLogicalDelete
//
//	@doc:
// 		Create a logical DML on top of a project from a DXL logical delete
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprLogicalDelete
	(
	const CDXLNode *pdxln
	)
{
	GPOS_ASSERT(NULL != pdxln);
	CDXLLogicalDelete *pdxlopDelete = CDXLLogicalDelete::Cast(pdxln->GetOperator());

	// translate the child dxl node
	CDXLNode *child_dxlnode = (*pdxln)[0];
	CExpression *pexprChild = PexprLogical(child_dxlnode);

	CTableDescriptor *ptabdesc = Ptabdesc(pdxlopDelete->GetTableDescr());

	ULONG ulCtid = pdxlopDelete->GetCtIdColId();
	ULONG ulSegmentId = pdxlopDelete->GetSegmentIdColId();

	CColRef *pcrCtid = PcrLookup(m_phmulcr, ulCtid);
	CColRef *pcrSegmentId = PcrLookup(m_phmulcr, ulSegmentId);

	ULongPtrArray *pdrgpulCols = pdxlopDelete->GetDeletionColIdArray();
	DrgPcr *pdrgpcr = CTranslatorDXLToExprUtils::Pdrgpcr(m_memory_pool, m_phmulcr, pdrgpulCols);

	return GPOS_NEW(m_memory_pool) CExpression
						(
						m_memory_pool,
						GPOS_NEW(m_memory_pool) CLogicalDelete(m_memory_pool, ptabdesc, pdrgpcr, pcrCtid, pcrSegmentId),
						pexprChild
						);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprLogicalUpdate
//
//	@doc:
// 		Create a logical DML on top of a split from a DXL logical update
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprLogicalUpdate
	(
	const CDXLNode *pdxln
	)
{
	GPOS_ASSERT(NULL != pdxln);
	CDXLLogicalUpdate *pdxlopUpdate = CDXLLogicalUpdate::Cast(pdxln->GetOperator());

	// translate the child dxl node
	CDXLNode *child_dxlnode = (*pdxln)[0];
	CExpression *pexprChild = PexprLogical(child_dxlnode);

	CTableDescriptor *ptabdesc = Ptabdesc(pdxlopUpdate->GetTableDescr());

	ULONG ulCtid = pdxlopUpdate->GetCtIdColId();
	ULONG ulSegmentId = pdxlopUpdate->GetSegmentIdColId();

	CColRef *pcrCtid = PcrLookup(m_phmulcr, ulCtid);
	CColRef *pcrSegmentId = PcrLookup(m_phmulcr, ulSegmentId);

	ULongPtrArray *pdrgpulInsertCols = pdxlopUpdate->PdrgpulInsert();
	DrgPcr *pdrgpcrInsert = CTranslatorDXLToExprUtils::Pdrgpcr(m_memory_pool, m_phmulcr, pdrgpulInsertCols);

	ULongPtrArray *pdrgpulDeleteCols = pdxlopUpdate->GetDeletionColIdArray();
	DrgPcr *pdrgpcrDelete = CTranslatorDXLToExprUtils::Pdrgpcr(m_memory_pool, m_phmulcr, pdrgpulDeleteCols);

	CColRef *pcrTupleOid = NULL;
	if (pdxlopUpdate->FPreserveOids())
	{
		ULONG ulTupleOid = pdxlopUpdate->UlTupleOid();
		pcrTupleOid = PcrLookup(m_phmulcr, ulTupleOid);
	}
	
	return GPOS_NEW(m_memory_pool) CExpression
						(
						m_memory_pool,
						GPOS_NEW(m_memory_pool) CLogicalUpdate(m_memory_pool, ptabdesc, pdrgpcrDelete, pdrgpcrInsert, pcrCtid, pcrSegmentId, pcrTupleOid),
						pexprChild
						);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprLogicalCTAS
//
//	@doc:
// 		Create a logical Insert from a logical DXL CTAS operator
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprLogicalCTAS
	(
	const CDXLNode *pdxln
	)
{
	GPOS_ASSERT(NULL != pdxln);
	CDXLLogicalCTAS *pdxlopCTAS = CDXLLogicalCTAS::Cast(pdxln->GetOperator());

	// translate the child dxl node
	CDXLNode *child_dxlnode = (*pdxln)[0];
	CExpression *pexprChild = PexprLogical(child_dxlnode);

	RegisterMDRelationCtas(pdxlopCTAS);
	CTableDescriptor *ptabdesc = PtabdescFromCTAS(pdxlopCTAS);

	ULongPtrArray *pdrgpulSourceCols = pdxlopCTAS->PdrgpulSource();
	DrgPcr *pdrgpcr = CTranslatorDXLToExprUtils::Pdrgpcr(m_memory_pool, m_phmulcr, pdrgpulSourceCols);

	return GPOS_NEW(m_memory_pool) CExpression
						(
						m_memory_pool,
						GPOS_NEW(m_memory_pool) CLogicalInsert(m_memory_pool, ptabdesc, pdrgpcr),
						pexprChild
						);
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprLogicalGroupBy
//
//	@doc:
// 		Create a logical group by expr from a DXL logical group by aggregate
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprLogicalGroupBy
	(
	const CDXLNode *pdxln
	)
{
	// get children
	CDXLLogicalGroupBy *pdxlopGrpby = CDXLLogicalGroupBy::Cast(pdxln->GetOperator());
	CDXLNode *pdxlnPrL = (*pdxln)[0];
	CDXLNode *child_dxlnode = (*pdxln)[1];
	
	// translate the child dxl node
	CExpression *pexprChild = PexprLogical(child_dxlnode);

	// translate proj list
	CExpression *pexprProjList = PexprScalarProjList(pdxlnPrL);

	// translate grouping columns
	DrgPcr *pdrgpcrGroupingCols = CTranslatorDXLToExprUtils::Pdrgpcr(m_memory_pool, m_phmulcr, pdxlopGrpby->GetGroupingColidArray());
	
	if (0 != pexprProjList->Arity())
	{
		GPOS_ASSERT(CUtils::FHasGlobalAggFunc(pexprProjList));
	}

	return GPOS_NEW(m_memory_pool) CExpression
						(
						m_memory_pool,
						GPOS_NEW(m_memory_pool) CLogicalGbAgg
									(
									m_memory_pool,
									pdrgpcrGroupingCols,
									COperator::EgbaggtypeGlobal /*egbaggtype*/
									),
						pexprChild,
						pexprProjList
						);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprLogicalLimit
//
//	@doc:
// 		Create a logical limit expr from a DXL logical limit node
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprLogicalLimit
	(
	const CDXLNode *pdxln
	)
{
	GPOS_ASSERT(NULL != pdxln && EdxlopLogicalLimit == pdxln->GetOperator()->GetDXLOperator());

	// get children
	CDXLNode *sort_col_list_dxl = (*pdxln)[EdxllogicallimitIndexSortColList];
	CDXLNode *pdxlnCount = (*pdxln)[EdxllogicallimitIndexLimitCount];
	CDXLNode *pdxlnOffset = (*pdxln)[EdxllogicallimitIndexLimitOffset];
	CDXLNode *child_dxlnode = (*pdxln)[EdxllogicallimitIndexChildPlan];
	
	// translate count	
	CExpression *pexprLimitCount = NULL;
	BOOL fHasCount = false;
	if (1 == pdxlnCount->Arity())
	{
		// translate limit count
		pexprLimitCount = Pexpr((*pdxlnCount)[0]);
		COperator *popCount = pexprLimitCount->Pop();
		BOOL fConst = (COperator::EopScalarConst == popCount->Eopid());
		if (!fConst ||
			(fConst && !CScalarConst::PopConvert(popCount)->Pdatum()->IsNull()))
		{
			fHasCount = true;
		}
	}
	else
	{
		// no limit count is specified, manufacture a null count
		pexprLimitCount = CUtils::PexprScalarConstInt8(m_memory_pool, 0 /*iVal*/, true /*is_null*/);
	}
	
	// translate offset
	CExpression *pexprLimitOffset = NULL;

	if (1 == pdxlnOffset->Arity())
	{
		pexprLimitOffset = Pexpr((*pdxlnOffset)[0]);
	}
	else
	{
		// manufacture an OFFSET 0
		pexprLimitOffset = CUtils::PexprScalarConstInt8(m_memory_pool, 0 /*iVal*/);
	}
	
	// translate limit child
	CExpression *pexprChild = PexprLogical(child_dxlnode);

	// translate sort col list
	COrderSpec *pos = Pos(sort_col_list_dxl);
	
	BOOL fNonRemovable = CDXLLogicalLimit::Cast(pdxln->GetOperator())->IsTopLimitUnderDMLorCTAS();
	CLogicalLimit *popLimit =
			GPOS_NEW(m_memory_pool) CLogicalLimit(m_memory_pool, pos, true /*fGlobal*/, fHasCount, fNonRemovable);
	return GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popLimit, pexprChild, pexprLimitOffset, pexprLimitCount);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprLogicalSeqPr
//
//	@doc:
// 		Create a logical sequence expr from a DXL logical window
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprLogicalSeqPr
	(
	const CDXLNode *pdxln
	)
{
	GPOS_ASSERT(NULL != pdxln);
	
	CDXLLogicalWindow *pdxlopWindow = CDXLLogicalWindow::Cast(pdxln->GetOperator());
	
	CDXLNode *pdxlnWindowChild = (*pdxln)[1];
	CExpression *pexprWindowChild = PexprLogical(pdxlnWindowChild);

	// maintains the map between window specification position -> list of project elements
	// used to generate a cascade of window nodes
	HMUlPdrgpexpr *phmulpdrgpexpr = GPOS_NEW(m_memory_pool) HMUlPdrgpexpr(m_memory_pool);

	CDXLNode *pdxlnPrL = (*pdxln)[0];
	GPOS_ASSERT(EdxlopScalarProjectList == pdxlnPrL->GetOperator()->GetDXLOperator());
	const ULONG ulArity = pdxlnPrL->Arity();
	GPOS_ASSERT(0 < ulArity);

	for (ULONG ul = 0; ul < ulArity; ul++)
	{
		CDXLNode *pdxlnProjElem = (*pdxlnPrL)[ul];

		GPOS_ASSERT(NULL != pdxlnProjElem);
		GPOS_ASSERT(EdxlopScalarProjectElem == pdxlnProjElem->GetOperator()->GetDXLOperator() && 1 == pdxlnProjElem->Arity());

		CDXLNode *pdxlnPrElChild = (*pdxlnProjElem)[0];
		// expect the project list to be normalized and expect to only find window functions and scalar identifiers
		GPOS_ASSERT(EdxlopScalarIdent == pdxlnPrElChild->GetOperator()->GetDXLOperator() || EdxlopScalarWindowRef == pdxlnPrElChild->GetOperator()->GetDXLOperator());
		CDXLScalarProjElem *pdxlopPrEl = CDXLScalarProjElem::Cast(pdxlnProjElem->GetOperator());
		
		if (EdxlopScalarWindowRef == pdxlnPrElChild->GetOperator()->GetDXLOperator())
		{
			// translate window function
			CDXLScalarWindowRef *pdxlopWindowRef = CDXLScalarWindowRef::Cast(pdxlnPrElChild->GetOperator());
			CExpression *pexprScWindowFunc = Pexpr(pdxlnPrElChild); 

			CScalar *popScalar = CScalar::PopConvert(pexprScWindowFunc->Pop());
			IMDId *pmdid = popScalar->MDIdType();
			const IMDType *pmdtype = m_pmda->Pmdtype(pmdid);

			CName name(pdxlopPrEl->PmdnameAlias()->Pstr());

			// generate a new column reference
			CColRef *pcr = m_pcf->PcrCreate(pmdtype, popScalar->TypeModifier(), name);
			CScalarProjectElement *popScPrEl = GPOS_NEW(m_memory_pool) CScalarProjectElement(m_memory_pool, pcr);

			// store colid -> colref mapping
#ifdef GPOS_DEBUG
		BOOL fInserted =
#endif
			m_phmulcr->Insert(GPOS_NEW(m_memory_pool) ULONG(pdxlopPrEl->UlId()), pcr);
			GPOS_ASSERT(fInserted);

			// generate a project element
			CExpression *pexprProjElem = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popScPrEl, pexprScWindowFunc);
			
			// add the created project element to the project list of the window node
			ULONG ulSpecPos = pdxlopWindowRef->UlWinSpecPos();			
			const DrgPexpr *pdrgpexpr = phmulpdrgpexpr->Find(&ulSpecPos); 			
			if (NULL == pdrgpexpr)
			{
				DrgPexpr *pdrgpexprNew = GPOS_NEW(m_memory_pool) DrgPexpr(m_memory_pool);
				pdrgpexprNew->Append(pexprProjElem);
#ifdef GPOS_DEBUG
			BOOL fInsert =
#endif
				phmulpdrgpexpr->Insert(GPOS_NEW(m_memory_pool) ULONG(ulSpecPos), pdrgpexprNew);
				GPOS_ASSERT(fInsert);
			}
			else 
			{
				const_cast<DrgPexpr *>(pdrgpexpr)->Append(pexprProjElem);
			}
		}
	}

	// create the window operators (or when applicable a tree of window operators)
	CExpression *pexprLgSequence = NULL;
	HMIterUlPdrgpexpr hmiterulpdrgexpr(phmulpdrgpexpr);
	
	while (hmiterulpdrgexpr.Advance())
	{
		ULONG ulPos = *(hmiterulpdrgexpr.Key());
		CDXLWindowSpec *pdxlws = pdxlopWindow->Pdxlws(ulPos);
		
		const DrgPexpr *pdrgpexpr = hmiterulpdrgexpr.Value();
		GPOS_ASSERT(NULL != pdrgpexpr);
		CScalarProjectList *popPrL = GPOS_NEW(m_memory_pool) CScalarProjectList(m_memory_pool);
		CExpression *pexprProjList = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popPrL, const_cast<DrgPexpr *>(pdrgpexpr));
		
		DrgPcr *pdrgpcr = PdrgpcrPartitionByCol(pdxlws->GetPartitionByColIdArray());
		CDistributionSpec *pds = NULL;
		if (0 < pdrgpcr->Size())
		{
			DrgPexpr *pdrgpexprScalarIdents = CUtils::PdrgpexprScalarIdents(m_memory_pool, pdrgpcr);
			pds = GPOS_NEW(m_memory_pool) CDistributionSpecHashed(pdrgpexprScalarIdents, true /* fNullsCollocated */);
		}
		else
		{
			// if no partition-by columns, window functions need gathered input
			pds = GPOS_NEW(m_memory_pool) CDistributionSpecSingleton(CDistributionSpecSingleton::EstMaster);
		}
		pdrgpcr->Release();
		
		DrgPwf *pdrgpwf = GPOS_NEW(m_memory_pool) DrgPwf(m_memory_pool);
		CWindowFrame *pwf = NULL;
		if (NULL != pdxlws->GetWindowFrame()) 
		{
			pwf = Pwf(pdxlws->GetWindowFrame());
		}
		else
		{
			// create an empty frame
			pwf = const_cast<CWindowFrame *>(CWindowFrame::PwfEmpty());
			pwf->AddRef();
		}
		pdrgpwf->Append(pwf);
		
		DrgPos *pdrgpos = GPOS_NEW(m_memory_pool) DrgPos(m_memory_pool);
		if (NULL != pdxlws->GetSortColListDXL()) 
		{
			COrderSpec *pos = Pos(pdxlws->GetSortColListDXL());
			pdrgpos->Append(pos);
		}
		else
		{
			pdrgpos->Append(GPOS_NEW(m_memory_pool) COrderSpec(m_memory_pool));
		}
		
		CLogicalSequenceProject *popLgSequence = GPOS_NEW(m_memory_pool) CLogicalSequenceProject(m_memory_pool, pds, pdrgpos, pdrgpwf);
		pexprLgSequence = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popLgSequence, pexprWindowChild, pexprProjList);
		pexprWindowChild = pexprLgSequence;
	}
	
	GPOS_ASSERT(NULL != pexprLgSequence);
	
	// clean up
	phmulpdrgpexpr->Release();

	return pexprLgSequence;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PdrgpcrPartitionByCol
//
//	@doc:
// 		Create the array of column reference used in the partition by column
//		list of a window specification
//
//---------------------------------------------------------------------------
DrgPcr *
CTranslatorDXLToExpr::PdrgpcrPartitionByCol
	(
	const ULongPtrArray *partition_by_col_id_array
	)
{
	const ULONG ulSize = partition_by_col_id_array->Size();
	DrgPcr *pdrgpcr = GPOS_NEW(m_memory_pool) DrgPcr(m_memory_pool);
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		const ULONG *pulColId = (*partition_by_col_id_array)[ul];

		// get its column reference from the hash map
		CColRef *pcr =  PcrLookup(m_phmulcr, *pulColId);
		pdrgpcr->Append(pcr);
	}
	
	return pdrgpcr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::Pwf
//
//	@doc:
// 		Create a window frame from a DXL window frame node
//
//---------------------------------------------------------------------------
CWindowFrame *
CTranslatorDXLToExpr::Pwf
	(
	const CDXLWindowFrame *window_frame
	)
{
	CDXLNode *pdxlnTrail = window_frame->PdxlnTrailing();
	CDXLNode *pdxlnLead = window_frame->PdxlnLeading();

	CWindowFrame::EFrameBoundary efbLead = Efb(CDXLScalarWindowFrameEdge::Cast(pdxlnLead->GetOperator())->Edxlfb());
	CWindowFrame::EFrameBoundary efbTrail = Efb(CDXLScalarWindowFrameEdge::Cast(pdxlnTrail->GetOperator())->Edxlfb());

	CExpression *pexprTrail = NULL;
	if (0 != pdxlnTrail->Arity())
	{
		pexprTrail = Pexpr((*pdxlnTrail)[0]);
	}

	CExpression *pexprLead = NULL;
	if (0 != pdxlnLead->Arity())
	{
		pexprLead = Pexpr((*pdxlnLead)[0]);
	}

	CWindowFrame::EFrameExclusionStrategy efes = Efes(window_frame->Edxlfes());
	CWindowFrame::EFrameSpec efs = CWindowFrame::EfsRows;
	if (EdxlfsRange == window_frame->Edxlfs())
	{
		efs = CWindowFrame::EfsRange;
	}
	
	CWindowFrame *pwf = GPOS_NEW(m_memory_pool) CWindowFrame(m_memory_pool, efs, efbLead, efbTrail, pexprLead, pexprTrail, efes);
	
	return pwf;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::Efb
//
//	@doc:
//		Return the window frame boundary
//
//---------------------------------------------------------------------------
CWindowFrame::EFrameBoundary
CTranslatorDXLToExpr::Efb
	(
	EdxlFrameBoundary edxlfb
	)
	const
{
	ULONG rgrgulMapping[][2] =
	{
		{EdxlfbUnboundedPreceding, CWindowFrame::EfbUnboundedPreceding},
		{EdxlfbBoundedPreceding, CWindowFrame::EfbBoundedPreceding},
		{EdxlfbCurrentRow, CWindowFrame::EfbCurrentRow},
		{EdxlfbUnboundedFollowing, CWindowFrame::EfbUnboundedFollowing},
		{EdxlfbBoundedFollowing, CWindowFrame::EfbBoundedFollowing},
		{EdxlfbDelayedBoundedPreceding, CWindowFrame::EfbDelayedBoundedPreceding},
		{EdxlfbDelayedBoundedFollowing, CWindowFrame::EfbDelayedBoundedFollowing}
	};

#ifdef GPOS_DEBUG
	const ULONG ulArity = GPOS_ARRAY_SIZE(rgrgulMapping);
	GPOS_ASSERT(ulArity > (ULONG) edxlfb  && "Invalid window frame boundary");
#endif
	CWindowFrame::EFrameBoundary efb = (CWindowFrame::EFrameBoundary)rgrgulMapping[(ULONG) edxlfb][1];
	
	return efb;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::Efes
//
//	@doc:
//		Return the window frame exclusion strategy
//
//---------------------------------------------------------------------------
CWindowFrame::EFrameExclusionStrategy
CTranslatorDXLToExpr::Efes
(
 EdxlFrameExclusionStrategy edxlfeb
 )
const
{
	ULONG rgrgulMapping[][2] =
	{
		{EdxlfesNone, CWindowFrame::EfesNone},
		{EdxlfesNulls, CWindowFrame::EfesNulls},
		{EdxlfesCurrentRow, CWindowFrame::EfesCurrentRow},
		{EdxlfesGroup, CWindowFrame::EfseMatchingOthers},
		{EdxlfesTies, CWindowFrame::EfesTies}
	};

#ifdef GPOS_DEBUG
	const ULONG ulArity = GPOS_ARRAY_SIZE(rgrgulMapping);
	GPOS_ASSERT(ulArity > (ULONG) edxlfeb  && "Invalid window frame exclusion strategy");
#endif
	CWindowFrame::EFrameExclusionStrategy efeb = (CWindowFrame::EFrameExclusionStrategy)rgrgulMapping[(ULONG) edxlfeb][1];

	return efeb;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprLogicalJoin
//
//	@doc:
// 		Create a logical join expr from a DXL logical join
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprLogicalJoin
	(
	const CDXLNode *pdxln
	)
{
	GPOS_ASSERT(NULL != pdxln);

	CDXLLogicalJoin *pdxlopJoin = CDXLLogicalJoin::Cast(pdxln->GetOperator());
	EdxlJoinType edxljt = pdxlopJoin->GetJoinType();

	if (EdxljtRight == edxljt)
	{
		return PexprRightOuterJoin(pdxln);
	}

	if (EdxljtInner != edxljt && EdxljtLeft != edxljt && EdxljtFull != edxljt)
	{
		GPOS_RAISE
				(
				gpopt::ExmaGPOPT,
				gpopt::ExmiUnsupportedOp,
				CDXLOperator::GetJoinTypeNameStr(pdxlopJoin->GetJoinType())->GetBuffer()
				);
	}

	DrgPexpr *pdrgpexprChildren = GPOS_NEW(m_memory_pool) DrgPexpr(m_memory_pool);

	const ULONG ulChildCount = pdxln->Arity();
	for (ULONG ul = 0; ul < ulChildCount-1; ++ul)
	{
		// get the next child dxl node and then translate it into an Expr
		CDXLNode *pdxlnNxtChild = (*pdxln)[ul];

		CExpression *pexprNxtChild = PexprLogical(pdxlnNxtChild);
		pdrgpexprChildren->Append(pexprNxtChild);
	}

	// get the scalar condition and then translate it
	CDXLNode *pdxlnCond = (*pdxln)[ulChildCount-1];
	CExpression *pexprCond = PexprScalar(pdxlnCond);
	pdrgpexprChildren->Append(pexprCond);

	return CUtils::PexprLogicalJoin(m_memory_pool, edxljt, pdrgpexprChildren);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprRightOuterJoin
//
//	@doc:
// 		Translate a DXL right outer join. The expression A ROJ B is translated
//		to: B LOJ A
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprRightOuterJoin
	(
	const CDXLNode *pdxln
	)
{
#ifdef GPOS_DEBUG
	CDXLLogicalJoin *pdxlopJoin = CDXLLogicalJoin::Cast(pdxln->GetOperator());
	const ULONG ulChildCount = pdxln->Arity();
#endif //GPOS_DEBUG
	GPOS_ASSERT(EdxljtRight == pdxlopJoin->GetJoinType() && 3 == ulChildCount);

	DrgPexpr *pdrgpexprChildren = GPOS_NEW(m_memory_pool) DrgPexpr(m_memory_pool);
	pdrgpexprChildren->Append(PexprLogical((*pdxln)[1]));
	pdrgpexprChildren->Append(PexprLogical((*pdxln)[0]));
	pdrgpexprChildren->Append(PexprScalar((*pdxln)[2]));

	return CUtils::PexprLogicalJoin(m_memory_pool, EdxljtLeft, pdrgpexprChildren);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::Ptabdesc
//
//	@doc:
//		Construct a table descriptor from DXL table descriptor 
//
//---------------------------------------------------------------------------
CTableDescriptor *
CTranslatorDXLToExpr::Ptabdesc
	(
	CDXLTableDescr *table_descr
	)
{
	CWStringConst strName(m_memory_pool, table_descr->MdName()->Pstr()->GetBuffer());

	IMDId *pmdid = table_descr->MDId();

	// get the relation information from the cache
	const IMDRelation *pmdrel = m_pmda->Pmdrel(pmdid);

	// construct mappings for columns that are not dropped
	HMIUl *phmiulAttnoColMapping = GPOS_NEW(m_memory_pool) HMIUl(m_memory_pool);
	HMIUl *phmiulAttnoPosMapping = GPOS_NEW(m_memory_pool) HMIUl(m_memory_pool);
	HMUlUl *phmululColMapping = GPOS_NEW(m_memory_pool) HMUlUl(m_memory_pool);
	
	const ULONG ulAllColumns = pmdrel->UlColumns();
	ULONG ulPosNonDropped = 0;
	for (ULONG ulPos = 0; ulPos < ulAllColumns; ulPos++)
	{
		const IMDColumn *pmdcol = pmdrel->Pmdcol(ulPos);
		if (pmdcol->IsDropped())
		{
			continue;
		}
		(void) phmiulAttnoColMapping->Insert(GPOS_NEW(m_memory_pool) INT(pmdcol->AttrNum()), GPOS_NEW(m_memory_pool) ULONG(ulPosNonDropped));
		(void) phmiulAttnoPosMapping->Insert(GPOS_NEW(m_memory_pool) INT(pmdcol->AttrNum()), GPOS_NEW(m_memory_pool) ULONG(ulPos));
		(void) phmululColMapping->Insert(GPOS_NEW(m_memory_pool) ULONG(ulPos), GPOS_NEW(m_memory_pool) ULONG(ulPosNonDropped));

		ulPosNonDropped++;
	}
	
	// get distribution policy
	IMDRelation::Ereldistrpolicy ereldistrpolicy = pmdrel->Ereldistribution();

	// get storage type
	IMDRelation::Erelstoragetype erelstorage = pmdrel->Erelstorage();

	pmdid->AddRef();
	CTableDescriptor *ptabdesc = GPOS_NEW(m_memory_pool) CTableDescriptor
						(
						m_memory_pool,
						pmdid,
						CName(m_memory_pool, &strName),
						pmdrel->FConvertHashToRandom(),
						ereldistrpolicy,
						erelstorage,
						table_descr->GetExecuteAsUserId()
						);

	const ULONG ulColumns = table_descr->Arity();
	for (ULONG ul = 0; ul < ulColumns; ul++)
	{
		const CDXLColDescr *pdxlcoldesc = table_descr->GetColumnDescrAt(ul);
		INT iAttno = pdxlcoldesc->AttrNum();

		ULONG *pulPos = phmiulAttnoPosMapping->Find(&iAttno);
		GPOS_ASSERT(NULL != pulPos);
		const IMDColumn *pmdcolNext = pmdrel->Pmdcol(*pulPos);

		BOOL fNullable = pmdcolNext->FNullable();

		GPOS_ASSERT(pdxlcoldesc->MDIdType()->IsValid());
		const IMDType *pmdtype = m_pmda->Pmdtype(pdxlcoldesc->MDIdType());

		GPOS_ASSERT(NULL != pdxlcoldesc->MdName()->Pstr()->GetBuffer());
		CWStringConst strColName(m_memory_pool, pdxlcoldesc->MdName()->Pstr()->GetBuffer());

		INT iAttNo = pdxlcoldesc->AttrNum();

		const ULONG ulWidth = pdxlcoldesc->Width();
		CColumnDescriptor *pcoldesc = GPOS_NEW(m_memory_pool) CColumnDescriptor
													(
													m_memory_pool,
													pmdtype,
													pdxlcoldesc->TypeModifier(),
													CName(m_memory_pool, &strColName),
													iAttNo,
													fNullable,
													ulWidth
													);

		ptabdesc->AddColumn(pcoldesc);
	}
	
	if (IMDRelation::EreldistrHash == ereldistrpolicy)
	{
		AddDistributionColumns(ptabdesc, pmdrel, phmiulAttnoColMapping);
	}

	if (pmdrel->FPartitioned())
	{
		const ULONG ulPartCols = pmdrel->UlPartColumns();
		// compute partition columns for table descriptor
		for (ULONG ul = 0; ul < ulPartCols; ul++)
		{
			const IMDColumn *pmdcol = pmdrel->PmdcolPartColumn(ul);
			INT iAttNo = pmdcol->AttrNum();
			ULONG *pulPos = phmiulAttnoColMapping->Find(&iAttNo);
			GPOS_ASSERT(NULL != pulPos);
			ptabdesc->AddPartitionColumn(*pulPos);
		}
	}
	
	// populate key sets
	CTranslatorDXLToExprUtils::AddKeySets(m_memory_pool, ptabdesc, pmdrel, phmululColMapping);
	
	phmiulAttnoPosMapping->Release();
	phmiulAttnoColMapping->Release();
	phmululColMapping->Release();

	return ptabdesc;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::RegisterMDRelationCtas
//
//	@doc:
//		Register the MD relation entry for the given CTAS operator
//
//---------------------------------------------------------------------------
void
CTranslatorDXLToExpr::RegisterMDRelationCtas
	(
	CDXLLogicalCTAS *pdxlopCTAS
	)
{
	GPOS_ASSERT(NULL != pdxlopCTAS);
	
	pdxlopCTAS->MDId()->AddRef();
	
	if (NULL != pdxlopCTAS->PdrgpulDistr())
	{
		pdxlopCTAS->PdrgpulDistr()->AddRef();
	}
	pdxlopCTAS->Pdxlctasopt()->AddRef();
	
	DrgPmdcol *pdrgpmdcol = GPOS_NEW(m_memory_pool) DrgPmdcol(m_memory_pool);
	ColumnDescrDXLArray *pdrgpdxlcd = pdxlopCTAS->GetColumnDescrDXLArray();
	const ULONG length = pdrgpdxlcd->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		CDXLColDescr *pdxlcd = (*pdrgpdxlcd)[ul];
		pdxlcd->MDIdType()->AddRef();
		
		CMDColumn *pmdcol = GPOS_NEW(m_memory_pool) CMDColumn
				(
				GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pdxlcd->MdName()->Pstr()),
				pdxlcd->AttrNum(),
				pdxlcd->MDIdType(),
				pdxlcd->TypeModifier(),
				true, // fNullable,
				pdxlcd->IsDropped(),
				NULL, // pdxlnDefaultValue,
				pdxlcd->Width()
				);
		pdrgpmdcol->Append(pmdcol);
	}
	
	CMDName *pmdnameSchema = NULL;
	if (NULL != pdxlopCTAS->PmdnameSchema())
	{
		pmdnameSchema = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pdxlopCTAS->PmdnameSchema()->Pstr());
	}
	
	IntPtrArray * pdrgpiVarTypeMod = pdxlopCTAS->PdrgpiVarTypeMod();
	pdrgpiVarTypeMod->AddRef();
	CMDRelationCtasGPDB *pmdrel = GPOS_NEW(m_memory_pool) CMDRelationCtasGPDB
			(
			m_memory_pool,
			pdxlopCTAS->MDId(),
			pmdnameSchema,
			GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pdxlopCTAS->MdName()->Pstr()),
			pdxlopCTAS->FTemporary(),
			pdxlopCTAS->FHasOids(),
			pdxlopCTAS->Erelstorage(),
			pdxlopCTAS->Ereldistrpolicy(),
			pdrgpmdcol,
			pdxlopCTAS->PdrgpulDistr(),
			GPOS_NEW(m_memory_pool) ULongPtrArray2D(m_memory_pool), // pdrgpdrgpulKeys,
			pdxlopCTAS->Pdxlctasopt(),
			pdrgpiVarTypeMod
			);
	
	DrgPimdobj *pdrgpmdobj = GPOS_NEW(m_memory_pool) DrgPimdobj(m_memory_pool);
	pdrgpmdobj->Append(pmdrel);
	CMDProviderMemory *pmdp = GPOS_NEW(m_memory_pool) CMDProviderMemory(m_memory_pool, pdrgpmdobj);
	m_pmda->RegisterProvider(pdxlopCTAS->MDId()->Sysid(), pmdp);
	
	// cleanup
	pdrgpmdobj->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PtabdescFromCTAS
//
//	@doc:
//		Construct a table descriptor for a CTAS operator
//
//---------------------------------------------------------------------------
CTableDescriptor *
CTranslatorDXLToExpr::PtabdescFromCTAS
	(
	CDXLLogicalCTAS *pdxlopCTAS
	)
{
	CWStringConst strName(m_memory_pool, pdxlopCTAS->MdName()->Pstr()->GetBuffer());

	IMDId *pmdid = pdxlopCTAS->MDId();

	// get the relation information from the cache
	const IMDRelation *pmdrel = m_pmda->Pmdrel(pmdid);

	// construct mappings for columns that are not dropped
	HMIUl *phmiulAttnoColMapping = GPOS_NEW(m_memory_pool) HMIUl(m_memory_pool);
	HMUlUl *phmululColMapping = GPOS_NEW(m_memory_pool) HMUlUl(m_memory_pool);
	
	const ULONG ulAllColumns = pmdrel->UlColumns();
	ULONG ulPosNonDropped = 0;
	for (ULONG ulPos = 0; ulPos < ulAllColumns; ulPos++)
	{
		const IMDColumn *pmdcol = pmdrel->Pmdcol(ulPos);
		if (pmdcol->IsDropped())
		{
			continue;
		}
		(void) phmiulAttnoColMapping->Insert(GPOS_NEW(m_memory_pool) INT(pmdcol->AttrNum()), GPOS_NEW(m_memory_pool) ULONG(ulPosNonDropped));
		(void) phmululColMapping->Insert(GPOS_NEW(m_memory_pool) ULONG(ulPos), GPOS_NEW(m_memory_pool) ULONG(ulPosNonDropped));

		ulPosNonDropped++;
	}
	
	// get distribution policy
	IMDRelation::Ereldistrpolicy ereldistrpolicy = pmdrel->Ereldistribution();

	// get storage type
	IMDRelation::Erelstoragetype erelstorage = pmdrel->Erelstorage();

	pmdid->AddRef();
	CTableDescriptor *ptabdesc = GPOS_NEW(m_memory_pool) CTableDescriptor
						(
						m_memory_pool,
						pmdid,
						CName(m_memory_pool, &strName),
						pmdrel->FConvertHashToRandom(),
						ereldistrpolicy,
						erelstorage,
						0 // TODO:  - Mar 5, 2014; ulExecuteAsUser
						);

	// populate column information from the dxl table descriptor
	ColumnDescrDXLArray *pdrgpdxlcd = pdxlopCTAS->GetColumnDescrDXLArray();
	const ULONG ulColumns = pdrgpdxlcd->Size();
	for (ULONG ul = 0; ul < ulColumns; ul++)
	{
		BOOL fNullable = false;
		if (ul < pmdrel->UlColumns())
		{
			fNullable = pmdrel->Pmdcol(ul)->FNullable();
		}

		const CDXLColDescr *pdxlcoldesc = (*pdrgpdxlcd)[ul];

		GPOS_ASSERT(pdxlcoldesc->MDIdType()->IsValid());
		const IMDType *pmdtype = m_pmda->Pmdtype(pdxlcoldesc->MDIdType());

		GPOS_ASSERT(NULL != pdxlcoldesc->MdName()->Pstr()->GetBuffer());
		CWStringConst strColName(m_memory_pool, pdxlcoldesc->MdName()->Pstr()->GetBuffer());

		INT iAttNo = pdxlcoldesc->AttrNum();

		const ULONG ulWidth = pdxlcoldesc->Width();
		CColumnDescriptor *pcoldesc = GPOS_NEW(m_memory_pool) CColumnDescriptor
													(
													m_memory_pool,
													pmdtype,
													pdxlcoldesc->TypeModifier(),
													CName(m_memory_pool, &strColName),
													iAttNo,
													fNullable,
													ulWidth
													);

		ptabdesc->AddColumn(pcoldesc);
	}
	
	if (IMDRelation::EreldistrHash == ereldistrpolicy)
	{
		AddDistributionColumns(ptabdesc, pmdrel, phmiulAttnoColMapping);
	}

	GPOS_ASSERT(!pmdrel->FPartitioned());

	phmiulAttnoColMapping->Release();
	phmululColMapping->Release();

	return ptabdesc;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarSubqueryExistential
//
//	@doc:
// 		Translate existential subquery
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarSubqueryExistential
	(
	Edxlopid edxlopid,
	CDXLNode *pdxlnLogicalChild
	)
{
	GPOS_ASSERT(EdxlopScalarSubqueryExists == edxlopid || EdxlopScalarSubqueryNotExists == edxlopid);
	GPOS_ASSERT(NULL != pdxlnLogicalChild);

	CExpression *pexprLogicalChild = Pexpr(pdxlnLogicalChild);
	GPOS_ASSERT(NULL != pexprLogicalChild);

	CScalar *popScalarSubquery = NULL;
	if (EdxlopScalarSubqueryExists == edxlopid)
	{
		popScalarSubquery = GPOS_NEW(m_memory_pool) CScalarSubqueryExists(m_memory_pool);
	}
	else
	{
		popScalarSubquery = GPOS_NEW(m_memory_pool) CScalarSubqueryNotExists(m_memory_pool);
	}
	
	return GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popScalarSubquery, pexprLogicalChild);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprLogicalConstTableGet
//
//	@doc:
// 		Create a logical const table get expression from the corresponding
//		DXL node
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprLogicalConstTableGet
	(
	const CDXLNode *pdxlnConstTable
	)
{
	CDXLLogicalConstTable *pdxlopConstTable = CDXLLogicalConstTable::Cast(pdxlnConstTable->GetOperator());

	const ColumnDescrDXLArray *pdrgpdxlcd = pdxlopConstTable->GetColumnDescrDXLArray();

	// translate the column descriptors
	DrgPcoldesc *pdrgpcoldesc = GPOS_NEW(m_memory_pool) DrgPcoldesc(m_memory_pool);
	const ULONG ulColumns = pdrgpdxlcd->Size();

	for (ULONG ulColIdx = 0; ulColIdx < ulColumns; ulColIdx++)
	{
		CDXLColDescr *pdxlcd = (*pdrgpdxlcd)[ulColIdx];
		const IMDType *pmdtype = m_pmda->Pmdtype(pdxlcd->MDIdType());
		CName name(m_memory_pool, pdxlcd->MdName()->Pstr());

		const ULONG ulWidth = pdxlcd->Width();
		CColumnDescriptor *pcoldesc = GPOS_NEW(m_memory_pool) CColumnDescriptor
														(
														m_memory_pool,
														pmdtype,
														pdxlcd->TypeModifier(),
														name,
														ulColIdx + 1, // iAttno
														true, // FNullable
														ulWidth
														);
		pdrgpcoldesc->Append(pcoldesc);
	}

	// translate values
	DrgPdrgPdatum *pdrgpdrgpdatum = GPOS_NEW(m_memory_pool) DrgPdrgPdatum(m_memory_pool);
	
	const ULONG ulValues = pdxlopConstTable->GetConstTupleCount();
	for (ULONG ul = 0; ul < ulValues; ul++)
	{
		const DXLDatumArray *pdrgpdxldatum = pdxlopConstTable->GetConstTupleDatumArrayAt(ul);
		DrgPdatum *pdrgpdatum = CTranslatorDXLToExprUtils::Pdrgpdatum(m_memory_pool, m_pmda, pdrgpdxldatum);
		pdrgpdrgpdatum->Append(pdrgpdatum);
	}

	// create a logical const table get operator
	CLogicalConstTableGet *popConstTableGet = GPOS_NEW(m_memory_pool) CLogicalConstTableGet
															(
															m_memory_pool,
															pdrgpcoldesc,
															pdrgpdrgpdatum
															);

	// construct the mapping between the DXL ColId and CColRef
	ConstructDXLColId2ColRefMapping(pdxlopConstTable->GetColumnDescrDXLArray(), popConstTableGet->PdrgpcrOutput());

	return GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popConstTableGet);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarSubqueryQuantified
//
//	@doc:
// 		Helper for creating quantified subquery
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarSubqueryQuantified
	(
	Edxlopid edxlopid,
	IMDId *pmdidScalarOp,
	const CWStringConst *pstr,
	ULONG col_id,
	CDXLNode *pdxlnLogicalChild,
	CDXLNode *pdxlnScalarChild
	)
{
	GPOS_ASSERT(EdxlopScalarSubqueryAny == edxlopid || EdxlopScalarSubqueryAll == edxlopid);
	GPOS_ASSERT(NULL != pstr);
	GPOS_ASSERT(NULL != pdxlnLogicalChild);
	GPOS_ASSERT(NULL != pdxlnScalarChild);

	// translate children

	CExpression *pexprLogicalChild = Pexpr(pdxlnLogicalChild);
	CExpression *pexprScalarChild = Pexpr(pdxlnScalarChild);

	// get colref for subquery colid
	const CColRef *pcr = PcrLookup(m_phmulcr, col_id);

	CScalar *popScalarSubquery = NULL;
	if (EdxlopScalarSubqueryAny == edxlopid)
	{
		popScalarSubquery = GPOS_NEW(m_memory_pool) CScalarSubqueryAny
								(
								m_memory_pool,
								pmdidScalarOp,
								GPOS_NEW(m_memory_pool) CWStringConst(m_memory_pool, pstr->GetBuffer()),
								pcr
								);
	}
	else
	{
		popScalarSubquery = GPOS_NEW(m_memory_pool) CScalarSubqueryAll
								(
								m_memory_pool,
								pmdidScalarOp,
								GPOS_NEW(m_memory_pool) CWStringConst(m_memory_pool, pstr->GetBuffer()),
								pcr
								);
	}

	// create a scalar subquery any expression with the relational expression as
	// first child and the scalar expression as second child
	CExpression *pexpr = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popScalarSubquery, pexprLogicalChild, pexprScalarChild);

	return pexpr;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarSubqueryQuantified
//
//	@doc:
// 		Create a quantified subquery from a DXL node
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarSubqueryQuantified
	(
	const CDXLNode *pdxlnSubquery
	)
{
	GPOS_ASSERT(NULL != pdxlnSubquery);
	
	CDXLScalar *dxl_op = dynamic_cast<CDXLScalar *>(pdxlnSubquery->GetOperator());
	GPOS_ASSERT(NULL != dxl_op);

	CDXLScalarSubqueryQuantified *pdxlopSubqueryQuantified = CDXLScalarSubqueryQuantified::Cast(pdxlnSubquery->GetOperator());
	GPOS_ASSERT(NULL != pdxlopSubqueryQuantified);

	IMDId *pmdid = pdxlopSubqueryQuantified->PmdidScalarOp();
	pmdid->AddRef();
	return PexprScalarSubqueryQuantified
		(
		dxl_op->GetDXLOperator(),
		pmdid,
		pdxlopSubqueryQuantified->PmdnameScalarOp()->Pstr(),
		pdxlopSubqueryQuantified->UlColId(),
		(*pdxlnSubquery)[CDXLScalarSubqueryQuantified::EdxlsqquantifiedIndexRelational],
		(*pdxlnSubquery)[CDXLScalarSubqueryQuantified::EdxlsqquantifiedIndexScalar]
		);
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalar
//
//	@doc:
// 		Create a logical select expr from a DXL logical select
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalar
	(
	const CDXLNode *pdxlnOp
	)
{
	GPOS_ASSERT(NULL != pdxlnOp);
	CDXLOperator *dxl_op = pdxlnOp->GetOperator();
	ULONG ulOpId =  (ULONG) dxl_op->GetDXLOperator();

	if (EdxlopScalarSubqueryExists == ulOpId || EdxlopScalarSubqueryNotExists == ulOpId)
	{
		return PexprScalarSubqueryExistential(pdxlnOp->GetOperator()->GetDXLOperator(), (*pdxlnOp)[0]);
	}
	
	PfPexpr pf = m_rgpfTranslators[ulOpId];

	if (NULL == pf)
	{
		GPOS_RAISE(gpopt::ExmaGPOPT, gpopt::ExmiUnsupportedOp, dxl_op->GetOpNameStr()->GetBuffer());
	}
	
	return (this->* pf)(pdxlnOp);	
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprCollapseNot
//
//	@doc:
// 		Collapse a NOT node by looking at its child.
//		Return NULL if it is not collapsible.
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprCollapseNot
	(
	const CDXLNode *pdxlnNotExpr
	)
{
	GPOS_ASSERT(NULL != pdxlnNotExpr);
	GPOS_ASSERT(CTranslatorDXLToExprUtils::FScalarBool(pdxlnNotExpr, Edxlnot));

	CDXLNode *pdxlnNotChild = (*pdxlnNotExpr)[0];

	if (CTranslatorDXLToExprUtils::FScalarBool(pdxlnNotChild, Edxlnot))
	{
		// two cascaded NOT nodes cancel each other
		return Pexpr((*pdxlnNotChild)[0]);
	}

	Edxlopid edxlopid = pdxlnNotChild->GetOperator()->GetDXLOperator();
	if (EdxlopScalarSubqueryExists == edxlopid || EdxlopScalarSubqueryNotExists == edxlopid)
	{
		// NOT followed by EXISTS/NOTEXISTS is translated as NOTEXISTS/EXISTS
		Edxlopid edxlopidNew = (EdxlopScalarSubqueryExists == edxlopid)? EdxlopScalarSubqueryNotExists : EdxlopScalarSubqueryExists;

		return PexprScalarSubqueryExistential(edxlopidNew, (*pdxlnNotChild)[0]);
	}

	if (EdxlopScalarSubqueryAny == edxlopid || EdxlopScalarSubqueryAll == edxlopid)
	{
		// NOT followed by ANY/ALL<op> is translated as ALL/ANY<inverse_op>
		CDXLScalarSubqueryQuantified *pdxlopSubqueryQuantified = CDXLScalarSubqueryQuantified::Cast(pdxlnNotChild->GetOperator());
		Edxlopid edxlopidNew = (EdxlopScalarSubqueryAny == edxlopid)? EdxlopScalarSubqueryAll : EdxlopScalarSubqueryAny;

		// get mdid and name of the inverse of the comparison operator used by quantified subquery
		IMDId *pmdidOp = pdxlopSubqueryQuantified->PmdidScalarOp();
		IMDId *pmdidInverseOp = m_pmda->Pmdscop(pmdidOp)->PmdidOpInverse();

		// if inverse operator cannot be found in metadata, the optimizer won't collapse NOT node
		if (NULL == pmdidInverseOp)
		{
			return NULL;
		}

		const CWStringConst *pstrInverseOp = m_pmda->Pmdscop(pmdidInverseOp)->Mdname().Pstr();
		
		pmdidInverseOp->AddRef();
		return PexprScalarSubqueryQuantified
				(
				edxlopidNew,
				pmdidInverseOp,
				pstrInverseOp,
				pdxlopSubqueryQuantified->UlColId(),
				(*pdxlnNotChild)[CDXLScalarSubqueryQuantified::EdxlsqquantifiedIndexRelational],
				(*pdxlnNotChild)[CDXLScalarSubqueryQuantified::EdxlsqquantifiedIndexScalar]
				);
	}

	// collapsing NOT node failed
	return NULL;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarBoolOp
//
//	@doc:
// 		Create a scalar logical op representation in the optimizer 
//		from a DXL scalar boolean expr
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarBoolOp
	(
	const CDXLNode *pdxlnBoolExpr
	)
{
	GPOS_ASSERT(NULL != pdxlnBoolExpr);

	EdxlBoolExprType edxlbooltype = CDXLScalarBoolExpr::Cast(pdxlnBoolExpr->GetOperator())->EdxlBoolType();

	GPOS_ASSERT( (edxlbooltype == Edxlnot) || (edxlbooltype == Edxlor) || (edxlbooltype == Edxland));
	GPOS_ASSERT_IMP(Edxlnot == edxlbooltype, 1 == pdxlnBoolExpr->Arity());
	
	if (Edxlnot == edxlbooltype)
	{
		// attempt collapsing NOT node
		CExpression *pexprResult = PexprCollapseNot(pdxlnBoolExpr);
		if (NULL != pexprResult)
		{
			return pexprResult;
		}
	}

	CScalarBoolOp::EBoolOperator eboolop = CTranslatorDXLToExprUtils::EBoolOperator(edxlbooltype);

	DrgPexpr *pdrgpexprChildren = PdrgpexprChildren(pdxlnBoolExpr);

	return CUtils::PexprScalarBoolOp(m_memory_pool, eboolop, pdrgpexprChildren);
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarOp
//
//	@doc:
// 		Create a scalar operation from a DXL scalar op expr
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarOp
	(
	const CDXLNode *pdxlnOpExpr
	)
{
	// TODO: Aug 22 2011; In GPDB the opexpr can only have two children. However, in other
	// databases this may not be the case
	GPOS_ASSERT(NULL != pdxlnOpExpr && (1 == pdxlnOpExpr->Arity() || (2 == pdxlnOpExpr->Arity())) );

	CDXLScalarOpExpr *dxl_op = CDXLScalarOpExpr::Cast(pdxlnOpExpr->GetOperator());

	DrgPexpr *pdrgpexprArgs = PdrgpexprChildren(pdxlnOpExpr);

	IMDId *pmdid = dxl_op->MDId();
	pmdid->AddRef();
	
	IMDId *pmdidReturnType = dxl_op->PmdidReturnType(); 
	if (NULL != pmdidReturnType)
	{
		pmdidReturnType->AddRef();
	}
	CScalarOp *pscop = GPOS_NEW(m_memory_pool) CScalarOp
										(
										m_memory_pool,
										pmdid,
										pmdidReturnType,
										GPOS_NEW(m_memory_pool) CWStringConst(m_memory_pool, dxl_op->PstrScalarOpName()->GetBuffer())
										);

	CExpression *pexpr = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, pscop, pdrgpexprArgs);

	return pexpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarIsDistinctFrom
//
//	@doc:
// 		Create a scalar distinct expr from a DXL scalar distinct compare
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarIsDistinctFrom
	(
	const CDXLNode *pdxlnDistCmp
	)
{
	GPOS_ASSERT(NULL != pdxlnDistCmp && 2 == pdxlnDistCmp->Arity());
	CDXLScalarDistinctComp *pdxlopDistCmp = CDXLScalarDistinctComp::Cast(pdxlnDistCmp->GetOperator());
	// get children
	CDXLNode *pdxlnLeft = (*pdxlnDistCmp)[0];
	CDXLNode *pdxlnRight = (*pdxlnDistCmp)[1];

	// translate left and right children
	CExpression *pexprLeft = Pexpr(pdxlnLeft);
	CExpression *pexprRight = Pexpr(pdxlnRight);
	
	IMDId *pmdidOp = pdxlopDistCmp->MDId();
	pmdidOp->AddRef();
	const IMDScalarOp *pmdscop = m_pmda->Pmdscop(pmdidOp);

	CScalarIsDistinctFrom *popScIDF = GPOS_NEW(m_memory_pool) CScalarIsDistinctFrom
													(
													m_memory_pool,
													pmdidOp,
													GPOS_NEW(m_memory_pool) CWStringConst(m_memory_pool, (pmdscop->Mdname().Pstr())->GetBuffer())
													);

	CExpression *pexpr = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popScIDF, pexprLeft, pexprRight);

	return pexpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarNullIf
//
//	@doc:
// 		Create a scalar nullif expr from a DXL scalar nullif
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarNullIf
	(
	const CDXLNode *pdxlnNullIf
	)
{
	GPOS_ASSERT(NULL != pdxlnNullIf && 2 == pdxlnNullIf->Arity());
	CDXLScalarNullIf *dxl_op = CDXLScalarNullIf::Cast(pdxlnNullIf->GetOperator());

	// translate children
	CExpression *pexprLeft = Pexpr((*pdxlnNullIf)[0]);
	CExpression *pexprRight = Pexpr((*pdxlnNullIf)[1]);

	IMDId *pmdidOp = dxl_op->PmdidOp();
	pmdidOp->AddRef();

	IMDId *mdid_type = dxl_op->MDIdType();
	mdid_type->AddRef();

	return GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, GPOS_NEW(m_memory_pool) CScalarNullIf(m_memory_pool, pmdidOp, mdid_type), pexprLeft, pexprRight);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarCmp
//
//	@doc:
// 		Create a scalar compare expr from a DXL scalar compare
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarCmp
	(
	const CDXLNode *pdxlnCmp
	)
{
	GPOS_ASSERT(NULL != pdxlnCmp && 2 == pdxlnCmp->Arity());
	CDXLScalarComp *pdxlopComp = CDXLScalarComp::Cast(pdxlnCmp->GetOperator());
	// get children
	CDXLNode *pdxlnLeft = (*pdxlnCmp)[0];
	CDXLNode *pdxlnRight = (*pdxlnCmp)[1];

	// translate left and right children
	CExpression *pexprLeft = Pexpr(pdxlnLeft);
	CExpression *pexprRight = Pexpr(pdxlnRight);

	IMDId *pmdid = pdxlopComp->MDId();
	pmdid->AddRef();

	CScalarCmp *popScCmp = GPOS_NEW(m_memory_pool) CScalarCmp
										(
										m_memory_pool,
										pmdid,
										GPOS_NEW(m_memory_pool) CWStringConst(m_memory_pool, pdxlopComp->PstrCmpOpName()->GetBuffer()),
										CUtils::Ecmpt(pmdid)
										);

	GPOS_ASSERT(NULL != popScCmp);
	GPOS_ASSERT(NULL != popScCmp->Pstr());
	GPOS_ASSERT(NULL != popScCmp->Pstr()->GetBuffer());
	
	CExpression *pexpr = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popScCmp, pexprLeft, pexprRight);

	return pexpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarFunc
//
//	@doc:
// 		Create a scalar func operator expression from a DXL func expr
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarFunc
	(
	const CDXLNode *pdxlnFunc
	)
{
	GPOS_ASSERT(NULL != pdxlnFunc);

	const ULONG ulLen = pdxlnFunc->Arity();

	CDXLScalarFuncExpr *pdxlopFuncExpr = CDXLScalarFuncExpr::Cast(pdxlnFunc->GetOperator());
	
	COperator *pop = NULL;

	IMDId *pmdidFunc = pdxlopFuncExpr->FuncMdId();
	pmdidFunc->AddRef();
	const IMDFunction *pmdfunc = m_pmda->Pmdfunc(pmdidFunc);

	IMDId *pmdidRetType = pdxlopFuncExpr->ReturnTypeMdId();
	pmdidRetType->AddRef();

	DrgPexpr *pdrgpexprArgs = NULL;
	IMDId *pmdidInput = NULL;
	if (0 < ulLen)
	{
		// translate function arguments
		pdrgpexprArgs = PdrgpexprChildren(pdxlnFunc);

		if (1 == ulLen)
		{
			CExpression *pexprFirstChild = (*pdrgpexprArgs)[0];
			COperator *popFirstChild = pexprFirstChild->Pop();
			if (popFirstChild->FScalar())
			{
				pmdidInput = CScalar::PopConvert(popFirstChild)->MDIdType();
			}
		}
	}

	if (CTranslatorDXLToExprUtils::FCastFunc(m_pmda, pdxlnFunc, pmdidInput))
	{
		const IMDCast *pmdcast = m_pmda->Pmdcast(pmdidInput, pmdidRetType);

		if (pmdcast->EmdPathType() == IMDCast::EmdtArrayCoerce)
		{
			CMDArrayCoerceCastGPDB *parrayCoerceCast = (CMDArrayCoerceCastGPDB *) pmdcast;
			pop = GPOS_NEW(m_memory_pool) CScalarArrayCoerceExpr
					(
					m_memory_pool,
					parrayCoerceCast->PmdidCastFunc(),
					pmdidRetType,
					parrayCoerceCast->TypeModifier(),
					parrayCoerceCast->FIsExplicit(),
					(COperator::ECoercionForm) parrayCoerceCast->Ecf(),
					parrayCoerceCast->ILoc()
					);
		}
		else
		{
			pop = GPOS_NEW(m_memory_pool) CScalarCast
					(
					m_memory_pool,
					pmdidRetType,
					pmdidFunc,
					pmdcast->FBinaryCoercible()
					);
		}
	}
	else
	{
		pop = GPOS_NEW(m_memory_pool) CScalarFunc
				(
				m_memory_pool,
				pmdidFunc,
				pmdidRetType,
				pdxlopFuncExpr->TypeModifier(),
				GPOS_NEW(m_memory_pool) CWStringConst(m_memory_pool, (pmdfunc->Mdname().Pstr())->GetBuffer())
				);
	}
	
	CExpression *pexprFunc = NULL;
	if (NULL != pdrgpexprArgs)
	{
		pexprFunc = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, pop, pdrgpexprArgs);
	}
	else
	{
		pexprFunc = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, pop);
	}
	
	return pexprFunc;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprWindowFunc
//
//	@doc:
// 		Create a scalar window function expression from a DXL window ref
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprWindowFunc
	(
	const CDXLNode *pdxlnWindowRef
	)
{
	CDXLScalarWindowRef *pdxlopWinref = CDXLScalarWindowRef::Cast(pdxlnWindowRef->GetOperator());

	IMDId *pmdidFunc = pdxlopWinref->FuncMdId();
	pmdidFunc->AddRef();

	CWStringConst *pstrName = GPOS_NEW(m_memory_pool) CWStringConst(m_memory_pool, CMDAccessorUtils::PstrWindowFuncName(m_pmda, pmdidFunc)->GetBuffer());

	CScalarWindowFunc::EWinStage ews = Ews(pdxlopWinref->Edxlwinstage());

	IMDId *pmdidRetType = pdxlopWinref->ReturnTypeMdId();
	pmdidRetType->AddRef();
	
	GPOS_ASSERT(NULL != pstrName);
	CScalarWindowFunc *popWindowFunc = GPOS_NEW(m_memory_pool) CScalarWindowFunc(m_memory_pool, pmdidFunc, pmdidRetType, pstrName, ews, pdxlopWinref->FDistinct(), pdxlopWinref->FStarArg(), pdxlopWinref->FSimpleAgg());

	CExpression *pexprWindowFunc = NULL;
	if (0 < pdxlnWindowRef->Arity())
	{
		DrgPexpr *pdrgpexprArgs = PdrgpexprChildren(pdxlnWindowRef);

		pexprWindowFunc= GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popWindowFunc, pdrgpexprArgs);
	}
	else
	{
		// window function has no arguments
		pexprWindowFunc = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popWindowFunc);
	}

	return pexprWindowFunc;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::Ews
//
//	@doc:
//		Translate the DXL representation of the window stage
//
//---------------------------------------------------------------------------
CScalarWindowFunc::EWinStage
CTranslatorDXLToExpr::Ews
	(
	EdxlWinStage edxlws
	)
	const
{
	ULONG rgrgulMapping[][2] =
	{
		{EdxlwinstageImmediate, CScalarWindowFunc::EwsImmediate},
		{EdxlwinstagePreliminary, CScalarWindowFunc::EwsPreliminary},
		{EdxlwinstageRowKey, CScalarWindowFunc::EwsRowKey}
	};
#ifdef GPOS_DEBUG
	const ULONG ulArity = GPOS_ARRAY_SIZE(rgrgulMapping);
	GPOS_ASSERT(ulArity > (ULONG) edxlws  && "Invalid window stage");
#endif
	CScalarWindowFunc::EWinStage ews= (CScalarWindowFunc::EWinStage)rgrgulMapping[(ULONG) edxlws][1];
	
	return ews;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarCoalesce
//
//	@doc:
// 		Create a scalar coalesce expression from a DXL scalar coalesce
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarCoalesce
	(
	const CDXLNode *pdxlnCoalesce
	)
{
	GPOS_ASSERT(NULL != pdxlnCoalesce);
	GPOS_ASSERT(0 < pdxlnCoalesce->Arity());

	CDXLScalarCoalesce *dxl_op = CDXLScalarCoalesce::Cast(pdxlnCoalesce->GetOperator());

	DrgPexpr *pdrgpexprChildren = PdrgpexprChildren(pdxlnCoalesce);

	IMDId *pmdid = dxl_op->MDIdType();
	pmdid->AddRef();

	return GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, GPOS_NEW(m_memory_pool) CScalarCoalesce(m_memory_pool, pmdid), pdrgpexprChildren);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarMinMax
//
//	@doc:
// 		Create a scalar MinMax expression from a DXL scalar MinMax
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarMinMax
	(
	const CDXLNode *pdxlnMinMax
	)
{
	GPOS_ASSERT(NULL != pdxlnMinMax);
	GPOS_ASSERT(0 < pdxlnMinMax->Arity());

	CDXLScalarMinMax *dxl_op = CDXLScalarMinMax::Cast(pdxlnMinMax->GetOperator());

	DrgPexpr *pdrgpexprChildren = PdrgpexprChildren(pdxlnMinMax);

	CDXLScalarMinMax::EdxlMinMaxType emmt = dxl_op->Emmt();
	GPOS_ASSERT(CDXLScalarMinMax::EmmtMin == emmt || CDXLScalarMinMax::EmmtMax == emmt);

	CScalarMinMax::EScalarMinMaxType esmmt = CScalarMinMax::EsmmtMin;
	if (CDXLScalarMinMax::EmmtMax == emmt)
	{
		esmmt = CScalarMinMax::EsmmtMax;
	}

	IMDId *pmdid = dxl_op->MDIdType();
	pmdid->AddRef();

	return GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, GPOS_NEW(m_memory_pool) CScalarMinMax(m_memory_pool, pmdid, esmmt), pdrgpexprChildren);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprAggFunc
//
//	@doc:
// 		Create a scalar agg func operator expression from a DXL aggref node
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprAggFunc
	(
	const CDXLNode *pdxlnAggref
	)
{
	CDXLScalarAggref *dxl_op = CDXLScalarAggref::Cast(pdxlnAggref->GetOperator());
	
	IMDId *pmdidAggFunc = dxl_op->PmdidAgg();
	pmdidAggFunc->AddRef();
	const IMDAggregate *pmdagg = m_pmda->Pmdagg(pmdidAggFunc);
	
	EAggfuncStage eaggfuncstage = EaggfuncstageLocal;
	if (EdxlaggstagePartial != dxl_op->Edxlaggstage())
	{
		eaggfuncstage = EaggfuncstageGlobal;
	}
	BOOL fSplit = (EdxlaggstageNormal != dxl_op->Edxlaggstage());

	IMDId *pmdidResolvedReturnType = dxl_op->PmdidResolvedRetType();
	if (NULL != pmdidResolvedReturnType)
	{
		// use the resolved type provided in DXL
		pmdidResolvedReturnType->AddRef();
	}

	CScalarAggFunc *popScAggFunc =
			CUtils::PopAggFunc
				(
				m_memory_pool,
				pmdidAggFunc,
				GPOS_NEW(m_memory_pool) CWStringConst(m_memory_pool, (pmdagg->Mdname().Pstr())->GetBuffer()),
				dxl_op->FDistinct(),
				eaggfuncstage,
				fSplit,
				pmdidResolvedReturnType
				);

	CExpression *pexprAggFunc = NULL;
	
	if (0 < pdxlnAggref->Arity())
	{
		// translate function arguments
		DrgPexpr *pdrgpexprArgs = PdrgpexprChildren(pdxlnAggref);

		// check if the arguments have set returning functions, if so raise an exception
		for (ULONG ul = 0; ul < pdrgpexprArgs->Size(); ul++)
		{
			CExpression *pexprAggrefChild = (*pdrgpexprArgs)[ul];
			CDrvdPropScalar *pdpScalar = CDrvdPropScalar::Pdpscalar(pexprAggrefChild->PdpDerive());

			if (pdpScalar->FHasNonScalarFunction())
			{
				GPOS_RAISE(gpopt::ExmaGPOPT, gpopt::ExmiUnsupportedOp, GPOS_WSZ_LIT("Aggregate function with set returning attributes"));
			}
		}

		pexprAggFunc= GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popScAggFunc, pdrgpexprArgs);
	}
	else
	{
		// aggregate function has no arguments
		pexprAggFunc = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popScAggFunc);
	}
	
	return pexprAggFunc;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprArray
//
//	@doc:
// 		Translate a scalar array
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprArray
	(
	const CDXLNode *pdxln
	)
{
	CDXLScalarArray *dxl_op = CDXLScalarArray::Cast(pdxln->GetOperator());
	
	IMDId *pmdidElem = dxl_op->PmdidElem();
	pmdidElem->AddRef();

	IMDId *pmdidArray = dxl_op->PmdidArray();
	pmdidArray->AddRef();

	CScalarArray *popArray = GPOS_NEW(m_memory_pool) CScalarArray(m_memory_pool, pmdidElem, pmdidArray, dxl_op->FMultiDimensional());
	
	DrgPexpr *pdrgpexprChildren = PdrgpexprChildren(pdxln);

	CExpression *pexprArray = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popArray, pdrgpexprChildren);

	CExpression *pexprCollapsedArray = CUtils::PexprCollapseConstArray(m_memory_pool, pexprArray);

	pexprArray->Release();

	return pexprCollapsedArray;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprArrayRef
//
//	@doc:
// 		Translate a scalar arrayref
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprArrayRef
	(
	const CDXLNode *pdxln
	)
{
	CDXLScalarArrayRef *dxl_op = CDXLScalarArrayRef::Cast(pdxln->GetOperator());

	IMDId *pmdidElem = dxl_op->PmdidElem();
	pmdidElem->AddRef();

	IMDId *pmdidArray = dxl_op->PmdidArray();
	pmdidArray->AddRef();

	IMDId *pmdidReturn = dxl_op->PmdidReturn();
	pmdidReturn->AddRef();

	CScalarArrayRef *popArrayref = GPOS_NEW(m_memory_pool) CScalarArrayRef(m_memory_pool, pmdidElem, dxl_op->TypeModifier(), pmdidArray, pmdidReturn);

	DrgPexpr *pdrgpexprChildren = PdrgpexprChildren(pdxln);

	return GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popArrayref, pdrgpexprChildren);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprArrayRefIndexList
//
//	@doc:
// 		Translate a scalar arrayref index list
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprArrayRefIndexList
	(
	const CDXLNode *pdxln
	)
{
	CDXLScalarArrayRefIndexList *dxl_op = CDXLScalarArrayRefIndexList::Cast(pdxln->GetOperator());
	CScalarArrayRefIndexList *popIndexlist = GPOS_NEW(m_memory_pool) CScalarArrayRefIndexList(m_memory_pool, Eilt(dxl_op->Eilb()));

	DrgPexpr *pdrgpexprChildren = PdrgpexprChildren(pdxln);

	return GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popIndexlist, pdrgpexprChildren);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::Eilt
//
//	@doc:
// 		Translate the arrayref index list type
//
//---------------------------------------------------------------------------
CScalarArrayRefIndexList::EIndexListType
CTranslatorDXLToExpr::Eilt
	(
	const CDXLScalarArrayRefIndexList::EIndexListBound eilb
	)
{
	switch (eilb)
	{
		case CDXLScalarArrayRefIndexList::EilbLower:
			return CScalarArrayRefIndexList::EiltLower;

		case CDXLScalarArrayRefIndexList::EilbUpper:
			return CScalarArrayRefIndexList::EiltUpper;

		default:
			GPOS_RAISE(gpopt::ExmaGPOPT, gpopt::ExmiUnsupportedOp, GPOS_WSZ_LIT("Invalid arrayref index type"));
			return CScalarArrayRefIndexList::EiltSentinel;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprArrayCmp
//
//	@doc:
// 		Translate a scalar array compare
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprArrayCmp
	(
	const CDXLNode *pdxln
	)
{
	CDXLScalarArrayComp *dxl_op = CDXLScalarArrayComp::Cast(pdxln->GetOperator());
	
	IMDId *pmdidOp = dxl_op->MDId();
	pmdidOp->AddRef();

	const CWStringConst *pstrOpName = dxl_op->PstrCmpOpName();
	
	EdxlArrayCompType edxlarrcmp = dxl_op->Edxlarraycomptype();
	CScalarArrayCmp::EArrCmpType earrcmpt = CScalarArrayCmp::EarrcmpSentinel;
	if (Edxlarraycomptypeall == edxlarrcmp)
	{
		earrcmpt = CScalarArrayCmp::EarrcmpAll;
	}
	else
	{
		GPOS_ASSERT(Edxlarraycomptypeany == edxlarrcmp);
		earrcmpt = CScalarArrayCmp::EarrcmpAny;
	}
	
	CScalarArrayCmp *popArrayCmp = GPOS_NEW(m_memory_pool) CScalarArrayCmp(m_memory_pool, pmdidOp, GPOS_NEW(m_memory_pool) CWStringConst(m_memory_pool, pstrOpName->GetBuffer()), earrcmpt);
	
	DrgPexpr *pdrgpexprChildren = PdrgpexprChildren(pdxln);

	return GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popArrayCmp, pdrgpexprChildren);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarIdent
//
//	@doc:
// 		Create a scalar ident expr from a DXL scalar ident
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarIdent
	(
	const CDXLNode *pdxlnIdent
	)
{
	// get dxl scalar identifier
	CDXLScalarIdent *dxl_op = CDXLScalarIdent::Cast(pdxlnIdent->GetOperator());

	// get the dxl column reference
	const CDXLColRef *pdxlcr = dxl_op->Pdxlcr();
	const ULONG col_id = pdxlcr->Id();

	// get its column reference from the hash map
	const CColRef *pcr =  PcrLookup(m_phmulcr, col_id);
	CExpression *pexpr = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, GPOS_NEW(m_memory_pool) CScalarIdent(m_memory_pool, pcr));

	return pexpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PdrgpexprChildren
//
//	@doc:
// 		Translate children of a DXL node
//
//---------------------------------------------------------------------------
DrgPexpr *
CTranslatorDXLToExpr::PdrgpexprChildren
	(
	const CDXLNode *pdxln
	)
{
	DrgPexpr *pdrgpexpr = GPOS_NEW(m_memory_pool) DrgPexpr(m_memory_pool);

	const ULONG ulArity = pdxln->Arity();
	for (ULONG ul = 0; ul < ulArity; ul++)
	{
		// get next child and translate it
		CDXLNode *child_dxlnode = (*pdxln)[ul];

		CExpression *pexprChild = Pexpr(child_dxlnode);
		pdrgpexpr->Append(pexprChild);
	}

	return pdrgpexpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarIf
//
//	@doc:
// 		Create a scalar if expression from a DXL scalar if statement
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarIf
	(
	const CDXLNode *pdxlnIfStmt
	)
{
	GPOS_ASSERT(NULL != pdxlnIfStmt);
	GPOS_ASSERT(3 == pdxlnIfStmt->Arity());

	CDXLScalarIfStmt *dxl_op = CDXLScalarIfStmt::Cast(pdxlnIfStmt->GetOperator());

	DrgPexpr *pdrgpexprChildren = PdrgpexprChildren(pdxlnIfStmt);

	IMDId *pmdid = dxl_op->PmdidResultType();
	pmdid->AddRef();
	CScalarIf *popScIf = GPOS_NEW(m_memory_pool) CScalarIf(m_memory_pool, pmdid);
	CExpression *pexpr = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popScIf, pdrgpexprChildren);

	return pexpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarSwitch
//
//	@doc:
// 		Create a scalar switch expression from a DXL scalar switch
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarSwitch
	(
	const CDXLNode *pdxlnSwitch
	)
{
	GPOS_ASSERT(NULL != pdxlnSwitch);
	GPOS_ASSERT(1 < pdxlnSwitch->Arity());

	CDXLScalarSwitch *dxl_op = CDXLScalarSwitch::Cast(pdxlnSwitch->GetOperator());

	DrgPexpr *pdrgpexprChildren = PdrgpexprChildren(pdxlnSwitch);

	IMDId *pmdid = dxl_op->MDIdType();
	pmdid->AddRef();
	CScalarSwitch *pop = GPOS_NEW(m_memory_pool) CScalarSwitch(m_memory_pool, pmdid);

	return GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, pop, pdrgpexprChildren);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarSwitchCase
//
//	@doc:
// 		Create a scalar switchcase expression from a DXL scalar switchcase
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarSwitchCase
	(
	const CDXLNode *pdxlnSwitchCase
	)
{
	GPOS_ASSERT(NULL != pdxlnSwitchCase);

	GPOS_ASSERT(2 == pdxlnSwitchCase->Arity());

	DrgPexpr *pdrgpexprChildren = PdrgpexprChildren(pdxlnSwitchCase);

	return GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, GPOS_NEW(m_memory_pool) CScalarSwitchCase(m_memory_pool), pdrgpexprChildren);
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarCaseTest
//
//	@doc:
// 		Create a scalar case test expression from a DXL scalar case test
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarCaseTest
	(
	const CDXLNode *pdxlnCaseTest
	)
{
	GPOS_ASSERT(NULL != pdxlnCaseTest);

	CDXLScalarCaseTest *dxl_op =
			CDXLScalarCaseTest::Cast(pdxlnCaseTest->GetOperator());

	IMDId *pmdid = dxl_op->MDIdType();
	pmdid->AddRef();
	CScalarCaseTest *pop = GPOS_NEW(m_memory_pool) CScalarCaseTest(m_memory_pool, pmdid);

	return GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, pop);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarNullTest
//
//	@doc:
// 		Create a scalar null test expr from a DXL scalar null test
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarNullTest
	(
	const CDXLNode *pdxlnNullTest
	)
{
	// get dxl scalar null test
	CDXLScalarNullTest *dxl_op =
			CDXLScalarNullTest::Cast(pdxlnNullTest->GetOperator());

	GPOS_ASSERT(NULL != dxl_op);

	// translate child expression
	GPOS_ASSERT(1 == pdxlnNullTest->Arity());
	
	CDXLNode *child_dxlnode = (*pdxlnNullTest)[0];
	CExpression *pexprChild = Pexpr(child_dxlnode);
	
	CExpression *pexpr = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, GPOS_NEW(m_memory_pool) CScalarNullTest(m_memory_pool), pexprChild);
	
	if (!dxl_op->FIsNullTest())
	{
		// IS NOT NULL test: add a not expression on top
		pexpr = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, GPOS_NEW(m_memory_pool) CScalarBoolOp(m_memory_pool, CScalarBoolOp::EboolopNot), pexpr);
	}

	return pexpr;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarBooleanTest
//
//	@doc:
// 		Create a scalar boolean test expr from a DXL scalar boolean test
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarBooleanTest
	(
	const CDXLNode *pdxlnScBoolTest
	)
{
	const ULONG rgulBoolTestMapping[][2] =
	{
		{EdxlbooleantestIsTrue, CScalarBooleanTest::EbtIsTrue},
		{EdxlbooleantestIsNotTrue, CScalarBooleanTest::EbtIsNotTrue},
		{EdxlbooleantestIsFalse, CScalarBooleanTest::EbtIsFalse},
		{EdxlbooleantestIsNotFalse, CScalarBooleanTest::EbtIsNotFalse},
		{EdxlbooleantestIsUnknown, CScalarBooleanTest::EbtIsUnknown},
		{EdxlbooleantestIsNotUnknown, CScalarBooleanTest::EbtIsNotUnknown},
	};

	// get dxl scalar null test
	CDXLScalarBooleanTest *dxl_op =
			CDXLScalarBooleanTest::Cast(pdxlnScBoolTest->GetOperator());

	GPOS_ASSERT(NULL != dxl_op);

	// translate child expression
	GPOS_ASSERT(1 == pdxlnScBoolTest->Arity());

	CDXLNode *child_dxlnode = (*pdxlnScBoolTest)[0];
	CExpression *pexprChild = Pexpr(child_dxlnode);

	CScalarBooleanTest::EBoolTest ebt = (CScalarBooleanTest::EBoolTest) (rgulBoolTestMapping[dxl_op->EdxlBoolType()][1]);
	
	return GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, GPOS_NEW(m_memory_pool) CScalarBooleanTest(m_memory_pool, ebt), pexprChild);
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarCast
//
//	@doc:
// 		Create a scalar relabel type from a DXL scalar cast
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarCast
	(
	const CDXLNode *pdxlnCast
	)
{
	// get dxl scalar relabel type
	CDXLScalarCast *dxl_op = CDXLScalarCast::Cast(pdxlnCast->GetOperator());
	GPOS_ASSERT(NULL != dxl_op);

	// translate child expression
	GPOS_ASSERT(1 == pdxlnCast->Arity());
	CDXLNode *child_dxlnode = (*pdxlnCast)[0];
	CExpression *pexprChild = Pexpr(child_dxlnode);

	IMDId *mdid_type = dxl_op->MDIdType();
	IMDId *pmdidFunc = dxl_op->FuncMdId();
	mdid_type->AddRef();
	pmdidFunc->AddRef();
	
	COperator *popChild = pexprChild->Pop();
	IMDId *pmdidInput = CScalar::PopConvert(popChild)->MDIdType();
	const IMDCast *pmdcast = m_pmda->Pmdcast(pmdidInput, mdid_type);
	BOOL fRelabel = pmdcast->FBinaryCoercible();

	CExpression *pexpr;
	
	if (pmdcast->EmdPathType() == IMDCast::EmdtArrayCoerce)
	{
		CMDArrayCoerceCastGPDB *parrayCoerceCast = (CMDArrayCoerceCastGPDB *) pmdcast;
		pexpr = GPOS_NEW(m_memory_pool) CExpression
									(
									m_memory_pool,
									GPOS_NEW(m_memory_pool) CScalarArrayCoerceExpr
														(
														m_memory_pool,
														parrayCoerceCast->PmdidCastFunc(),
														mdid_type,
														parrayCoerceCast->TypeModifier(),
														parrayCoerceCast->FIsExplicit(),
														(COperator::ECoercionForm) parrayCoerceCast->Ecf(),
														parrayCoerceCast->ILoc()
														),
									pexprChild
									);
	}
	else
	{
		
		pexpr= GPOS_NEW(m_memory_pool) CExpression
									(
									m_memory_pool,
									GPOS_NEW(m_memory_pool) CScalarCast(m_memory_pool, mdid_type, pmdidFunc, fRelabel),
									pexprChild
									);
	}

	return pexpr;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarCoerceToDomain
//
//	@doc:
// 		Create a scalar CoerceToDomain from a DXL scalar CoerceToDomain
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarCoerceToDomain
	(
	const CDXLNode *pdxlnCoerce
	)
{
	// get dxl scalar coerce operator
	CDXLScalarCoerceToDomain *dxl_op = CDXLScalarCoerceToDomain::Cast(pdxlnCoerce->GetOperator());
	GPOS_ASSERT(NULL != dxl_op);

	// translate child expression
	GPOS_ASSERT(1 == pdxlnCoerce->Arity());
	CDXLNode *child_dxlnode = (*pdxlnCoerce)[0];
	CExpression *pexprChild = Pexpr(child_dxlnode);

	IMDId *mdid_type = dxl_op->PmdidResultType();
	mdid_type->AddRef();

	EdxlCoercionForm edxlcf = dxl_op->Edxlcf();

	return GPOS_NEW(m_memory_pool) CExpression
				(
				m_memory_pool,
				GPOS_NEW(m_memory_pool) CScalarCoerceToDomain
						(
						m_memory_pool,
						mdid_type,
						dxl_op->TypeModifier(),
						(COperator::ECoercionForm) edxlcf, // map Coercion Form directly based on position in enum
						dxl_op->ILoc()
						),
				pexprChild
				);
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarCoerceViaIO
//
//	@doc:
// 		Create a scalar CoerceViaIO from a DXL scalar CoerceViaIO
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarCoerceViaIO
	(
	const CDXLNode *pdxlnCoerce
	)
{
	// get dxl scalar coerce operator
	CDXLScalarCoerceViaIO *dxl_op = CDXLScalarCoerceViaIO::Cast(pdxlnCoerce->GetOperator());
	GPOS_ASSERT(NULL != dxl_op);

	// translate child expression
	GPOS_ASSERT(1 == pdxlnCoerce->Arity());
	CDXLNode *child_dxlnode = (*pdxlnCoerce)[0];
	CExpression *pexprChild = Pexpr(child_dxlnode);

	IMDId *mdid_type = dxl_op->PmdidResultType();
	mdid_type->AddRef();

	EdxlCoercionForm edxlcf = dxl_op->Edxlcf();

	return GPOS_NEW(m_memory_pool) CExpression
				(
				m_memory_pool,
				GPOS_NEW(m_memory_pool) CScalarCoerceViaIO
						(
						m_memory_pool,
						mdid_type,
						dxl_op->TypeModifier(),
						(COperator::ECoercionForm) edxlcf, // map Coercion Form directly based on position in enum
						dxl_op->ILoc()
						),
				pexprChild
				);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarArrayCoerceExpr
//
//	@doc:
// 		Create a scalar array coerce expr from a DXL scalar array coerce expr
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarArrayCoerceExpr
	(
	const CDXLNode *pdxlnArrayCoerceExpr
	)
{
	GPOS_ASSERT(NULL != pdxlnArrayCoerceExpr);

	CDXLScalarArrayCoerceExpr *dxl_op = CDXLScalarArrayCoerceExpr::Cast(pdxlnArrayCoerceExpr->GetOperator());

	GPOS_ASSERT(1 == pdxlnArrayCoerceExpr->Arity());
	CDXLNode *child_dxlnode = (*pdxlnArrayCoerceExpr)[0];
	CExpression *pexprChild = Pexpr(child_dxlnode);

	IMDId *pmdidElementFunc = dxl_op->PmdidElementFunc();
	pmdidElementFunc->AddRef();

	IMDId *pmdidResultType = dxl_op->PmdidResultType();
	pmdidResultType->AddRef();

	EdxlCoercionForm edxlcf = dxl_op->Edxlcf();

	return GPOS_NEW(m_memory_pool) CExpression
				(
				m_memory_pool,
				GPOS_NEW(m_memory_pool) CScalarArrayCoerceExpr
						(
						m_memory_pool,
						pmdidElementFunc,
						pmdidResultType,
						dxl_op->TypeModifier(),
						dxl_op->FIsExplicit(),
						(COperator::ECoercionForm) edxlcf, // map Coercion Form directly based on position in enum
						dxl_op->ILoc()
						),
				pexprChild
				);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarConst
//
//	@doc:
// 		Create a scalar const expr from a DXL scalar constant value
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarConst
	(
	const CDXLNode *pdxlnConstVal
	)
{
	GPOS_ASSERT(NULL != pdxlnConstVal);

	// translate the dxl scalar const value
	CDXLScalarConstValue *dxl_op =
			CDXLScalarConstValue::Cast(pdxlnConstVal->GetOperator());
	CScalarConst *popConst = CTranslatorDXLToExprUtils::PopConst(m_memory_pool, m_pmda, dxl_op);
	
	return GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popConst);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarSubquery
//
//	@doc:
// 		Create a scalar subquery expr from a DXL scalar subquery node
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarSubquery
	(
	const CDXLNode *pdxlnSubquery
	)
{
	GPOS_ASSERT(NULL != pdxlnSubquery);
	CDXLScalarSubquery *pdxlopSubquery =
			CDXLScalarSubquery::Cast(pdxlnSubquery->GetOperator());

	// translate child
	CDXLNode *child_dxlnode = (*pdxlnSubquery)[0];
	CExpression *pexprChild = Pexpr(child_dxlnode);
	
	// get subquery colref for colid
	ULONG col_id = pdxlopSubquery->UlColId();
	const CColRef *pcr = PcrLookup(m_phmulcr, col_id);
		
	CScalarSubquery *popScalarSubquery = GPOS_NEW(m_memory_pool) CScalarSubquery(m_memory_pool, pcr, false /*fGeneratedByExist*/, false /*fGeneratedByQuantified*/);
	GPOS_ASSERT(NULL != popScalarSubquery);
	CExpression *pexpr = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, popScalarSubquery, pexprChild);

	return pexpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarProjElem
//
//	@doc:
// 		Create a scalar project elem expression from a DXL scalar project elem node
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarProjElem
	(
	const CDXLNode *pdxlnPrEl
	)
{
	GPOS_ASSERT(NULL != pdxlnPrEl && EdxlopScalarProjectElem == pdxlnPrEl->GetOperator()->GetDXLOperator());
	GPOS_ASSERT(1 == pdxlnPrEl->Arity());
	
	CDXLScalarProjElem *pdxlopPrEl = CDXLScalarProjElem::Cast(pdxlnPrEl->GetOperator());
	
	// translate child
	CDXLNode *child_dxlnode = (*pdxlnPrEl)[0];
	CExpression *pexprChild = Pexpr(child_dxlnode);

	CScalar *popScalar = CScalar::PopConvert(pexprChild->Pop());

	IMDId *pmdid = popScalar->MDIdType();
	const IMDType *pmdtype = m_pmda->Pmdtype(pmdid);

	CName name(pdxlopPrEl->PmdnameAlias()->Pstr());
	
	// generate a new column reference
	CColRef *pcr = m_pcf->PcrCreate(pmdtype, popScalar->TypeModifier(), name);
	
	// store colid -> colref mapping
#ifdef GPOS_DEBUG
	BOOL fInserted =
#endif
	m_phmulcr->Insert(GPOS_NEW(m_memory_pool) ULONG(pdxlopPrEl->UlId()), pcr);
	
	GPOS_ASSERT(fInserted);
	
	CExpression *pexprProjElem = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, GPOS_NEW(m_memory_pool) CScalarProjectElement(m_memory_pool, pcr), pexprChild);
	return pexprProjElem;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::PexprScalarProjList
//
//	@doc:
// 		Create a scalar project list expression from a DXL scalar project list node
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExpr::PexprScalarProjList
	(
	const CDXLNode *pdxlnPrL
	)
{
	GPOS_ASSERT(NULL != pdxlnPrL &&  EdxlopScalarProjectList == pdxlnPrL->GetOperator()->GetDXLOperator());
	
	// translate project elements
	CExpression *pexprProjList = NULL;
	
	if (0 == pdxlnPrL->Arity())
	{
		pexprProjList = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, GPOS_NEW(m_memory_pool) CScalarProjectList(m_memory_pool));
	}
	else
	{	
		DrgPexpr *pdrgpexprProjElems = GPOS_NEW(m_memory_pool) DrgPexpr(m_memory_pool);

		const ULONG ulLen = pdxlnPrL->Arity();
		for (ULONG ul = 0; ul < ulLen; ul++)
		{
			CDXLNode *pdxlnProjElem = (*pdxlnPrL)[ul];
			CExpression *pexprProjElem = PexprScalarProjElem(pdxlnProjElem);
			pdrgpexprProjElems->Append(pexprProjElem);
		}
		
		pexprProjList = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, GPOS_NEW(m_memory_pool) CScalarProjectList(m_memory_pool), pdrgpexprProjElems);
	}
	
	return pexprProjList;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::Pos
//
//	@doc:
// 		Construct an order spec from a DXL sort col list node
//
//---------------------------------------------------------------------------
COrderSpec *
CTranslatorDXLToExpr::Pos
	(
	const CDXLNode *pdxln
	)
{
	GPOS_ASSERT(NULL != pdxln);
	
	COrderSpec *pos = GPOS_NEW(m_memory_pool) COrderSpec(m_memory_pool);
	
	const ULONG ulLen = pdxln->Arity();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		CDXLNode *pdxlnSortCol = (*pdxln)[ul];
		
		CDXLScalarSortCol *dxl_op = CDXLScalarSortCol::Cast(pdxlnSortCol->GetOperator());
		const ULONG col_id = dxl_op->UlColId();

		// get its column reference from the hash map
		CColRef *pcr =  PcrLookup(m_phmulcr, col_id);
		
		IMDId *pmdidSortOp = dxl_op->PmdidSortOp();
		pmdidSortOp->AddRef();
		
		COrderSpec::ENullTreatment ent = COrderSpec::EntLast;
		if (dxl_op->FSortNullsFirst())
		{
			ent = COrderSpec::EntFirst;
		}
		
		pos->Append(pmdidSortOp, pcr, ent);
	}
	
	return pos;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExpr::AddDistributionColumns
//
//	@doc:
// 		Add distribution column info from the MD relation to the table descriptor
//
//---------------------------------------------------------------------------
void
CTranslatorDXLToExpr::AddDistributionColumns
	(
	CTableDescriptor *ptabdesc,
	const IMDRelation *pmdrel,
	HMIUl *phmiulAttnoColMapping
	)
{
	GPOS_ASSERT(NULL != ptabdesc);
	GPOS_ASSERT(NULL != pmdrel);
	
	// compute distribution columns for table descriptor
	ULONG ulCols = pmdrel->UlDistrColumns();
	for (ULONG ul = 0; ul < ulCols; ul++)
	{
		const IMDColumn *pmdcol = pmdrel->PmdcolDistrColumn(ul);
		INT iAttno = pmdcol->AttrNum();
		ULONG *pulPos = phmiulAttnoColMapping->Find(&iAttno);
		GPOS_ASSERT(NULL != pulPos);
		
		ptabdesc->AddDistributionColumn(*pulPos);
	}
}

// EOF
