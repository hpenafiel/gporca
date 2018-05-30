//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalDifferenceAll.cpp
//
//	@doc:
//		Implementation of DifferenceAll operator
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/base/CKeyCollection.h"
#include "gpopt/operators/CLogicalDifferenceAll.h"
#include "gpopt/operators/CExpressionHandle.h"

#include "naucrates/statistics/CStatsPredUtils.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDifferenceAll::CLogicalDifferenceAll
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalDifferenceAll::CLogicalDifferenceAll
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
//		CLogicalDifferenceAll::CLogicalDifferenceAll
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalDifferenceAll::CLogicalDifferenceAll
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
//		CLogicalDifferenceAll::~CLogicalDifferenceAll
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalDifferenceAll::~CLogicalDifferenceAll()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDifferenceAll::Maxcard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalDifferenceAll::Maxcard
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
//		CLogicalDifferenceAll::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
COperator *
CLogicalDifferenceAll::PopCopyWithRemappedColumns
	(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr,
	BOOL fMustExist
	)
{
	DrgPcr *pdrgpcrOutput = CUtils::PdrgpcrRemap(memory_pool, m_pdrgpcrOutput, phmulcr, fMustExist);
	DrgDrgPcr *pdrgpdrgpcrInput = CUtils::PdrgpdrgpcrRemap(memory_pool, m_pdrgpdrgpcrInput, phmulcr, fMustExist);

	return GPOS_NEW(memory_pool) CLogicalDifferenceAll(memory_pool, pdrgpcrOutput, pdrgpdrgpcrInput);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDifferenceAll::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
CKeyCollection *
CLogicalDifferenceAll::PkcDeriveKeys
	(
	IMemoryPool *, // memory_pool,
	CExpressionHandle & //exprhdl
	)
	const
{
	// TODO: Add keys on columns contributing to the setop from the outer child
	return NULL;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDifferenceAll::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
IStatistics *
CLogicalDifferenceAll::PstatsDerive
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	DrgPstat * // not used
	)
	const
{
	GPOS_ASSERT(Esp(exprhdl) > EspNone);

	// difference all is transformed into a LASJ,
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
											true /* fIgnoreLasjHistComputation*/
											);

	// clean up
	pexprScCond->Release();
	pdrgpstatspredjoin->Release();
	pdrgpcrsOutput->Release();

	return pstatsLASJ;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDifferenceAll::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
CXformSet *
CLogicalDifferenceAll::PxfsCandidates
	(
	IMemoryPool *memory_pool
	)
	const
{
	CXformSet *pxfs = GPOS_NEW(memory_pool) CXformSet(memory_pool);
	(void) pxfs->ExchangeSet(CXform::ExfDifferenceAll2LeftAntiSemiJoin);
	return pxfs;
}

// EOF
