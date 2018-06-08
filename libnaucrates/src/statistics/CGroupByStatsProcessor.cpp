//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright 2018 Pivotal, Inc.
//
//	@filename:
//		CGroupByStatsProcessor.cpp
//
//	@doc:
//		Statistics helper routines for processing group by operations
//---------------------------------------------------------------------------

#include "gpopt/operators/ops.h"
#include "gpopt/optimizer/COptimizerConfig.h"

#include "naucrates/statistics/CStatistics.h"
#include "naucrates/statistics/CGroupByStatsProcessor.h"
#include "naucrates/statistics/CStatisticsUtils.h"

using namespace gpopt;

// return statistics object after Group by computation
CStatistics *
CGroupByStatsProcessor::PstatsGroupBy
	(
	IMemoryPool *memory_pool,
	const CStatistics *pstatsInput,
	ULongPtrArray *pdrgpulGC,
	ULongPtrArray *pdrgpulAgg,
	CBitSet *pbsKeys
	)
{
	// create hash map from colid -> histogram for resultant structure
	HMUlHist *phmulhist = GPOS_NEW(memory_pool) HMUlHist(memory_pool);

	// hash map colid -> width
	HMUlDouble *phmuldoubleWidth = GPOS_NEW(memory_pool) HMUlDouble(memory_pool);

	CColumnFactory *pcf = COptCtxt::PoctxtFromTLS()->Pcf();

	CStatistics *pstatsAgg = NULL;
	CDouble dRowsAgg = CStatistics::DMinRows;
	if (pstatsInput->IsEmpty())
	{
		// add dummy histograms for the aggregates and grouping columns
		CHistogram::AddDummyHistogramAndWidthInfo(memory_pool, pcf, phmulhist, phmuldoubleWidth, pdrgpulAgg, true /* is_empty */);
		CHistogram::AddDummyHistogramAndWidthInfo(memory_pool, pcf, phmulhist, phmuldoubleWidth, pdrgpulGC, true /* is_empty */);

		pstatsAgg = GPOS_NEW(memory_pool) CStatistics(memory_pool, phmulhist, phmuldoubleWidth, dRowsAgg, true /* is_empty */);
	}
	else
	{
		// for computed aggregates, we're not going to be very smart right now
		CHistogram::AddDummyHistogramAndWidthInfo(memory_pool, pcf, phmulhist, phmuldoubleWidth, pdrgpulAgg, false /* is_empty */);

		CColRefSet *pcrsGrpColComputed = GPOS_NEW(memory_pool) CColRefSet(memory_pool);
		CColRefSet *pcrsGrpColsForStats = CStatisticsUtils::PcrsGrpColsForStats(memory_pool, pdrgpulGC, pcrsGrpColComputed);

		// add statistical information of columns (1) used to compute the cardinality of the aggregate
		// and (2) the grouping columns that are computed
		CStatisticsUtils::AddGrpColStats(memory_pool, pstatsInput, pcrsGrpColsForStats, phmulhist, phmuldoubleWidth);
		CStatisticsUtils::AddGrpColStats(memory_pool, pstatsInput, pcrsGrpColComputed, phmulhist, phmuldoubleWidth);

		const CStatisticsConfig *pstatsconf = pstatsInput->PStatsConf();

		DrgPdouble *pdrgpdNDV = CStatisticsUtils::PdrgPdoubleNDV(memory_pool, pstatsconf, pstatsInput, pcrsGrpColsForStats, pbsKeys);
		CDouble dGroups = CStatisticsUtils::DNumOfDistinctVal(pstatsconf, pdrgpdNDV);

		// clean up
		pcrsGrpColsForStats->Release();
		pcrsGrpColComputed->Release();
		pdrgpdNDV->Release();

		dRowsAgg = std::min(std::max(CStatistics::DMinRows.Get(), dGroups.Get()), pstatsInput->Rows().Get());

		// create a new stats object for the output
		pstatsAgg = GPOS_NEW(memory_pool) CStatistics(memory_pool, phmulhist, phmuldoubleWidth, dRowsAgg, pstatsInput->IsEmpty());
	}

	// In the output statistics object, the upper bound source cardinality of the grouping column
	// cannot be greater than the upper bound source cardinality information maintained in the input
	// statistics object. Therefore we choose CStatistics::EcbmMin the bounding method which takes
	// the minimum of the cardinality upper bound of the source column (in the input hash map)
	// and estimated group by cardinality.

	// modify source id to upper bound card information
	CStatisticsUtils::ComputeCardUpperBounds(memory_pool, pstatsInput, pstatsAgg, dRowsAgg, CStatistics::EcbmMin /* ecbm */);

	return pstatsAgg;
}

// EOF
