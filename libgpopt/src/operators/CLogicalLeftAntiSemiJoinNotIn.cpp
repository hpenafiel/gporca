//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CLogicalLeftAntiSemiJoinNotIn.cpp
//
//	@doc:
//		Implementation of left anti semi join operator
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"

#include "gpopt/operators/CLogicalLeftAntiSemiJoinNotIn.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftAntiSemiJoinNotIn::CLogicalLeftAntiSemiJoinNotIn
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CLogicalLeftAntiSemiJoinNotIn::CLogicalLeftAntiSemiJoinNotIn
	(
	IMemoryPool *memory_pool
	)
	:
	CLogicalLeftAntiSemiJoin(memory_pool)
{
	GPOS_ASSERT(NULL != memory_pool);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftAntiSemiJoinNotIn::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
CXformSet *
CLogicalLeftAntiSemiJoinNotIn::PxfsCandidates
	(
	IMemoryPool *memory_pool
	)
	const
{
	CXformSet *pxfs = GPOS_NEW(memory_pool) CXformSet(memory_pool);

	(void) pxfs->ExchangeSet(CXform::ExfAntiSemiJoinNotInAntiSemiJoinNotInSwap);
	(void) pxfs->ExchangeSet(CXform::ExfAntiSemiJoinNotInAntiSemiJoinSwap);
	(void) pxfs->ExchangeSet(CXform::ExfAntiSemiJoinNotInSemiJoinSwap);
	(void) pxfs->ExchangeSet(CXform::ExfAntiSemiJoinNotInInnerJoinSwap);
	(void) pxfs->ExchangeSet(CXform::ExfLeftAntiSemiJoinNotIn2CrossProduct);
	(void) pxfs->ExchangeSet(CXform::ExfLeftAntiSemiJoinNotIn2NLJoinNotIn);
	(void) pxfs->ExchangeSet(CXform::ExfLeftAntiSemiJoinNotIn2HashJoinNotIn);
	return pxfs;
}

// EOF
