//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CLogicalDynamicGetBase.cpp
//
//	@doc:
//		Implementation of dynamic table access base class
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/base/CConstraintInterval.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CPartIndexMap.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CColRefTable.h"
#include "gpopt/base/COptCtxt.h"

#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalDynamicGetBase.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/metadata/CName.h"

#include "naucrates/statistics/CStatsPredUtils.h"
#include "naucrates/statistics/CFilterStatsProcessor.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGetBase::CLogicalDynamicGetBase
//
//	@doc:
//		ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalDynamicGetBase::CLogicalDynamicGetBase
	(
	IMemoryPool *memory_pool
	)
	:
	CLogical(memory_pool),
	m_pnameAlias(NULL),
	m_ptabdesc(NULL),
	m_scan_id(0),
	m_pdrgpcrOutput(NULL),
	m_pdrgpdrgpcrPart(NULL),
	m_ulSecondaryScanId(0),
	m_fPartial(false),
	m_part_constraint(NULL),
	m_ppartcnstrRel(NULL),
	m_pcrsDist(NULL)
{
	m_fPattern = true;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGetBase::CLogicalDynamicGetBase
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CLogicalDynamicGetBase::CLogicalDynamicGetBase
	(
	IMemoryPool *memory_pool,
	const CName *pnameAlias,
	CTableDescriptor *ptabdesc,
	ULONG scan_id,
	DrgPcr *pdrgpcrOutput, 
	DrgDrgPcr *pdrgpdrgpcrPart,
	ULONG ulSecondaryScanId,
	BOOL fPartial,
	CPartConstraint *ppartcnstr,
	CPartConstraint *ppartcnstrRel
	)
	:
	CLogical(memory_pool),
	m_pnameAlias(pnameAlias),
	m_ptabdesc(ptabdesc),
	m_scan_id(scan_id),
	m_pdrgpcrOutput(pdrgpcrOutput),
	m_pdrgpdrgpcrPart(pdrgpdrgpcrPart),
	m_ulSecondaryScanId(ulSecondaryScanId),
	m_fPartial(fPartial),
	m_part_constraint(ppartcnstr),
	m_ppartcnstrRel(ppartcnstrRel),
	m_pcrsDist(NULL)
{
	GPOS_ASSERT(NULL != ptabdesc);
	GPOS_ASSERT(NULL != pnameAlias);
	GPOS_ASSERT(NULL != pdrgpcrOutput);
	GPOS_ASSERT(NULL != pdrgpdrgpcrPart);
	GPOS_ASSERT(NULL != ppartcnstr);
	GPOS_ASSERT(NULL != ppartcnstrRel);

	GPOS_ASSERT_IMP(scan_id != ulSecondaryScanId, NULL != ppartcnstr);
	GPOS_ASSERT_IMP(fPartial, NULL != m_part_constraint->PcnstrCombined() && "Partial scan with unsupported constraint type");

	m_pcrsDist = CLogical::PcrsDist(memory_pool, m_ptabdesc, m_pdrgpcrOutput);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGetBase::CLogicalDynamicGetBase
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CLogicalDynamicGetBase::CLogicalDynamicGetBase
	(
	IMemoryPool *memory_pool,
	const CName *pnameAlias,
	CTableDescriptor *ptabdesc,
	ULONG scan_id
	)
	:
	CLogical(memory_pool),
	m_pnameAlias(pnameAlias),
	m_ptabdesc(ptabdesc),
	m_scan_id(scan_id),
	m_pdrgpcrOutput(NULL),
	m_ulSecondaryScanId(scan_id),
	m_fPartial(false),
	m_part_constraint(NULL),
	m_ppartcnstrRel(NULL),
	m_pcrsDist(NULL)
{
	GPOS_ASSERT(NULL != ptabdesc);
	GPOS_ASSERT(NULL != pnameAlias);
	
	// generate a default column set for the table descriptor
	m_pdrgpcrOutput = PdrgpcrCreateMapping(memory_pool, m_ptabdesc->Pdrgpcoldesc(), UlOpId());
	m_pdrgpdrgpcrPart = PdrgpdrgpcrCreatePartCols(memory_pool, m_pdrgpcrOutput, m_ptabdesc->PdrgpulPart());
	
	// generate a constraint "true"
	HMUlCnstr *phmulcnstr = CUtils::PhmulcnstrBoolConstOnPartKeys(memory_pool, m_pdrgpdrgpcrPart, true /*value*/);
	CBitSet *pbsDefaultParts = CUtils::PbsAllSet(memory_pool, m_pdrgpdrgpcrPart->Size());
	m_pdrgpdrgpcrPart->AddRef();
	m_part_constraint = GPOS_NEW(memory_pool) CPartConstraint(memory_pool, phmulcnstr, pbsDefaultParts, true /*fUnbounded*/, m_pdrgpdrgpcrPart);
	m_part_constraint->AddRef();
	m_ppartcnstrRel = m_part_constraint;
        
	m_pcrsDist = CLogical::PcrsDist(memory_pool, m_ptabdesc, m_pdrgpcrOutput);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGetBase::~CLogicalDynamicGetBase
//
//	@doc:
//		dtor
//
//---------------------------------------------------------------------------
CLogicalDynamicGetBase::~CLogicalDynamicGetBase()
{
	CRefCount::SafeRelease(m_ptabdesc);
	CRefCount::SafeRelease(m_pdrgpcrOutput);
	CRefCount::SafeRelease(m_pdrgpdrgpcrPart);
	CRefCount::SafeRelease(m_part_constraint);
	CRefCount::SafeRelease(m_ppartcnstrRel);
	CRefCount::SafeRelease(m_pcrsDist);

	GPOS_DELETE(m_pnameAlias);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGetBase::PcrsDeriveOutput
//
//	@doc:
//		Derive output columns
//
//---------------------------------------------------------------------------
CColRefSet *
CLogicalDynamicGetBase::PcrsDeriveOutput
	(
	IMemoryPool *memory_pool,
	CExpressionHandle & // exprhdl
	)
{
	CColRefSet *pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool);
	pcrs->Include(m_pdrgpcrOutput);

	return pcrs;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGetBase::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
CKeyCollection *
CLogicalDynamicGetBase::PkcDeriveKeys
	(
	IMemoryPool *memory_pool,
	CExpressionHandle & // exprhdl
	)
	const
{
	const DrgPbs *pdrgpbs = m_ptabdesc->PdrgpbsKeys();

	return CLogical::PkcKeysBaseTable(memory_pool, pdrgpbs, m_pdrgpcrOutput);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGetBase::PpcDeriveConstraint
//
//	@doc:
//		Derive constraint property
//
//---------------------------------------------------------------------------
CPropConstraint *
CLogicalDynamicGetBase::PpcDeriveConstraint
	(
	IMemoryPool *memory_pool,
	CExpressionHandle & // exprhdl
	)
	const
{
	return PpcDeriveConstraintFromTable(memory_pool, m_ptabdesc, m_pdrgpcrOutput);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGetBase::PpartinfoDerive
//
//	@doc:
//		Derive partition consumer info
//
//---------------------------------------------------------------------------
CPartInfo *
CLogicalDynamicGetBase::PpartinfoDerive
	(
	IMemoryPool *memory_pool,
	CExpressionHandle & // exprhdl
	)
	const
{
	IMDId *mdid = m_ptabdesc->MDId();
	mdid->AddRef();
	m_pdrgpdrgpcrPart->AddRef();
	m_ppartcnstrRel->AddRef(); 
	
	CPartInfo *ppartinfo = GPOS_NEW(memory_pool) CPartInfo(memory_pool);
	ppartinfo->AddPartConsumer(memory_pool, m_scan_id, mdid, m_pdrgpdrgpcrPart, m_ppartcnstrRel);
	
	return ppartinfo;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGetBase::SetPartConstraint
//
//	@doc:
//		Set part constraint
//
//---------------------------------------------------------------------------
void
CLogicalDynamicGetBase::SetPartConstraint
	(
	CPartConstraint *ppartcnstr
	) 
{
	GPOS_ASSERT(NULL != ppartcnstr);
	GPOS_ASSERT(NULL != m_part_constraint);

	m_part_constraint->Release();
	m_part_constraint = ppartcnstr;
	
	m_ppartcnstrRel->Release();
	ppartcnstr->AddRef();
	m_ppartcnstrRel = ppartcnstr;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGetBase::SetSecondaryScanId
//
//	@doc:
//		Set secondary scan id
//
//---------------------------------------------------------------------------
void
CLogicalDynamicGetBase::SetSecondaryScanId
	(
	ULONG scan_id
	)
{
	m_ulSecondaryScanId = scan_id;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGetBase::SetPartial
//
//	@doc:
//		Set partial to true
//
//---------------------------------------------------------------------------
void
CLogicalDynamicGetBase::SetPartial()
{
	GPOS_ASSERT(!FPartial());
	GPOS_ASSERT(NULL != m_part_constraint->PcnstrCombined() && "Partial scan with unsupported constraint type");
	m_fPartial = true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGetBase::PstatsDeriveFilter
//
//	@doc:
//		Derive stats from base table using filters on partition and/or index columns
//
//---------------------------------------------------------------------------
IStatistics *
CLogicalDynamicGetBase::PstatsDeriveFilter
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	CExpression *pexprFilter
	)
	const
{
	CExpression *pexprFilterNew = NULL;
	CConstraint *pcnstr = m_part_constraint->PcnstrCombined();
	if (m_fPartial && NULL != pcnstr && !pcnstr->FUnbounded())
	{
		if (NULL == pexprFilter)
		{
			pexprFilterNew = pcnstr->PexprScalar(memory_pool);
			pexprFilterNew->AddRef();
		}
		else
		{
			pexprFilterNew = CPredicateUtils::PexprConjunction(memory_pool, pexprFilter, pcnstr->PexprScalar(memory_pool));
		}
	}
	else if (NULL != pexprFilter)
	{
		pexprFilterNew = pexprFilter;
		pexprFilterNew->AddRef();
	}

	CColRefSet *pcrsStat = GPOS_NEW(memory_pool) CColRefSet(memory_pool);
	CDrvdPropScalar *pdpscalar = NULL;

	if (NULL != pexprFilterNew)
	{
		pdpscalar = CDrvdPropScalar::Pdpscalar(pexprFilterNew->PdpDerive());
		pcrsStat->Include(pdpscalar->PcrsUsed());
	}

	// requesting statistics on distribution columns to estimate data skew
	if (NULL != m_pcrsDist)
	{
		pcrsStat->Include(m_pcrsDist);
	}


	CStatistics *pstatsFullTable = dynamic_cast<CStatistics *>(PstatsBaseTable(memory_pool, exprhdl, m_ptabdesc, pcrsStat));
	
	pcrsStat->Release();
	
	if (NULL == pexprFilterNew || pdpscalar->FHasSubquery())
	{
		return pstatsFullTable;
	}

	CStatsPred *pstatspred =  CStatsPredUtils::PstatspredExtract
												(
												memory_pool, 
												pexprFilterNew, 
												NULL /*pcrsOuterRefs*/
												);
	pexprFilterNew->Release();

	IStatistics *pstatsResult = CFilterStatsProcessor::PstatsFilter(memory_pool, pstatsFullTable, pstatspred, true /* fCapNdvs */);
	pstatspred->Release();
	pstatsFullTable->Release();

	return pstatsResult;
}

// EOF

