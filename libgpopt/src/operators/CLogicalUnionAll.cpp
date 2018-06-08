//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalUnionAll.cpp
//
//	@doc:
//		Implementation of UnionAll operator
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CLogicalUnionAll.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "naucrates/statistics/CUnionAllStatsProcessor.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnionAll::CLogicalUnionAll
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalUnionAll::CLogicalUnionAll
	(
	IMemoryPool *memory_pool
	)
	:
	CLogicalUnion(memory_pool),
	m_ulScanIdPartialIndex(0)
{
	m_fPattern = true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnionAll::CLogicalUnionAll
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalUnionAll::CLogicalUnionAll
	(
	IMemoryPool *memory_pool,
	DrgPcr *pdrgpcrOutput,
	DrgDrgPcr *pdrgpdrgpcrInput,
	ULONG ulScanIdPartialIndex
	)
	:
	CLogicalUnion(memory_pool, pdrgpcrOutput, pdrgpdrgpcrInput),
	m_ulScanIdPartialIndex(ulScanIdPartialIndex)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnionAll::~CLogicalUnionAll
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalUnionAll::~CLogicalUnionAll()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnionAll::Maxcard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalUnionAll::Maxcard
	(
	IMemoryPool *, // memory_pool
	CExpressionHandle &exprhdl
	)
	const
{
	const ULONG arity = exprhdl.Arity();

	CMaxCard maxcard = exprhdl.Pdprel(0)->Maxcard();
	for (ULONG ul = 1; ul < arity; ul++)
	{
		maxcard += exprhdl.Pdprel(ul)->Maxcard();
	}

	return maxcard;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnionAll::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
COperator *
CLogicalUnionAll::PopCopyWithRemappedColumns
	(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr,
	BOOL fMustExist
	)
{
	DrgPcr *pdrgpcrOutput = CUtils::PdrgpcrRemap(memory_pool, m_pdrgpcrOutput, phmulcr, fMustExist);
	DrgDrgPcr *pdrgpdrgpcrInput = CUtils::PdrgpdrgpcrRemap(memory_pool, m_pdrgpdrgpcrInput, phmulcr, fMustExist);

	return GPOS_NEW(memory_pool) CLogicalUnionAll(memory_pool, pdrgpcrOutput, pdrgpdrgpcrInput, m_ulScanIdPartialIndex);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnionAll::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
CKeyCollection *
CLogicalUnionAll::PkcDeriveKeys
	(
	IMemoryPool *, //memory_pool,
	CExpressionHandle & // exprhdl
	)
	const
{
	return NULL;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnionAll::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
CXformSet *
CLogicalUnionAll::PxfsCandidates
	(
	IMemoryPool *memory_pool
	)
	const
{
	CXformSet *xform_set = GPOS_NEW(memory_pool) CXformSet(memory_pool);
	(void) xform_set->ExchangeSet(CXform::ExfImplementUnionAll);

	return xform_set;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnionAll::PstatsDeriveUnionAll
//
//	@doc:
//		Derive statistics based on union all semantics
//
//---------------------------------------------------------------------------
IStatistics *
CLogicalUnionAll::PstatsDeriveUnionAll
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl
	)
{
	GPOS_ASSERT(COperator::EopLogicalUnionAll == exprhdl.Pop()->Eopid() || COperator::EopLogicalUnion == exprhdl.Pop()->Eopid());

	DrgPcr *pdrgpcrOutput = CLogicalSetOp::PopConvert(exprhdl.Pop())->PdrgpcrOutput();
	DrgDrgPcr *pdrgpdrgpcrInput = CLogicalSetOp::PopConvert(exprhdl.Pop())->PdrgpdrgpcrInput();
	GPOS_ASSERT(NULL != pdrgpcrOutput);
	GPOS_ASSERT(NULL != pdrgpdrgpcrInput);

	IStatistics *pstatsResult = exprhdl.Pstats(0);
	pstatsResult->AddRef();
	const ULONG arity = exprhdl.Arity();
	for (ULONG ul = 1; ul < arity; ul++)
	{
		IStatistics *pstatsChild = exprhdl.Pstats(ul);
		CStatistics *pstats = CUnionAllStatsProcessor::PstatsUnionAll
											(
											memory_pool,
											dynamic_cast<CStatistics *>(pstatsResult),
											dynamic_cast<CStatistics *>(pstatsChild),
											CColRef::Pdrgpul(memory_pool, pdrgpcrOutput),
											CColRef::Pdrgpul(memory_pool, (*pdrgpdrgpcrInput)[0]),
											CColRef::Pdrgpul(memory_pool, (*pdrgpdrgpcrInput)[ul])
											);
		pstatsResult->Release();
		pstatsResult = pstats;
	}

	return pstatsResult;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalUnionAll::PstatsDerive
//
//	@doc:
//		Derive statistics based on union all semantics
//
//---------------------------------------------------------------------------
IStatistics *
CLogicalUnionAll::PstatsDerive
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	DrgPstat * // not used
	)
	const
{
	GPOS_ASSERT(EspNone < Esp(exprhdl));

	return PstatsDeriveUnionAll(memory_pool, exprhdl);
}

// EOF
