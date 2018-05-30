//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalIntersectAll.cpp
//
//	@doc:
//		Implementation of Intersect all operator
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/base/CKeyCollection.h"
#include "gpopt/operators/CLogicalIntersectAll.h"
#include "gpopt/operators/CLogicalLeftSemiJoin.h"
#include "gpopt/operators/CExpressionHandle.h"

#include "naucrates/statistics/CStatsPredUtils.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersectAll::CLogicalIntersectAll
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalIntersectAll::CLogicalIntersectAll
	(
	IMemoryPool *memory_pool
	)
	:
	CLogicalSetOp(memory_pool)
{
	m_fPattern = true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersectAll::CLogicalIntersectAll
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalIntersectAll::CLogicalIntersectAll
	(
	IMemoryPool *memory_pool,
	DrgPcr *pdrgpcrOutput,
	DrgDrgPcr *pdrgpdrgpcrInput
	)
	:
	CLogicalSetOp(memory_pool, pdrgpcrOutput, pdrgpdrgpcrInput)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersectAll::~CLogicalIntersectAll
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalIntersectAll::~CLogicalIntersectAll()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersectAll::Maxcard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalIntersectAll::Maxcard
	(
	IMemoryPool *, // memory_pool
	CExpressionHandle &exprhdl
	)
	const
{
	// contradictions produce no rows
	if (CDrvdPropRelational::Pdprel(exprhdl.Pdp())->Ppc()->FContradiction())
	{
		return CMaxCard(0 /*ull*/);
	}

	CMaxCard maxcardL = exprhdl.Pdprel(0)->Maxcard();
	CMaxCard maxcardR = exprhdl.Pdprel(1)->Maxcard();

	if (maxcardL <= maxcardR)
	{
		return maxcardL;
	}

	return maxcardR;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersectAll::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
COperator *
CLogicalIntersectAll::PopCopyWithRemappedColumns
	(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr,
	BOOL fMustExist
	)
{
	DrgPcr *pdrgpcrOutput = CUtils::PdrgpcrRemap(memory_pool, m_pdrgpcrOutput, phmulcr, fMustExist);
	DrgDrgPcr *pdrgpdrgpcrInput = CUtils::PdrgpdrgpcrRemap(memory_pool, m_pdrgpdrgpcrInput, phmulcr, fMustExist);

	return GPOS_NEW(memory_pool) CLogicalIntersectAll(memory_pool, pdrgpcrOutput, pdrgpdrgpcrInput);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersectAll::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
CKeyCollection *
CLogicalIntersectAll::PkcDeriveKeys
	(
	IMemoryPool *, //memory_pool,
	CExpressionHandle & //exprhdl
	)
	const
{
	// TODO: Add the keys from outer and inner child
	return NULL;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersectAll::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
CXformSet *
CLogicalIntersectAll::PxfsCandidates
	(
	IMemoryPool *memory_pool
	)
	const
{
	CXformSet *pxfs = GPOS_NEW(memory_pool) CXformSet(memory_pool);
	(void) pxfs->ExchangeSet(CXform::ExfIntersectAll2LeftSemiJoin);

	return pxfs;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersectAll::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
IStatistics *
CLogicalIntersectAll::PstatsDerive
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	DrgDrgPcr *pdrgpdrgpcrInput,
	DrgPcrs *pdrgpcrsOutput // output of relational children
	)
{
	GPOS_ASSERT(2 == exprhdl.UlArity());

	IStatistics *pstatsOuter = exprhdl.Pstats(0);
	IStatistics *pstatsInner = exprhdl.Pstats(1);

	// construct the scalar condition similar to transform that turns an "intersect all" into a "left semi join"
	// over a window operation on the individual input (for row_number)

	// TODO:  Jan 8th 2012, add the stats for window operation
	CExpression *pexprScCond = CUtils::PexprConjINDFCond(memory_pool, pdrgpdrgpcrInput);
	CColRefSet *pcrsOuterRefs = exprhdl.Pdprel()->PcrsOuter();
	DrgPstatspredjoin *pdrgpstatspredjoin = CStatsPredUtils::Pdrgpstatspredjoin
														(
														memory_pool, 
														exprhdl, 
														pexprScCond, 
														pdrgpcrsOutput, 
														pcrsOuterRefs
														);
	IStatistics *pstatsSemiJoin = CLogicalLeftSemiJoin::PstatsDerive(memory_pool, pdrgpstatspredjoin, pstatsOuter, pstatsInner);

	// clean up
	pexprScCond->Release();
	pdrgpstatspredjoin->Release();

	return pstatsSemiJoin;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalIntersectAll::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
IStatistics *
CLogicalIntersectAll::PstatsDerive
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	DrgPstat * // not used
	)
	const
{
	GPOS_ASSERT(Esp(exprhdl) > EspNone);

	DrgPcrs *pdrgpcrsOutput = GPOS_NEW(memory_pool) DrgPcrs(memory_pool);
	const ULONG ulSize = m_pdrgpdrgpcrInput->Size();
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CColRefSet *pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool, (*m_pdrgpdrgpcrInput)[ul]);
		pdrgpcrsOutput->Append(pcrs);
	}
	IStatistics *pstats = PstatsDerive(memory_pool, exprhdl, m_pdrgpdrgpcrInput, pdrgpcrsOutput);

	// clean up
	pdrgpcrsOutput->Release();

	return pstats;
}

// EOF
