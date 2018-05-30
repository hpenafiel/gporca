//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 Pivotal, Inc.
//
//	@filename:
//		CLogicalJoin.cpp
//
//	@doc:
//		Implementation of logical join class
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/COptCtxt.h"

#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalJoin.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "naucrates/statistics/CJoinStatsProcessor.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalJoin::CLogicalJoin
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalJoin::CLogicalJoin
	(
	IMemoryPool *memory_pool
	)
	:
	CLogical(memory_pool)
{
	GPOS_ASSERT(NULL != memory_pool);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalJoin::FMatch
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CLogicalJoin::FMatch
	(
	COperator *pop
	)
	const
{
	return (pop->Eopid() == Eopid());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalJoin::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
IStatistics *
CLogicalJoin::PstatsDerive
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	DrgPstat *pdrgpstatCtxt
	)
	const
{
	return CJoinStatsProcessor::PstatsJoin(memory_pool, exprhdl, pdrgpstatCtxt);
}

// EOF

