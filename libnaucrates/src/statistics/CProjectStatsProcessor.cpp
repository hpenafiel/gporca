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
	IMemoryPool *pmp,
	const CStatistics *pstatsInput,
	ULongPtrArray *pdrgpulProjColIds,
	HMUlDatum *phmuldatum
	)
{
	GPOS_ASSERT(NULL != pdrgpulProjColIds);

	CColumnFactory *pcf = COptCtxt::PoctxtFromTLS()->Pcf();

	// create hash map from colid -> histogram for resultant structure
	HMUlHist *phmulhistNew = GPOS_NEW(pmp) HMUlHist(pmp);

	// column ids on which widths are to be computed
	HMUlDouble *phmuldoubleWidth = GPOS_NEW(pmp) HMUlDouble(pmp);

	const ULONG ulLen = pdrgpulProjColIds->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		ULONG ulColId = *(*pdrgpulProjColIds)[ul];
		const CHistogram *phist = pstatsInput->Phist(ulColId);

		if (NULL == phist)
		{

			// create histogram for the new project column
			DrgPbucket *pdrgbucket = GPOS_NEW(pmp) DrgPbucket(pmp);
			CDouble dNullFreq = 0.0;

			BOOL fWellDefined = false;
			if (NULL != phmuldatum)
			{
				IDatum *pdatum = phmuldatum->Find(&ulColId);
				if (NULL != pdatum)
				{
					fWellDefined = true;
					if (!pdatum->FNull())
					{
						pdrgbucket->Append(CBucket::PbucketSingleton(pmp, pdatum));
					}
					else
					{
						dNullFreq = 1.0;
					}
				}
			}

			CHistogram *phistPrCol = NULL;
			CColRef *pcr = pcf->PcrLookup(ulColId);
			GPOS_ASSERT(NULL != pcr);

			if (0 == pdrgbucket->Size() && IMDType::EtiBool == pcr->Pmdtype()->Eti())
			{
				pdrgbucket->Release();
			 	phistPrCol = CHistogram::PhistDefaultBoolColStats(pmp);
			}
			else
			{
				phistPrCol = GPOS_NEW(pmp) CHistogram
										(
										pdrgbucket,
										fWellDefined,
										dNullFreq,
										CHistogram::DDefaultNDVRemain,
										CHistogram::DDefaultNDVFreqRemain
										);
			}

			phmulhistNew->Insert(GPOS_NEW(pmp) ULONG(ulColId), phistPrCol);
		}
		else
		{
			phmulhistNew->Insert(GPOS_NEW(pmp) ULONG(ulColId), phist->PhistCopy(pmp));
		}

		// look up width
		const CDouble *pdWidth = pstatsInput->PdWidth(ulColId);
		if (NULL == pdWidth)
		{
			CColRef *pcr = pcf->PcrLookup(ulColId);
			GPOS_ASSERT(NULL != pcr);

			CDouble dWidth = CStatisticsUtils::DDefaultColumnWidth(pcr->Pmdtype());
			phmuldoubleWidth->Insert(GPOS_NEW(pmp) ULONG(ulColId), GPOS_NEW(pmp) CDouble(dWidth));
		}
		else
		{
			phmuldoubleWidth->Insert(GPOS_NEW(pmp) ULONG(ulColId), GPOS_NEW(pmp) CDouble(*pdWidth));
		}
	}

	CDouble dRowsInput = pstatsInput->DRows();
	// create an output stats object
	CStatistics *pstatsProject = GPOS_NEW(pmp) CStatistics
											(
											pmp,
											phmulhistNew,
											phmuldoubleWidth,
											dRowsInput,
											pstatsInput->IsEmpty(),
											pstatsInput->UlNumberOfPredicates()
											);

	// In the output statistics object, the upper bound source cardinality of the project column
	// is equivalent the estimate project cardinality.
	CStatisticsUtils::ComputeCardUpperBounds(pmp, pstatsInput, pstatsProject, dRowsInput, CStatistics::EcbmInputSourceMaxCard /* ecbm */);

	// add upper bound card information for the project columns
	CStatistics::CreateAndInsertUpperBoundNDVs(pmp, pstatsProject, pdrgpulProjColIds, dRowsInput);

	return pstatsProject;
}

// EOF
