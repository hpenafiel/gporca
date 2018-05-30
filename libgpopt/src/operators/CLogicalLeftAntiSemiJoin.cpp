//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CLogicalLeftAntiSemiJoin.cpp
//
//	@doc:
//		Implementation of left anti semi join operator
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"

#include "gpopt/operators/CLogicalLeftAntiSemiJoin.h"
#include "naucrates/statistics/CStatsPredUtils.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftAntiSemiJoin::CLogicalLeftAntiSemiJoin
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CLogicalLeftAntiSemiJoin::CLogicalLeftAntiSemiJoin
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
//		CLogicalLeftAntiSemiJoin::MaxCard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalLeftAntiSemiJoin::MaxCard
	(
	IMemoryPool *, // memory_pool
	CExpressionHandle &exprhdl
	)
	const
{
	// pass on max card of first child
	return exprhdl.Pdprel(0)->Maxcard();
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftAntiSemiJoin::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
CXformSet *
CLogicalLeftAntiSemiJoin::PxfsCandidates
	(
	IMemoryPool *memory_pool
	)
	const
{
	CXformSet *pxfs = GPOS_NEW(memory_pool) CXformSet(memory_pool);

	(void) pxfs->ExchangeSet(CXform::ExfAntiSemiJoinAntiSemiJoinSwap);
	(void) pxfs->ExchangeSet(CXform::ExfAntiSemiJoinAntiSemiJoinNotInSwap);
	(void) pxfs->ExchangeSet(CXform::ExfAntiSemiJoinSemiJoinSwap);
	(void) pxfs->ExchangeSet(CXform::ExfAntiSemiJoinInnerJoinSwap);
	(void) pxfs->ExchangeSet(CXform::ExfLeftAntiSemiJoin2CrossProduct);
	(void) pxfs->ExchangeSet(CXform::ExfLeftAntiSemiJoin2NLJoin);
	(void) pxfs->ExchangeSet(CXform::ExfLeftAntiSemiJoin2HashJoin);
	return pxfs;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftAntiSemiJoin::PcrsDeriveOutput
//
//	@doc:
//		Derive output columns
//
//---------------------------------------------------------------------------
CColRefSet *
CLogicalLeftAntiSemiJoin::PcrsDeriveOutput
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
//		CLogicalLeftAntiSemiJoin::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
CKeyCollection *
CLogicalLeftAntiSemiJoin::PkcDeriveKeys
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
//		CLogicalLeftAntiSemiJoin::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
IStatistics *
CLogicalLeftAntiSemiJoin::PstatsDerive
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
	IStatistics *pstatsLASJoin = pstatsOuter->PstatsLASJoin
												(
												memory_pool,
												pstatsInner,
												pdrgpstatspredjoin,
												true /* fIgnoreLasjHistComputation */
												);

	// clean up
	pdrgpstatspredjoin->Release();

	return pstatsLASJoin;
}
// EOF

