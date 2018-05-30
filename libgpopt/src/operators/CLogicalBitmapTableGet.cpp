//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CLogicalBitmapTableGet.cpp
//
//	@doc:
//		Logical operator for table access via bitmap indexes.
//
//	@owner:
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalBitmapTableGet.h"

#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/xforms/CXform.h"

#include "naucrates/statistics/CStatisticsUtils.h"

using namespace gpopt;
using namespace gpos;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalBitmapTableGet::CLogicalBitmapTableGet
//
//	@doc:
//		Ctor
//		Takes ownership of ptabdesc, pnameTableAlias and pdrgpcrOutput.
//
//---------------------------------------------------------------------------
CLogicalBitmapTableGet::CLogicalBitmapTableGet
	(
	IMemoryPool *memory_pool,
	CTableDescriptor *ptabdesc,
	ULONG ulOriginOpId,
	const CName *pnameTableAlias,
	DrgPcr *pdrgpcrOutput
	)
	:
	CLogical(memory_pool),
	m_ptabdesc(ptabdesc),
	m_ulOriginOpId(ulOriginOpId),
	m_pnameTableAlias(pnameTableAlias),
	m_pdrgpcrOutput(pdrgpcrOutput)
{
	GPOS_ASSERT(NULL != ptabdesc);
	GPOS_ASSERT(NULL != pnameTableAlias);
	GPOS_ASSERT(NULL != pdrgpcrOutput);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalBitmapTableGet::CLogicalBitmapTableGet
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalBitmapTableGet::CLogicalBitmapTableGet
	(
	IMemoryPool *memory_pool
	)
	:
	CLogical(memory_pool),
	m_ptabdesc(NULL),
	m_ulOriginOpId(ULONG_MAX),
	m_pnameTableAlias(NULL),
	m_pdrgpcrOutput(NULL)
{}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalBitmapTableGet::~CLogicalBitmapTableGet
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalBitmapTableGet::~CLogicalBitmapTableGet()
{
	CRefCount::SafeRelease(m_ptabdesc);
	CRefCount::SafeRelease(m_pdrgpcrOutput);

	GPOS_DELETE(m_pnameTableAlias);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalBitmapTableGet::HashValue
//
//	@doc:
//		Operator specific hash function
//
//---------------------------------------------------------------------------
ULONG
CLogicalBitmapTableGet::HashValue() const
{
	ULONG ulHash = gpos::CombineHashes(COperator::HashValue(), m_ptabdesc->MDId()->HashValue());
	ulHash = gpos::CombineHashes(ulHash, CUtils::UlHashColArray(m_pdrgpcrOutput));

	return ulHash;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalBitmapTableGet::FMatch
//
//	@doc:
//		Match this operator with the given one.
//
//---------------------------------------------------------------------------
BOOL
CLogicalBitmapTableGet::FMatch
	(
	COperator *pop
	)
	const
{
	return CUtils::FMatchBitmapScan(this, pop);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalBitmapTableGet::PcrsDeriveOutput
//
//	@doc:
//		Derive output columns
//
//---------------------------------------------------------------------------
CColRefSet *
CLogicalBitmapTableGet::PcrsDeriveOutput
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &
#ifdef GPOS_DEBUG
		exprhdl
#endif
	)
{
	GPOS_ASSERT(exprhdl.Pop() == this);

	return GPOS_NEW(memory_pool) CColRefSet(memory_pool, m_pdrgpcrOutput);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalBitmapTableGet::PcrsDeriveOuter
//
//	@doc:
//		Derive outer references
//
//---------------------------------------------------------------------------
CColRefSet *
CLogicalBitmapTableGet::PcrsDeriveOuter
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl
	)
{
	return PcrsDeriveOuterIndexGet(memory_pool, exprhdl);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalBitmapTableGet::PpcDeriveConstraint
//
//	@doc:
//		Derive the constraint property.
//
//---------------------------------------------------------------------------
CPropConstraint *
CLogicalBitmapTableGet::PpcDeriveConstraint
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
//		CLogicalBitmapTableGet::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
IStatistics *
CLogicalBitmapTableGet::PstatsDerive
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
//		CLogicalBitmapTableGet::OsPrint
//
//	@doc:
//		Debug print of this operator
//
//---------------------------------------------------------------------------
IOstream &
CLogicalBitmapTableGet::OsPrint
	(
	IOstream &os
	)
	const
{
	os << SzId() << " ";
	os << ", Table Name: (";
	m_ptabdesc->Name().OsPrint(os);
	os <<")";
	os << ", Columns: [";
	CUtils::OsPrintDrgPcr(os, m_pdrgpcrOutput);
	os << "]";

	return os;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalBitmapTableGet::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
COperator *
CLogicalBitmapTableGet::PopCopyWithRemappedColumns
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
	CName *pnameAlias = GPOS_NEW(memory_pool) CName(memory_pool, *m_pnameTableAlias);

	m_ptabdesc->AddRef();

	return GPOS_NEW(memory_pool) CLogicalBitmapTableGet(memory_pool, m_ptabdesc, m_ulOriginOpId, pnameAlias, pdrgpcrOutput);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalBitmapTableGet::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
CXformSet *
CLogicalBitmapTableGet::PxfsCandidates
	(
	IMemoryPool *memory_pool
	)
	const
{
	CXformSet *pxfs = GPOS_NEW(memory_pool) CXformSet(memory_pool);
	(void) pxfs->ExchangeSet(CXform::ExfImplementBitmapTableGet);

	return pxfs;
}

// EOF
