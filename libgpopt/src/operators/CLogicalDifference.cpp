//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalDifference.cpp
//
//	@doc:
//		Implementation of Difference operator
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/base/CKeyCollection.h"
#include "gpopt/operators/CLogicalDifference.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalGbAgg.h"

#include "naucrates/statistics/CStatsPredUtils.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDifference::CLogicalDifference
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalDifference::CLogicalDifference
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
//		CLogicalDifference::CLogicalDifference
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalDifference::CLogicalDifference
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
//		CLogicalDifference::~CLogicalDifference
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalDifference::~CLogicalDifference()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDifference::Maxcard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalDifference::Maxcard
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

	return exprhdl.Pdprel(0)->Maxcard();
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDifference::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
COperator *
CLogicalDifference::PopCopyWithRemappedColumns
	(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr,
	BOOL fMustExist
	)
{
	DrgPcr *pdrgpcrOutput = CUtils::PdrgpcrRemap(memory_pool, m_pdrgpcrOutput, phmulcr, fMustExist);
	DrgDrgPcr *pdrgpdrgpcrInput = CUtils::PdrgpdrgpcrRemap(memory_pool, m_pdrgpdrgpcrInput, phmulcr, fMustExist);

	return GPOS_NEW(memory_pool) CLogicalDifference(memory_pool, pdrgpcrOutput, pdrgpdrgpcrInput);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDifference::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
CXformSet *
CLogicalDifference::PxfsCandidates
	(
	IMemoryPool *memory_pool
	)
	const
{
	CXformSet *pxfs = GPOS_NEW(memory_pool) CXformSet(memory_pool);
	(void) pxfs->ExchangeSet(CXform::ExfDifference2LeftAntiSemiJoin);
	return pxfs;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDifference::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
IStatistics *
CLogicalDifference::PstatsDerive
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	DrgPstat * // not used
	)
	const
{
	GPOS_ASSERT(Esp(exprhdl) > EspNone);

	// difference is transformed into an aggregate over a LASJ,
	// we follow the same route to compute statistics
	DrgPcrs *pdrgpcrsOutput = GPOS_NEW(memory_pool) DrgPcrs(memory_pool);
	const ULONG ulSize = m_pdrgpdrgpcrInput->Size();
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CColRefSet *pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool, (*m_pdrgpdrgpcrInput)[ul]);
		pdrgpcrsOutput->Append(pcrs);
	}

	IStatistics *pstatsOuter = exprhdl.Pstats(0);
	IStatistics *pstatsInner = exprhdl.Pstats(1);

	// construct the scalar condition for the LASJ
	CExpression *pexprScCond = CUtils::PexprConjINDFCond(memory_pool, m_pdrgpdrgpcrInput);

	// compute the statistics for LASJ
	CColRefSet *pcrsOuterRefs = exprhdl.Pdprel()->PcrsOuter();
	DrgPstatspredjoin *pdrgpstatspredjoin = CStatsPredUtils::Pdrgpstatspredjoin
														(
														memory_pool, 
														exprhdl, 
														pexprScCond, 
														pdrgpcrsOutput, 
														pcrsOuterRefs
														);
	IStatistics *pstatsLASJ = pstatsOuter->PstatsLASJoin
											(
											memory_pool,
											pstatsInner,
											pdrgpstatspredjoin,
											true /* fIgnoreLasjHistComputation */
											);

	// clean up
	pexprScCond->Release();
	pdrgpstatspredjoin->Release();

	// computed columns
	ULongPtrArray *pdrgpulComputedCols = GPOS_NEW(memory_pool) ULongPtrArray(memory_pool);
	IStatistics *pstats = CLogicalGbAgg::PstatsDerive
											(
											memory_pool,
											pstatsLASJ,
											(*m_pdrgpdrgpcrInput)[0], // we group by the columns of the first child
											pdrgpulComputedCols, // no computed columns for set ops
											NULL // no keys, use all grouping cols
											);

	// clean up
	pdrgpulComputedCols->Release();
	pstatsLASJ->Release();
	pdrgpcrsOutput->Release();

	return pstats;
}

// EOF
