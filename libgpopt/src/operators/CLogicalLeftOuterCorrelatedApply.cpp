//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalLeftOuterCorrelatedApply.cpp
//
//	@doc:
//		Implementation of left outer correlated apply operator
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/operators/CLogicalLeftOuterCorrelatedApply.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftOuterCorrelatedApply::CLogicalLeftOuterCorrelatedApply
//
//	@doc:
//		Ctor - for patterns
//
//---------------------------------------------------------------------------
CLogicalLeftOuterCorrelatedApply::CLogicalLeftOuterCorrelatedApply
	(
	IMemoryPool *memory_pool
	)
	:
	CLogicalLeftOuterApply(memory_pool)
{}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftOuterCorrelatedApply::CLogicalLeftOuterCorrelatedApply
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalLeftOuterCorrelatedApply::CLogicalLeftOuterCorrelatedApply
	(
	IMemoryPool *memory_pool,
	DrgPcr *pdrgpcrInner,
	EOperatorId eopidOriginSubq
	)
	:
	CLogicalLeftOuterApply(memory_pool, pdrgpcrInner, eopidOriginSubq)
{}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftOuterCorrelatedApply::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
CXformSet *
CLogicalLeftOuterCorrelatedApply::PxfsCandidates
	(
	IMemoryPool *memory_pool
	)
	const
{
	CXformSet *pxfs = GPOS_NEW(memory_pool) CXformSet(memory_pool);
	(void) pxfs->ExchangeSet(CXform::ExfImplementLeftOuterCorrelatedApply);

	return pxfs;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftOuterCorrelatedApply::FMatch
//
//	@doc:
//		Match function
//
//---------------------------------------------------------------------------
BOOL
CLogicalLeftOuterCorrelatedApply::FMatch
	(
	COperator *pop
	)
	const
{
	if (pop->Eopid() == Eopid())
	{
		return m_pdrgpcrInner->Equals(CLogicalLeftOuterCorrelatedApply::PopConvert(pop)->PdrgPcrInner());
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftOuterCorrelatedApply::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
COperator *
CLogicalLeftOuterCorrelatedApply::PopCopyWithRemappedColumns
	(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr,
	BOOL fMustExist
	)
{
	DrgPcr *pdrgpcrInner = CUtils::PdrgpcrRemap(memory_pool, m_pdrgpcrInner, phmulcr, fMustExist);

	return GPOS_NEW(memory_pool) CLogicalLeftOuterCorrelatedApply(memory_pool, pdrgpcrInner, m_eopidOriginSubq);
}

// EOF

