//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CLogicalDynamicBitmapTableGet.cpp
//
//	@doc:
//		Logical operator for dynamic table access via bitmap indexes.
//
//	@owner:
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalDynamicBitmapTableGet.h"

#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/xforms/CXform.h"

#include "naucrates/statistics/CStatisticsUtils.h"

using namespace gpopt;
using namespace gpos;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::CLogicalDynamicBitmapTableGet
//
//	@doc:
//		Ctor
//		Takes ownership of ptabdesc, pnameTableAlias and pdrgpcrOutput.
//
//---------------------------------------------------------------------------
CLogicalDynamicBitmapTableGet::CLogicalDynamicBitmapTableGet
	(
	IMemoryPool *memory_pool,
	CTableDescriptor *ptabdesc,
	ULONG ulOriginOpId,
	const CName *pnameTableAlias,
	ULONG ulPartIndex,
	DrgPcr *pdrgpcrOutput,
	DrgDrgPcr *pdrgpdrgpcrPart,
	ULONG ulSecondaryPartIndexId,
	BOOL fPartial,
	CPartConstraint *ppartcnstr,
	CPartConstraint *ppartcnstrRel
	)
	:
	CLogicalDynamicGetBase
	(
	memory_pool,
	pnameTableAlias,
	ptabdesc,
	ulPartIndex,
	pdrgpcrOutput,
	pdrgpdrgpcrPart,
	ulSecondaryPartIndexId,
	fPartial,
	ppartcnstr,
	ppartcnstrRel
	),
	m_ulOriginOpId(ulOriginOpId)

{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::CLogicalDynamicBitmapTableGet
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalDynamicBitmapTableGet::CLogicalDynamicBitmapTableGet
	(
	IMemoryPool *memory_pool
	)
	:
	CLogicalDynamicGetBase(memory_pool),
	m_ulOriginOpId(ULONG_MAX)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::~CLogicalDynamicBitmapTableGet
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalDynamicBitmapTableGet::~CLogicalDynamicBitmapTableGet()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::HashValue
//
//	@doc:
//		Operator specific hash function
//
//---------------------------------------------------------------------------
ULONG
CLogicalDynamicBitmapTableGet::HashValue() const
{
	ULONG ulHash = gpos::CombineHashes(COperator::HashValue(), m_ptabdesc->MDId()->HashValue());
	ulHash = gpos::CombineHashes(ulHash, gpos::HashValue(&m_scan_id));
	ulHash = gpos::CombineHashes(ulHash, CUtils::UlHashColArray(m_pdrgpcrOutput));

	return ulHash;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::FMatch
//
//	@doc:
//		Match this operator with the given one.
//
//---------------------------------------------------------------------------
BOOL
CLogicalDynamicBitmapTableGet::FMatch
	(
	COperator *pop
	)
	const
{
	return CUtils::FMatchDynamicBitmapScan(this, pop);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::PpcDeriveConstraint
//
//	@doc:
//		Derive the constraint property.
//
//---------------------------------------------------------------------------
CPropConstraint *
CLogicalDynamicBitmapTableGet::PpcDeriveConstraint
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl
	)
	const
{
	return PpcDeriveConstraintFromTableWithPredicates(memory_pool, exprhdl, m_ptabdesc, m_pdrgpcrOutput);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::PcrsDeriveOuter
//
//	@doc:
//		Derive outer references
//
//---------------------------------------------------------------------------
CColRefSet *
CLogicalDynamicBitmapTableGet::PcrsDeriveOuter
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl
	)
{
	return PcrsDeriveOuterIndexGet(memory_pool, exprhdl);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
IStatistics *
CLogicalDynamicBitmapTableGet::PstatsDerive
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	DrgPstat *pdrgpstatCtxt
	)
	const
{
	return CStatisticsUtils::PstatsBitmapTableGet(memory_pool, exprhdl, pdrgpstatCtxt);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::OsPrint
//
//	@doc:
//		Debug print of this operator
//
//---------------------------------------------------------------------------
IOstream &
CLogicalDynamicBitmapTableGet::OsPrint
	(
	IOstream &os
	)
	const
{
	os << SzId() << " ";
	os << ", Table Name: (";
	m_ptabdesc->Name().OsPrint(os);
	os <<") Scan Id: " << m_scan_id;
	os << ", Columns: [";
	CUtils::OsPrintDrgPcr(os, m_pdrgpcrOutput);
	os << "]";

	return os;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
COperator *
CLogicalDynamicBitmapTableGet::PopCopyWithRemappedColumns
	(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr,
	BOOL fMustExist
	)
{
	DrgPcr *pdrgpcrOutput = NULL;
	if (fMustExist)
	{
		pdrgpcrOutput = CUtils::PdrgpcrRemapAndCreate(memory_pool, m_pdrgpcrOutput, phmulcr);
	}
	else
	{
		pdrgpcrOutput = CUtils::PdrgpcrRemap(memory_pool, m_pdrgpcrOutput, phmulcr, fMustExist);
	}
	CName *pnameAlias = GPOS_NEW(memory_pool) CName(memory_pool, *m_pnameAlias);

	m_ptabdesc->AddRef();

	DrgDrgPcr *pdrgpdrgpcrPart = CUtils::PdrgpdrgpcrRemap(memory_pool, m_pdrgpdrgpcrPart, phmulcr, fMustExist);
	CPartConstraint *ppartcnstr = m_ppartcnstr->PpartcnstrCopyWithRemappedColumns(memory_pool, phmulcr, fMustExist);
	CPartConstraint *ppartcnstrRel = m_ppartcnstrRel->PpartcnstrCopyWithRemappedColumns(memory_pool, phmulcr, fMustExist);

	return GPOS_NEW(memory_pool) CLogicalDynamicBitmapTableGet
					(
					memory_pool,
					m_ptabdesc,
					m_ulOriginOpId,
					pnameAlias,
					m_scan_id,
					pdrgpcrOutput,
					pdrgpdrgpcrPart,
					m_ulSecondaryScanId,
					m_fPartial,
					ppartcnstr,
					ppartcnstrRel
					);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicBitmapTableGet::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
CXformSet *
CLogicalDynamicBitmapTableGet::PxfsCandidates
	(
	IMemoryPool *memory_pool
	)
const
{
	CXformSet *pxfs = GPOS_NEW(memory_pool) CXformSet(memory_pool);
	(void) pxfs->ExchangeSet(CXform::ExfImplementDynamicBitmapTableGet);

	return pxfs;
}

// EOF
