//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright 2018 Pivotal, Inc.
//
//	@filename:
//		CUnionAllStatsProcessor.cpp
//
//	@doc:
//		Statistics helper routines for processing union all operations
//---------------------------------------------------------------------------

#include "naucrates/statistics/CUnionAllStatsProcessor.h"
#include "naucrates/statistics/CStatisticsUtils.h"

using namespace gpopt;

// return statistics object after union all operation with input statistics object
CStatistics *
CUnionAllStatsProcessor::PstatsUnionAll
	(
	IMemoryPool *memory_pool,
	const CStatistics *pstatsFst,
	const CStatistics *pstatsSnd,
	ULongPtrArray *pdrgpulOutput,
	ULongPtrArray *pdrgpulInput1,
	ULongPtrArray *pdrgpulInput2
	)
{
	GPOS_ASSERT(NULL != memory_pool);
	GPOS_ASSERT(NULL != pstatsSnd);

	// lengths must match
	GPOS_ASSERT(pdrgpulOutput->Size() == pdrgpulInput1->Size());
	GPOS_ASSERT(pdrgpulOutput->Size() == pdrgpulInput2->Size());

	// create hash map from colid -> histogram for resultant structure
	HMUlHist *phmulhistNew = GPOS_NEW(memory_pool) HMUlHist(memory_pool);

	// column ids on which widths are to be computed
	HMUlDouble *phmuldoubleWidth = GPOS_NEW(memory_pool) HMUlDouble(memory_pool);

	BOOL fEmptyUnionAll = pstatsFst->IsEmpty() && pstatsSnd->IsEmpty();
	CColumnFactory *pcf = COptCtxt::PoctxtFromTLS()->Pcf();
	CDouble dRowsUnionAll = CStatistics::DMinRows;
	if (fEmptyUnionAll)
	{
		CHistogram::AddDummyHistogramAndWidthInfo(memory_pool, pcf, phmulhistNew, phmuldoubleWidth, pdrgpulOutput, true /*fEmpty*/);
	}
	else
	{
		const ULONG ulLen = pdrgpulOutput->Size();
		for (ULONG ul = 0; ul < ulLen; ul++)
		{
			ULONG ulColIdOutput = *(*pdrgpulOutput)[ul];
			ULONG ulColIdInput1 = *(*pdrgpulInput1)[ul];
			ULONG ulColIdInput2 = *(*pdrgpulInput2)[ul];

			const CHistogram *phistInput1 = pstatsFst->Phist(ulColIdInput1);
			GPOS_ASSERT(NULL != phistInput1);
			const CHistogram *phistInput2 = pstatsSnd->Phist(ulColIdInput2);
			GPOS_ASSERT(NULL != phistInput2);

			if (phistInput1->FWellDefined() || phistInput2->FWellDefined())
			{
				CHistogram *phistOutput = phistInput1->PhistUnionAllNormalized(memory_pool, pstatsFst->Rows(), phistInput2, pstatsSnd->Rows());
				CStatisticsUtils::AddHistogram(memory_pool, ulColIdOutput, phistOutput, phmulhistNew);
				GPOS_DELETE(phistOutput);
			}
			else
			{
				CColRef *pcr = pcf->PcrLookup(ulColIdOutput);
				GPOS_ASSERT(NULL != pcr);

				CHistogram *phistDummy = CHistogram::PhistDefault(memory_pool, pcr, false /* fEmpty*/);
				phmulhistNew->Insert(GPOS_NEW(memory_pool) ULONG(ulColIdOutput), phistDummy);
			}

			// look up width
			const CDouble *pdWidth = pstatsFst->PdWidth(ulColIdInput1);
			GPOS_ASSERT(NULL != pdWidth);
			phmuldoubleWidth->Insert(GPOS_NEW(memory_pool) ULONG(ulColIdOutput), GPOS_NEW(memory_pool) CDouble(*pdWidth));
		}

		dRowsUnionAll = pstatsFst->Rows() + pstatsSnd->Rows();
	}

	// release inputs
	pdrgpulOutput->Release();
	pdrgpulInput1->Release();
	pdrgpulInput2->Release();

	// create an output stats object
	CStatistics *pstatsUnionAll = GPOS_NEW(memory_pool) CStatistics
											(
											memory_pool,
											phmulhistNew,
											phmuldoubleWidth,
											dRowsUnionAll,
											fEmptyUnionAll,
											0 /* m_ulNumPredicates */
											);

	// In the output statistics object, the upper bound source cardinality of the UNION ALL column
	// is the estimate union all cardinality.

	// modify upper bound card information
	CStatisticsUtils::ComputeCardUpperBounds(memory_pool, pstatsFst, pstatsUnionAll, dRowsUnionAll, CStatistics::EcbmOutputCard /* ecbm */);

	return pstatsUnionAll;
}

// EOF
