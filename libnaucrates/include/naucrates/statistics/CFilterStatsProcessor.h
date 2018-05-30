//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 Pivotal, Inc.
//
//	@filename:
//		CFilterStatsProcessor.h
//
//	@doc:
//		Compute statistics for filter operation
//---------------------------------------------------------------------------
#ifndef GPNAUCRATES_CFilterStatsProcessor_H
#define GPNAUCRATES_CFilterStatsProcessor_H

#include "gpos/base.h"
#include "gpopt/operators/ops.h"
#include "gpopt/optimizer/COptimizerConfig.h"

#include "naucrates/statistics/CStatistics.h"
#include "naucrates/statistics/CFilterStatsProcessor.h"
#include "naucrates/statistics/CJoinStatsProcessor.h"
#include "naucrates/statistics/CStatisticsUtils.h"
#include "naucrates/statistics/CScaleFactorUtils.h"

namespace gpnaucrates
{

	class CFilterStatsProcessor
	{
		private:

			// create a new histogram after applying the filter that is not an AND/OR predicate
			static
			CHistogram *PhistSimpleFilter
				(
				IMemoryPool *memory_pool,
				CStatsPred *pstatspred,
				CBitSet *pbsFilterColIds,
				CHistogram *phistBefore,
				CDouble *pdScaleFactorLast,
				ULONG *pulColIdLast
				);

			// create a new histogram after applying a point filter
			static
			CHistogram *PhistPointFilter
				(
				IMemoryPool *memory_pool,
				CStatsPredPoint *pstatspred,
				CBitSet *pbsFilterColIds,
				CHistogram *phistBefore,
				CDouble *pdScaleFactorLast,
				ULONG *pulColIdLast
				);

			// create a new histogram after applying a LIKE filter
			static
			CHistogram *PhistLikeFilter
				(
				IMemoryPool *memory_pool,
				CStatsPredLike *pstatspred,
				CBitSet *pbsFilterColIds,
				CHistogram *phistBefore,
				CDouble *pdScaleFactorLast,
				ULONG *pulColIdLast
				);

			// create a new histogram for an unsupported predicate
			static
			CHistogram *PhistUnsupportedPred
				(
				IMemoryPool *memory_pool,
				CStatsPredUnsupported *pstatspred,
				CBitSet *pbsFilterColIds,
				CHistogram *phistBefore,
				CDouble *pdScaleFactorLast,
				ULONG *pulColIdLast
				);

			// create a new hash map of histograms after applying a conjunctive or disjunctive filter
			static
			HMUlHist *PhmulhistApplyConjOrDisjFilter
				(
				IMemoryPool *memory_pool,
				const CStatisticsConfig *pstatsconf,
				HMUlHist *phmulhistInput,
				CDouble dRowsInput,
				CStatsPred *pstatspred,
				CDouble *pdScaleFactor
				);

			// create new hash map of histograms after applying the conjunction predicate
			static
			HMUlHist *PhmulhistApplyConjFilter
				(
				IMemoryPool *memory_pool,
				const CStatisticsConfig *pstatsconf,
				HMUlHist *phmulhistIntermediate,
				CDouble dRowsInput,
				CStatsPredConj *pstatspredConj,
				CDouble *pdScaleFactor
				);

			// create new hash map of histograms after applying the disjunctive predicate
			static
			HMUlHist *PhmulhistApplyDisjFilter
				(
				IMemoryPool *memory_pool,
				const CStatisticsConfig *pstatsconf,
				HMUlHist *phmulhistInput,
				CDouble dRowsInput,
				CStatsPredDisj *pstatspred,
				CDouble *pdScaleFactor
				);

			// check if the column is a new column for statistic calculation
			static
			BOOL FNewStatsColumn(ULONG ulColId, ULONG ulColIdLast);

		public:

		// filter
		static
		CStatistics *PstatsFilter(IMemoryPool *memory_pool, const CStatistics *pstatsInput, CStatsPred *pstatspredBase, BOOL fCapNdvs);

		// derive statistics for filter operation based on given scalar expression
		static
		IStatistics *PstatsFilterForScalarExpr
						(
						IMemoryPool *memory_pool,
						CExpressionHandle &exprhdl,
						IStatistics *pstatsChild,
						CExpression *pexprScalarLocal, // filter expression on local columns only
						CExpression *pexprScalarOuterRefs, // filter expression involving outer references
						DrgPstat *pdrgpstatOuter
						);
	};
}

#endif // !GPNAUCRATES_CFilterStatsProcessor_H

// EOF

