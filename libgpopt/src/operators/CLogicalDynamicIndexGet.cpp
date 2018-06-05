//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalDynamicIndexGet.cpp
//
//	@doc:
//		Implementation of index access for partitioned tables
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpos/common/CAutoP.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CColRefTable.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/base/COptCtxt.h"

#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalDynamicIndexGet.h"
#include "gpopt/metadata/CName.h"
#include "gpopt/metadata/CPartConstraint.h"

#include "naucrates/statistics/CStatisticsUtils.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::CLogicalDynamicIndexGet
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalDynamicIndexGet::CLogicalDynamicIndexGet
	(
	IMemoryPool *memory_pool
	)
	:
	CLogicalDynamicGetBase(memory_pool),
	m_pindexdesc(NULL),
	m_ulOriginOpId(ULONG_MAX),
	m_pos(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::CLogicalDynamicIndexGet
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalDynamicIndexGet::CLogicalDynamicIndexGet
	(
	IMemoryPool *memory_pool,
	const IMDIndex *pmdindex,
	CTableDescriptor *ptabdesc,
	ULONG ulOriginOpId,
	const CName *pnameAlias,
	ULONG part_idx_id,
	DrgPcr *pdrgpcrOutput,
	DrgDrgPcr *pdrgpdrgpcrPart,
	ULONG ulSecondaryPartIndexId,
	CPartConstraint *ppartcnstr,
	CPartConstraint *ppartcnstrRel
	)
	:
	CLogicalDynamicGetBase(memory_pool, pnameAlias, ptabdesc, part_idx_id, pdrgpcrOutput, pdrgpdrgpcrPart, ulSecondaryPartIndexId, FPartialIndex(ptabdesc, pmdindex), ppartcnstr, ppartcnstrRel),
	m_pindexdesc(NULL),
	m_ulOriginOpId(ulOriginOpId)
{
	GPOS_ASSERT(NULL != pmdindex);

	// create the index descriptor
	m_pindexdesc  = CIndexDescriptor::Pindexdesc(memory_pool, ptabdesc, pmdindex);

	// compute the order spec
	m_pos = PosFromIndex(m_memory_pool, pmdindex, m_pdrgpcrOutput, ptabdesc);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::~CLogicalDynamicIndexGet
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalDynamicIndexGet::~CLogicalDynamicIndexGet()
{
	CRefCount::SafeRelease(m_pindexdesc);
	CRefCount::SafeRelease(m_pos);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::HashValue
//
//	@doc:
//		Operator specific hash function
//
//---------------------------------------------------------------------------
ULONG
CLogicalDynamicIndexGet::HashValue() const
{
	return gpos::CombineHashes(COperator::HashValue(),
	                             gpos::CombineHashes(gpos::HashValue(&m_scan_id),
					             m_pindexdesc->MDId()->HashValue()));
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::FMatch
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CLogicalDynamicIndexGet::FMatch
	(
	COperator *pop
	)
	const
{
	return CUtils::FMatchDynamicIndex(this, pop);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::PcrsDeriveOuter
//
//	@doc:
//		Derive outer references
//
//---------------------------------------------------------------------------
CColRefSet *
CLogicalDynamicIndexGet::PcrsDeriveOuter
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl
	)
{
	return PcrsDeriveOuterIndexGet(memory_pool, exprhdl);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
COperator *
CLogicalDynamicIndexGet::PopCopyWithRemappedColumns
	(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr,
	BOOL fMustExist
	)
{
	CMDAccessor *pmda = COptCtxt::PoctxtFromTLS()->Pmda();
	const IMDIndex *pmdindex = pmda->Pmdindex(m_pindexdesc->MDId());
	CName *pnameAlias = GPOS_NEW(memory_pool) CName(memory_pool, *m_pnameAlias);

	DrgPcr *pdrgpcrOutput = NULL;
	if (fMustExist)
	{
		pdrgpcrOutput = CUtils::PdrgpcrRemapAndCreate(memory_pool, m_pdrgpcrOutput, phmulcr);
	}
	else
	{
		pdrgpcrOutput = CUtils::PdrgpcrRemap(memory_pool, m_pdrgpcrOutput, phmulcr, fMustExist);
	}

	DrgDrgPcr *pdrgpdrgpcrPart = CUtils::PdrgpdrgpcrRemap(memory_pool, m_pdrgpdrgpcrPart, phmulcr, fMustExist);
	CPartConstraint *ppartcnstr = m_ppartcnstr->PpartcnstrCopyWithRemappedColumns(memory_pool, phmulcr, fMustExist);
	CPartConstraint *ppartcnstrRel = m_ppartcnstrRel->PpartcnstrCopyWithRemappedColumns(memory_pool, phmulcr, fMustExist);

	m_ptabdesc->AddRef();

	return GPOS_NEW(memory_pool) CLogicalDynamicIndexGet
						(
						memory_pool,
						pmdindex,
						m_ptabdesc,
						m_ulOriginOpId,
						pnameAlias,
						m_scan_id,
						pdrgpcrOutput,
						pdrgpdrgpcrPart,
						m_ulSecondaryScanId,
						ppartcnstr,
						ppartcnstrRel
						);
}

// Checking if index is partial given the table descriptor and mdid of the index
BOOL
CLogicalDynamicIndexGet::FPartialIndex
	(
	CTableDescriptor *ptabdesc,
	const IMDIndex *pmdindex
	)
{
	// refer to the relation on which this index is defined for index partial information
	CMDAccessor *pmda = COptCtxt::PoctxtFromTLS()->Pmda();
	const IMDRelation *pmdrel = pmda->Pmdrel(ptabdesc->MDId());
	return pmdrel->FPartialIndex(pmdindex->MDId());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::FInputOrderSensitive
//
//	@doc:
//		Is input order sensitive
//
//---------------------------------------------------------------------------
BOOL
CLogicalDynamicIndexGet::FInputOrderSensitive() const
{
	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
CXformSet *
CLogicalDynamicIndexGet::PxfsCandidates
	(
	IMemoryPool *memory_pool
	)
const
{
	CXformSet *pxfs = GPOS_NEW(memory_pool) CXformSet(memory_pool);
	(void) pxfs->ExchangeSet(CXform::ExfDynamicIndexGet2DynamicIndexScan);
	return pxfs;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::PstatsDerive
//
//	@doc:
//		Load up statistics from metadata
//
//---------------------------------------------------------------------------

IStatistics *
CLogicalDynamicIndexGet::PstatsDerive
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	DrgPstat *pdrgpstatCtxt
	)
	const
{
	return CStatisticsUtils::PstatsIndexGet(memory_pool, exprhdl, pdrgpstatCtxt);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicIndexGet::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CLogicalDynamicIndexGet::OsPrint
	(
	IOstream &os
	)
const
{
	if (m_fPattern)
	{
		return COperator::OsPrint(os);
	}

	os << SzId() << " ";
	// index name
	os << "  Index Name: (";
	m_pindexdesc->Name().OsPrint(os);
	// table alias name
	os <<")";
	os << ", Table Name: (";
	m_pnameAlias->OsPrint(os);
	os <<"), ";
	m_ppartcnstr->OsPrint(os);
	os << ", Columns: [";
	CUtils::OsPrintDrgPcr(os, m_pdrgpcrOutput);
	os << "] Scan Id: " << m_scan_id << "." << m_ulSecondaryScanId;

	if (!m_ppartcnstr->FUnbounded())
	{
		os << ", ";
		m_ppartcnstr->OsPrint(os);
	}
	
	return os;
}

// EOF

