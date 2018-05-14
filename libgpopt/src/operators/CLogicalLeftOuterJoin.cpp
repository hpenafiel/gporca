//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CLogicalLeftOuterJoin.cpp
//
//	@doc:
//		Implementation of left outer join operator
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"

#include "gpopt/operators/CLogicalLeftOuterJoin.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftOuterJoin::CLogicalLeftOuterJoin
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CLogicalLeftOuterJoin::CLogicalLeftOuterJoin
	(
	IMemoryPool *pmp
	)
	:
	CLogicalJoin(pmp)
{
	GPOS_ASSERT(NULL != pmp);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftOuterJoin::Maxcard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalLeftOuterJoin::Maxcard
	(
	IMemoryPool *, // pmp
	CExpressionHandle &exprhdl
	)
	const
{
	return CLogical::Maxcard(exprhdl, 2 /*ulScalarIndex*/, exprhdl.Pdprel(0)->Maxcard());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftOuterJoin::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
CXformSet *
CLogicalLeftOuterJoin::PxfsCandidates
	(
	IMemoryPool *pmp
	) 
	const
{
	CXformSet *pxfs = GPOS_NEW(pmp) CXformSet(pmp);

	(void) pxfs->ExchangeSet(CXform::ExfPushDownLeftOuterJoin);
	(void) pxfs->ExchangeSet(CXform::ExfSimplifyLeftOuterJoin);
	(void) pxfs->ExchangeSet(CXform::ExfLeftOuterJoin2BitmapIndexGetApply);
	(void) pxfs->ExchangeSet(CXform::ExfLeftOuterJoin2IndexGetApply);
	(void) pxfs->ExchangeSet(CXform::ExfLeftOuterJoin2NLJoin);
	(void) pxfs->ExchangeSet(CXform::ExfLeftOuterJoin2HashJoin);
	(void) pxfs->ExchangeSet(CXform::ExfLeftOuter2InnerUnionAllLeftAntiSemiJoin);
	(void) pxfs->ExchangeSet(CXform::ExfLeftOuterJoinWithInnerSelect2BitmapIndexGetApply);
	(void) pxfs->ExchangeSet(CXform::ExfLeftOuterJoinWithInnerSelect2IndexGetApply);

	return pxfs;
}



// EOF

