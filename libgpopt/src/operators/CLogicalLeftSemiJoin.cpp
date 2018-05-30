	//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CLogicalLeftSemiJoin.cpp
//
//	@doc:
//		Implementation of left semi join operator
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalLeftSemiJoin.h"

#include "naucrates/statistics/CStatsPredUtils.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftSemiJoin::CLogicalLeftSemiJoin
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CLogicalLeftSemiJoin::CLogicalLeftSemiJoin
	(
	IMemoryPool *memory_pool
	)
	:
	CLogicalJoin(memory_pool)
{
	GPOS_ASSERT(NULL != memory_pool);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftSemiJoin::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
CXformSet *
CLogicalLeftSemiJoin::PxfsCandidates
	(
	IMemoryPool *memory_pool
	)
	const
{
	CXformSet *pxfs = GPOS_NEW(memory_pool) CXformSet(memory_pool);

	(void) pxfs->ExchangeSet(CXform::ExfSemiJoinSemiJoinSwap);
	(void) pxfs->ExchangeSet(CXform::ExfSemiJoinAntiSemiJoinSwap);
	(void) pxfs->ExchangeSet(CXform::ExfSemiJoinAntiSemiJoinNotInSwap);
	(void) pxfs->ExchangeSet(CXform::ExfSemiJoinInnerJoinSwap);
	(void) pxfs->ExchangeSet(CXform::ExfLeftSemiJoin2InnerJoin);
	(void) pxfs->ExchangeSet(CXform::ExfLeftSemiJoin2InnerJoinUnderGb);
	(void) pxfs->ExchangeSet(CXform::ExfLeftSemiJoin2CrossProduct);
	(void) pxfs->ExchangeSet(CXform::ExfLeftSemiJoin2NLJoin);
	(void) pxfs->ExchangeSet(CXform::ExfLeftSemiJoin2HashJoin);

	return pxfs;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftSemiJoin::PcrsDeriveOutput
//
//	@doc:
//		Derive output columns
//
//---------------------------------------------------------------------------
CColRefSet *
CLogicalLeftSemiJoin::PcrsDeriveOutput
	(
	IMemoryPool *, // memory_pool
	CExpressionHandle &exprhdl
	)
{
	GPOS_ASSERT(3 == exprhdl.Arity());

	return PcrsDeriveOutputPassThru(exprhdl);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftSemiJoin::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
CKeyCollection *
CLogicalLeftSemiJoin::PkcDeriveKeys
	(
	IMemoryPool *, // memory_pool
	CExpressionHandle &exprhdl
	)
	const
{
	return PkcDeriveKeysPassThru(exprhdl, 0 /* ulChild */);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftSemiJoin::Maxcard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalLeftSemiJoin::Maxcard
	(
	IMemoryPool *, // memory_pool
	CExpressionHandle &exprhdl
	)
	const
{
	return CLogical::Maxcard(exprhdl, 2 /*ulScalarIndex*/, exprhdl.Pdprel(0)->Maxcard());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftSemiJoin::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
IStatistics *
CLogicalLeftSemiJoin::PstatsDerive
	(
	IMemoryPool *memory_pool,
	DrgPstatspredjoin *pdrgpstatspredjoin,
	IStatistics *pstatsOuter,
	IStatistics *pstatsInner
	)
{
	return pstatsOuter->PstatsLSJoin(memory_pool, pstatsInner, pdrgpstatspredjoin);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftSemiJoin::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
IStatistics *
CLogicalLeftSemiJoin::PstatsDerive
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	DrgPstat * // not used
	)
	const
{
	GPOS_ASSERT(Esp(exprhdl) > EspNone);
	IStatistics *pstatsOuter = exprhdl.Pstats(0);
	IStatistics *pstatsInner = exprhdl.Pstats(1);
	DrgPstatspredjoin *pdrgpstatspredjoin = CStatsPredUtils::Pdrgpstatspredjoin(memory_pool, exprhdl);
	IStatistics *pstatsSemiJoin = PstatsDerive(memory_pool, pdrgpstatspredjoin, pstatsOuter, pstatsInner);

	pdrgpstatspredjoin->Release();

	return pstatsSemiJoin;
}
// EOF
