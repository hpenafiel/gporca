//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright 2018 Pivotal, Inc.
//
//	@filename:
//		CProjectStatsProcessor.cpp
//
//	@doc:
//		Statistics helper routines for processing project operations
//---------------------------------------------------------------------------

#include "naucrates/statistics/CProjectStatsProcessor.h"
#include "naucrates/statistics/CStatisticsUtils.h"

using namespace gpopt;


//  return a statistics object for a project operation
CStatistics *
CProjectStatsProcessor::PstatsProject
	(
	IMemoryPool *memory_pool,
	const CStatistics *pstatsInput,
	ULongPtrArray *pdrgpulProjColIds,
	HMUlDatum *phmuldatum
	)
{
	GPOS_ASSERT(NULL != pdrgpulProjColIds);

	CColumnFactory *pcf = COptCtxt::PoctxtFromTLS()->Pcf();

	// create hash map from colid -> histogram for resultant structure
	HMUlHist *phmulhistNew = GPOS_NEW(memory_pool) HMUlHist(memory_pool);

	// column ids on which widths are to be computed
	HMUlDouble *phmuldoubleWidth = GPOS_NEW(memory_pool) HMUlDouble(memory_pool);

	const ULONG ulLen = pdrgpulProjColIds->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		ULONG col_id = *(*pdrgpulProjColIds)[ul];
		const CHistogram *phist = pstatsInput->Phist(col_id);

		if (NULL == phist)
		{

			// create histogram for the new project column
			DrgPbucket *pdrgbucket = GPOS_NEW(memory_pool) DrgPbucket(memory_pool);
			CDouble null_freq = 0.0;

			BOOL fWellDefined = false;
			if (NULL != phmuldatum)
			{
				IDatum *pdatum = phmuldatum->Find(&col_id);
				if (NULL != pdatum)
				{
					fWellDefined = true;
					if (!pdatum->IsNull())
					{
						pdrgbucket->Append(CBucket::PbucketSingleton(memory_pool, pdatum));
					}
					else
					{
						null_freq = 1.0;
					}
				}
			}

			CHistogram *phistPrCol = NULL;
			CColRef *pcr = pcf->PcrLookup(col_id);
			GPOS_ASSERT(NULL != pcr);

			if (0 == pdrgbucket->Size() && IMDType::EtiBool == pcr->Pmdtype()->Eti())
			{
				pdrgbucket->Release();
			 	phistPrCol = CHistogram::PhistDefaultBoolColStats(memory_pool);
			}
			else
			{
				phistPrCol = GPOS_NEW(memory_pool) CHistogram
										(
										pdrgbucket,
										fWellDefined,
										null_freq,
										CHistogram::DDefaultNDVRemain,
										CHistogram::DDefaultNDVFreqRemain
										);
			}

			phmulhistNew->Insert(GPOS_NEW(memory_pool) ULONG(col_id), phistPrCol);
		}
		else
		{
			phmulhistNew->Insert(GPOS_NEW(memory_pool) ULONG(col_id), phist->PhistCopy(memory_pool));
		}

		// look up width
		const CDouble *pdWidth = pstatsInput->PdWidth(col_id);
		if (NULL == pdWidth)
		{
			CColRef *pcr = pcf->PcrLookup(col_id);
			GPOS_ASSERT(NULL != pcr);

			CDouble width = CStatisticsUtils::DDefaultColumnWidth(pcr->Pmdtype());
			phmuldoubleWidth->Insert(GPOS_NEW(memory_pool) ULONG(col_id), GPOS_NEW(memory_pool) CDouble(width));
		}
		else
		{
			phmuldoubleWidth->Insert(GPOS_NEW(memory_pool) ULONG(col_id), GPOS_NEW(memory_pool) CDouble(*pdWidth));
		}
	}

	CDouble dRowsInput = pstatsInput->Rows();
	// create an output stats object
	CStatistics *pstatsProject = GPOS_NEW(memory_pool) CStatistics
											(
											memory_pool,
											phmulhistNew,
											phmuldoubleWidth,
											dRowsInput,
											pstatsInput->IsEmpty(),
											pstatsInput->UlNumberOfPredicates()
											);

	// In the output statistics object, the upper bound source cardinality of the project column
	// is equivalent the estimate project cardinality.
	CStatisticsUtils::ComputeCardUpperBounds(memory_pool, pstatsInput, pstatsProject, dRowsInput, CStatistics::EcbmInputSourceMaxCard /* ecbm */);

	// add upper bound card information for the project columns
	CStatistics::CreateAndInsertUpperBoundNDVs(memory_pool, pstatsProject, pdrgpulProjColIds, dRowsInput);

	return pstatsProject;
}

// EOF
