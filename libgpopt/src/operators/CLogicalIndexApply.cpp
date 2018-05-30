//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 Pivotal, Inc.
//
//	Implementation of inner / left outer index apply operator
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpopt/operators/CLogicalIndexApply.h"
#include "naucrates/statistics/CJoinStatsProcessor.h"

using namespace gpopt;

CLogicalIndexApply::CLogicalIndexApply
	(
	IMemoryPool *memory_pool
	)
	:
	CLogicalApply(memory_pool),
	m_pdrgpcrOuterRefs(NULL),
	m_fOuterJoin(false)
{
	m_fPattern = true;
}

CLogicalIndexApply::CLogicalIndexApply
	(
	IMemoryPool *memory_pool,
	DrgPcr *pdrgpcrOuterRefs,
	BOOL fOuterJoin
	)
	:
	CLogicalApply(memory_pool),
	m_pdrgpcrOuterRefs(pdrgpcrOuterRefs),
	m_fOuterJoin(fOuterJoin)
{
	GPOS_ASSERT(NULL != pdrgpcrOuterRefs);
}


CLogicalIndexApply::~CLogicalIndexApply()
{
	CRefCount::SafeRelease(m_pdrgpcrOuterRefs);
}


CMaxCard
CLogicalIndexApply::Maxcard
	(
	IMemoryPool *, // memory_pool
	CExpressionHandle &exprhdl
	)
	const
{
	return CLogical::Maxcard(exprhdl, 2 /*ulScalarIndex*/, MaxcardDef(exprhdl));
}


CXformSet *
CLogicalIndexApply::PxfsCandidates
	(
	IMemoryPool *memory_pool
	)
	const
{
	CXformSet *pxfs = GPOS_NEW(memory_pool) CXformSet(memory_pool);
	(void) pxfs->ExchangeSet(CXform::ExfImplementIndexApply);
	return pxfs;
}

BOOL
CLogicalIndexApply::FMatch
	(
	COperator *pop
	)
	const
{
	GPOS_ASSERT(NULL != pop);

	if (pop->Eopid() == Eopid())
	{
		return m_pdrgpcrOuterRefs->Equals(CLogicalIndexApply::PopConvert(pop)->PdrgPcrOuterRefs());
	}

	return false;
}


IStatistics *
CLogicalIndexApply::PstatsDerive
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	DrgPstat* // pdrgpstatCtxt
	)
	const
{
	GPOS_ASSERT(EspNone < Esp(exprhdl));

	IStatistics *pstatsOuter = exprhdl.Pstats(0);
	IStatistics *pstatsInner = exprhdl.Pstats(1);
	CExpression *pexprScalar = exprhdl.PexprScalarChild(2 /*ulChildIndex*/);

	// join stats of the children
	DrgPstat *statistics_array = GPOS_NEW(memory_pool) DrgPstat(memory_pool);
	pstatsOuter->AddRef();
	statistics_array->Append(pstatsOuter);
	pstatsInner->AddRef();
	statistics_array->Append(pstatsInner);
	IStatistics::EStatsJoinType eStatsJoinType = IStatistics::EsjtInnerJoin;
	// we use Inner Join semantics here except in the case of Left Outer Join
	if (m_fOuterJoin)
	{
		eStatsJoinType = IStatistics::EsjtLeftOuterJoin;
	}
	IStatistics *pstats = CJoinStatsProcessor::PstatsJoinArray(memory_pool, statistics_array, pexprScalar, eStatsJoinType);
	statistics_array->Release();

	return pstats;
}

// return a copy of the operator with remapped columns
COperator *
CLogicalIndexApply::PopCopyWithRemappedColumns
(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr,
	BOOL fMustExist
	)
{
	DrgPcr *pdrgpcr = CUtils::PdrgpcrRemap(memory_pool, m_pdrgpcrOuterRefs, phmulcr, fMustExist);

	return GPOS_NEW(memory_pool) CLogicalIndexApply(memory_pool, pdrgpcr, m_fOuterJoin);
}

// EOF

