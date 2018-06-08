//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 Pivotal, Inc.
//
//	@filename:
//		CHistogram.cpp
//
//	@doc:
//		Implementation of single-dimensional histogram
//---------------------------------------------------------------------------


#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/statistics/CHistogram.h"
#include "naucrates/dxl/operators/CDXLScalarConstValue.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "gpos/io/COstreamString.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/common/syslibwrapper.h"

#include "naucrates/statistics/CStatistics.h"
#include "naucrates/statistics/CStatisticsUtils.h"
#include "naucrates/statistics/CLeftAntiSemiJoinStatsProcessor.h"
#include "naucrates/statistics/CScaleFactorUtils.h"

#include "gpopt/base/CColRef.h"

using namespace gpnaucrates;
using namespace gpopt;
using namespace gpmd;

// default histogram selectivity
const CDouble CHistogram::DDefaultSelectivity(0.4);

// default scale factor when there is no filtering of input
const CDouble CHistogram::DNeutralScaleFactor(1.0);

// the minimum number of distinct values in a column
const CDouble CHistogram::DMinDistinct(1.0);

// default Null frequency
const CDouble CHistogram::DDefaultNullFreq(0.0);

// default NDV remain
const CDouble CHistogram::DDefaultNDVRemain(0.0);

// default frequency of NDV remain
const CDouble CHistogram::DDefaultNDVFreqRemain(0.0);

// sample size used to estimate skew
#define GPOPT_SKEW_SAMPLE_SIZE 1000

// ctor
CHistogram::CHistogram
	(
	DrgPbucket *pdrgppbucket,
	BOOL fWellDefined
	)
	:
	m_pdrgppbucket(pdrgppbucket),
	m_fWellDefined(fWellDefined),
	m_null_freq(CHistogram::DDefaultNullFreq),
	m_distint_remaining(DDefaultNDVRemain),
	m_freq_remaining(DDefaultNDVFreqRemain),
	m_fSkewMeasured(false),
	m_dSkew(1.0),
	m_fNDVScaled(false),
	m_is_col_stats_missing(false)
{
	GPOS_ASSERT(NULL != pdrgppbucket);
}

// ctor
CHistogram::CHistogram
	(
	DrgPbucket *pdrgppbucket,
	BOOL fWellDefined,
	CDouble null_freq,
	CDouble distinct_remaining,
	CDouble freq_remaining,
	BOOL is_col_stats_missing
	)
	:
	m_pdrgppbucket(pdrgppbucket),
	m_fWellDefined(fWellDefined),
	m_null_freq(null_freq),
	m_distint_remaining(distinct_remaining),
	m_freq_remaining(freq_remaining),
	m_fSkewMeasured(false),
	m_dSkew(1.0),
	m_fNDVScaled(false),
	m_is_col_stats_missing(is_col_stats_missing)
{
	GPOS_ASSERT(m_pdrgppbucket);
	GPOS_ASSERT(CDouble(0.0) <= null_freq);
	GPOS_ASSERT(CDouble(1.0) >= null_freq);
	GPOS_ASSERT(CDouble(0.0) <= distinct_remaining);
	GPOS_ASSERT(CDouble(0.0) <= freq_remaining);
	GPOS_ASSERT(CDouble(1.0) >= freq_remaining);
	// if distinct_remaining is 0, freq_remaining must be 0 too
	GPOS_ASSERT_IMP(distinct_remaining < CStatistics::DEpsilon, freq_remaining < CStatistics::DEpsilon);
}

// set histograms null frequency
void
CHistogram::SetNullFrequency
	(
	CDouble null_freq
	)
{
	GPOS_ASSERT(CDouble(0.0) <= null_freq && CDouble(1.0) >= null_freq);
	m_null_freq = null_freq;
}

//	print function
IOstream&
CHistogram::OsPrint
	(
	IOstream &os
	)
	const
{
	os << std::endl << "[" << std::endl;

	ULONG ulNumBuckets = m_pdrgppbucket->Size();
	for (ULONG ulBucketIdx = 0; ulBucketIdx < ulNumBuckets; ulBucketIdx++)
	{
		os << "b" << ulBucketIdx << " = ";
		(*m_pdrgppbucket)[ulBucketIdx]->OsPrint(os);
		os << std::endl;
	}
	os << "]" << std::endl;

	os << "Null fraction: " << m_null_freq << std::endl;

	os << "Remaining NDV: " << m_distint_remaining << std::endl;

	os << "Remaining frequency: " << m_freq_remaining << std::endl;

	if (m_fSkewMeasured)
	{
		os << "Skew: " << m_dSkew << std::endl;
	}
	else
	{
		os << "Skew: not measured" << std::endl;
	}

	os << "Was NDVs re-scaled Based on Row Estimate: " << m_fNDVScaled << std::endl;

	return os;
}

// check if histogram is empty
BOOL
CHistogram::IsEmpty
	()
	const
{
	return (0 == m_pdrgppbucket->Size() && CStatistics::DEpsilon > m_null_freq && CStatistics::DEpsilon > m_distint_remaining);
}

// construct new histogram with less than or less than equal to filter
CHistogram *
CHistogram::PhistLessThanOrLessThanEqual
	(
	IMemoryPool *memory_pool,
	CStatsPred::EStatsCmpType escmpt,
	CPoint *ppoint
	)
	const
{
	GPOS_ASSERT(CStatsPred::EstatscmptL == escmpt || CStatsPred::EstatscmptLEq == escmpt);

	DrgPbucket *pdrgppbucketNew = GPOS_NEW(memory_pool) DrgPbucket(memory_pool);
	const ULONG ulNumBuckets = m_pdrgppbucket->Size();

	for (ULONG ulBucketIdx = 0; ulBucketIdx < ulNumBuckets; ulBucketIdx++)
	{
		CBucket *pbucket = (*m_pdrgppbucket)[ulBucketIdx];
		if (pbucket->FBefore(ppoint))
		{
			break;
		}
		else if (pbucket->FAfter(ppoint))
		{
			pdrgppbucketNew->Append(pbucket->PbucketCopy(memory_pool));
		}
		else
		{
			GPOS_ASSERT(pbucket->FContains(ppoint));
			CBucket *pbucketLast = pbucket->PbucketScaleUpper(memory_pool, ppoint, CStatsPred::EstatscmptLEq == escmpt /*fIncludeUpper*/);
			if (NULL != pbucketLast)
			{
				pdrgppbucketNew->Append(pbucketLast);
			}
			break;
		}
	}

	CDouble distinct_remaining = 0.0;
	CDouble freq_remaining = 0.0;
	if (CStatistics::DEpsilon < m_distint_remaining * DDefaultSelectivity)
	{
		distinct_remaining = m_distint_remaining * DDefaultSelectivity;
		freq_remaining = m_freq_remaining * DDefaultSelectivity;
	}

	return GPOS_NEW(memory_pool) CHistogram
					(
					pdrgppbucketNew,
					true, // fWellDefined
					CDouble(0.0), // fNullFreq
					distinct_remaining,
					freq_remaining
					);
}

// return an array buckets after applying non equality filter on the histogram buckets
DrgPbucket *
CHistogram::PdrgppbucketNEqual
	(
	IMemoryPool *memory_pool,
	CPoint *ppoint
	)
	const
{
	GPOS_ASSERT(NULL != ppoint);
	DrgPbucket *pdrgppbucketNew = GPOS_NEW(memory_pool) DrgPbucket(memory_pool);
	const ULONG ulNumBuckets = m_pdrgppbucket->Size();
	bool fPointNull = ppoint->Pdatum()->IsNull();

	for (ULONG ulBucketIdx = 0; ulBucketIdx < ulNumBuckets; ulBucketIdx++)
	{
		CBucket *pbucket = (*m_pdrgppbucket)[ulBucketIdx];

		if (pbucket->FContains(ppoint) && !fPointNull)
		{
			CBucket *pbucketLT = pbucket->PbucketScaleUpper(memory_pool, ppoint, false /*fIncludeUpper */);
			if (NULL != pbucketLT)
			{
				pdrgppbucketNew->Append(pbucketLT);
			}
			CBucket *pbucketGT = pbucket->PbucketGreaterThan(memory_pool, ppoint);
			if (NULL != pbucketGT)
			{
				pdrgppbucketNew->Append(pbucketGT);
			}
		}
		else
		{
			pdrgppbucketNew->Append(pbucket->PbucketCopy(memory_pool));
		}
	}

	return pdrgppbucketNew;
}

// construct new histogram with not equal filter
CHistogram *
CHistogram::PhistNEqual
	(
	IMemoryPool *memory_pool,
	CPoint *ppoint
	)
	const
{
	GPOS_ASSERT(NULL != ppoint);

	DrgPbucket *pdrgppbucket = PdrgppbucketNEqual(memory_pool, ppoint);
	CDouble null_freq(0.0);

	return GPOS_NEW(memory_pool) CHistogram(pdrgppbucket, true /*fWellDefined*/, null_freq, m_distint_remaining, m_freq_remaining);
}

// construct new histogram with IDF filter
CHistogram *
CHistogram::PhistIDF
	(
	IMemoryPool *memory_pool,
	CPoint *ppoint
	)
	const
{
	GPOS_ASSERT(NULL != ppoint);

	DrgPbucket *pdrgppbucket =  PdrgppbucketNEqual(memory_pool, ppoint);
	CDouble null_freq(0.0);
	if (!ppoint->Pdatum()->IsNull())
	{
		// (col IDF NOT-NULL-CONSTANT) means that null values will also be returned
		null_freq = m_null_freq;
	}

	return GPOS_NEW(memory_pool) CHistogram(pdrgppbucket, true /*fWellDefined*/, null_freq, m_distint_remaining, m_freq_remaining);
}

// return an array buckets after applying equality filter on the histogram buckets
DrgPbucket *
CHistogram::PdrgppbucketEqual
	(
	IMemoryPool *memory_pool,
	CPoint *ppoint
	)
	const
{
	GPOS_ASSERT(NULL != ppoint);

	DrgPbucket *pdrgppbucket = GPOS_NEW(memory_pool) DrgPbucket(memory_pool);

	if (ppoint->Pdatum()->IsNull())
	{
		return pdrgppbucket;
	}

	const ULONG ulNumBuckets = m_pdrgppbucket->Size();
	ULONG ulBucketIdx = 0;

	for (ulBucketIdx = 0; ulBucketIdx < ulNumBuckets; ulBucketIdx++)
	{
		CBucket *pbucket = (*m_pdrgppbucket)[ulBucketIdx];

		if (pbucket->FContains(ppoint))
		{
			if (pbucket->FSingleton())
			{
				// reuse existing bucket
				pdrgppbucket->Append(pbucket->PbucketCopy(memory_pool));
			}
			else
			{
				// scale containing bucket
				CBucket *pbucketLast = pbucket->PbucketSingleton(memory_pool, ppoint);
				pdrgppbucket->Append(pbucketLast);
			}
			break; // only one bucket can contain point
		}
	}

	return pdrgppbucket;
}

// construct new histogram with equality filter
CHistogram *
CHistogram::PhistEqual
	(
	IMemoryPool *memory_pool,
	CPoint *ppoint
	)
	const
{
	GPOS_ASSERT(NULL != ppoint);

	if (ppoint->Pdatum()->IsNull())
	{
		return GPOS_NEW(memory_pool) CHistogram
							(
							GPOS_NEW(memory_pool) DrgPbucket(memory_pool),
							true /* fWellDefined */,
							m_null_freq,
							DDefaultNDVRemain,
							DDefaultNDVFreqRemain
							);
	}
	
	DrgPbucket *pdrgppbucket =  PdrgppbucketEqual(memory_pool, ppoint);

	if (CStatistics::DEpsilon < m_distint_remaining && 0 == pdrgppbucket->Size()) // no match is found in the buckets
	{
		return GPOS_NEW(memory_pool) CHistogram
						(
						pdrgppbucket,
						true, // fWellDefined
						0.0, // null_freq
						1.0, // distinct_remaining
						std::min(CDouble(1.0), m_freq_remaining / m_distint_remaining) // freq_remaining
						);
	}

	return GPOS_NEW(memory_pool) CHistogram(pdrgppbucket);
}

// construct new histogram with INDF filter
CHistogram *
CHistogram::PhistINDF
	(
	IMemoryPool *memory_pool,
	CPoint *ppoint
	)
	const
{
	GPOS_ASSERT(NULL != ppoint);

	DrgPbucket *pdrgppbucket =  PdrgppbucketEqual(memory_pool, ppoint);
	const ULONG num_of_buckets = pdrgppbucket->Size();
	CDouble null_freq(0.0);
	if (ppoint->Pdatum()->IsNull())
	{
		GPOS_ASSERT(0 == num_of_buckets);
		// (col INDF NULL) means that only the null values will be returned
		null_freq = m_null_freq;
	}

	if (CStatistics::DEpsilon < m_distint_remaining && 0 == num_of_buckets) // no match is found in the buckets
	{
		return GPOS_NEW(memory_pool) CHistogram
						(
						pdrgppbucket,
						true, // fWellDefined
						null_freq,
						1.0, // distinct_remaining
						std::min(CDouble(1.0), m_freq_remaining / m_distint_remaining) // freq_remaining
						);
	}

	return GPOS_NEW(memory_pool) CHistogram
					(
					pdrgppbucket,
					true /* fWellDefined */,
					null_freq,
					CHistogram::DDefaultNDVRemain,
					CHistogram::DDefaultNDVFreqRemain
					);
}

// construct new histogram with greater than or greater than equal filter
CHistogram *
CHistogram::PhistGreaterThanOrGreaterThanEqual
	(
	IMemoryPool *memory_pool,
	CStatsPred::EStatsCmpType escmpt,
	CPoint *ppoint
	)
	const
{
	GPOS_ASSERT(CStatsPred::EstatscmptGEq == escmpt || CStatsPred::EstatscmptG == escmpt);

	DrgPbucket *pdrgppbucketNew = GPOS_NEW(memory_pool) DrgPbucket(memory_pool);
	const ULONG ulNumBuckets = m_pdrgppbucket->Size();

	// find first bucket that contains ppoint
	ULONG ulBucketIdx = 0;
	for (ulBucketIdx = 0; ulBucketIdx < ulNumBuckets; ulBucketIdx++)
	{
		CBucket *pbucket = (*m_pdrgppbucket)[ulBucketIdx];
		if (pbucket->FBefore(ppoint))
		{
			break;
		}
		if (pbucket->FContains(ppoint))
		{
			if (CStatsPred::EstatscmptGEq == escmpt)
			{
				// first bucket needs to be scaled down
				CBucket *pbucketFirst = pbucket->PbucketScaleLower(memory_pool, ppoint,  true /* fIncludeLower */);
				pdrgppbucketNew->Append(pbucketFirst);
			}
			else
			{
				CBucket *pbucketGT = pbucket->PbucketGreaterThan(memory_pool, ppoint);
				if (NULL != pbucketGT)
				{
					pdrgppbucketNew->Append(pbucketGT);
				}
			}
			ulBucketIdx++;
			break;
		}
	}

	// add rest of the buckets
	for (; ulBucketIdx < ulNumBuckets; ulBucketIdx++)
	{
		CBucket *pbucket = (*m_pdrgppbucket)[ulBucketIdx];
		pdrgppbucketNew->Append(pbucket->PbucketCopy(memory_pool));
	}

	CDouble distinct_remaining = 0.0;
	CDouble freq_remaining = 0.0;
	if (CStatistics::DEpsilon < m_distint_remaining * DDefaultSelectivity)
	{
		distinct_remaining = m_distint_remaining * DDefaultSelectivity;
		freq_remaining = m_freq_remaining * DDefaultSelectivity;
	}

	return GPOS_NEW(memory_pool) CHistogram
					(
					pdrgppbucketNew,
					true, // fWellDefined
					CDouble(0.0), // fNullFreq
					distinct_remaining,
					freq_remaining
					);
}

// sum of frequencies from buckets.
CDouble
CHistogram::DFrequency
	()
	const
{
	CDouble dFrequency(0.0);
	const ULONG num_of_buckets = m_pdrgppbucket->Size();
	for (ULONG ulBucketIdx = 0; ulBucketIdx < num_of_buckets; ulBucketIdx++)
	{
		CBucket *pbucket = (*m_pdrgppbucket)[ulBucketIdx];
		dFrequency = dFrequency + pbucket->DFrequency();
	}

	if (CStatistics::DEpsilon < m_null_freq)
	{
		dFrequency = dFrequency + m_null_freq;
	}

	return dFrequency + m_freq_remaining;
}

// sum of number of distinct values from buckets
CDouble
CHistogram::DDistinct
	()
	const
{
	CDouble dDistinct(0.0);
	const ULONG num_of_buckets = m_pdrgppbucket->Size();
	for (ULONG ulBucketIdx = 0; ulBucketIdx < num_of_buckets; ulBucketIdx++)
	{
		CBucket *pbucket = (*m_pdrgppbucket)[ulBucketIdx];
		dDistinct = dDistinct + pbucket->DDistinct();
	}
	CDouble dDistinctNull(0.0);
	if (CStatistics::DEpsilon < m_null_freq)
	{
		dDistinctNull = 1.0;
	}

	return dDistinct + dDistinctNull + m_distint_remaining;
}

// cap the total number of distinct values (NDVs) in buckets to the number of rows
void
CHistogram::CapNDVs
	(
	CDouble rows
	)
{
	const ULONG num_of_buckets = m_pdrgppbucket->Size();
	CDouble dDistinct = DDistinct();
	if (rows >= dDistinct)
	{
		// no need for capping
		return;
	}

	m_fNDVScaled = true;

	CDouble dScaleRatio = (rows / dDistinct).Get();
	for (ULONG ul = 0; ul < num_of_buckets; ul++)
	{
		CBucket *pbucket = (*m_pdrgppbucket)[ul];
		CDouble dDistinctBucket = pbucket->DDistinct();
		pbucket->SetDistinct(std::max(CHistogram::DMinDistinct.Get(), (dDistinctBucket * dScaleRatio).Get()));
	}

	m_distint_remaining = m_distint_remaining * dScaleRatio;
}

// sum of frequencies is approx 1.0
BOOL
CHistogram::FNormalized
	()
	const
{
	CDouble dFrequency = DFrequency();

	return (dFrequency > CDouble(1.0) - CStatistics::DEpsilon
			&& dFrequency < CDouble(1.0) + CStatistics::DEpsilon);
}

//	check if histogram is well formed? Checks are:
//		1. Buckets should be in order (if applicable)
//		2. Buckets should not overlap
//		3. Frequencies should add up to less than 1.0
BOOL
CHistogram::IsValid
	()
	const
{
	// frequencies should not add up to more than 1.0
	if (DFrequency() > CDouble(1.0) + CStatistics::DEpsilon)
	{
		return false;
	}

	for (ULONG ulBucketIdx = 1; ulBucketIdx < m_pdrgppbucket->Size(); ulBucketIdx++)
	{
		CBucket *pbucket = (*m_pdrgppbucket)[ulBucketIdx];
		CBucket *pbucketPrev = (*m_pdrgppbucket)[ulBucketIdx - 1];

		// the later bucket's lower point must be greater than or equal to
		// earlier bucket's upper point. Even if the underlying datum does not
		// support ordering, the check is safe.
		if (pbucket->PpLower()->FLessThan(pbucketPrev->PpUpper()))
		{
			return false;
		}
	}
	return true;
}

// construct new histogram with filter and normalize output histogram
CHistogram *
CHistogram::PhistFilterNormalized
	(
	IMemoryPool *memory_pool,
	CStatsPred::EStatsCmpType escmpt,
	CPoint *ppoint,
	CDouble *pdScaleFactor
	)
	const
{
	// if histogram is not well-defined, then result is not well defined
	if (!FWellDefined())
	{
		CHistogram *phistAfter =  GPOS_NEW(memory_pool) CHistogram(GPOS_NEW(memory_pool) DrgPbucket(memory_pool), false /* fWellDefined */);
		*pdScaleFactor = CDouble(1.0) / CHistogram::DDefaultSelectivity;
		return phistAfter;
	}

	CHistogram *phistAfter = PhistFilter(memory_pool, escmpt, ppoint);
	*pdScaleFactor = phistAfter->DNormalize();
	GPOS_ASSERT(phistAfter->IsValid());

	return phistAfter;
}

// construct new histogram by joining with another and normalize
// output histogram. If the join is not an equality join the function
// returns an empty histogram
CHistogram *
CHistogram::PhistJoinNormalized
	(
	IMemoryPool *memory_pool,
	CStatsPred::EStatsCmpType escmpt,
	CDouble rows,
	const CHistogram *phistOther,
	CDouble dRowsOther,
	CDouble *pdScaleFactor
	)
	const
{
	GPOS_ASSERT(NULL != phistOther);

	if (CStatsPred::EstatscmptEqNDV == escmpt)
	{
		*pdScaleFactor = std::max
								 (
								  std::max(CHistogram::DMinDistinct.Get(), DDistinct().Get()),
								  std::max(CHistogram::DMinDistinct.Get(), phistOther->DDistinct().Get())
								  );
		return PhistJoinEqualityNDV(memory_pool, phistOther);
	}

	BOOL fEqOrINDF = (CStatsPred::EstatscmptEq == escmpt || CStatsPred::EstatscmptINDF == escmpt);
	if (!fEqOrINDF)
	{
		*pdScaleFactor = CScaleFactorUtils::DDefaultScaleFactorNonEqualityJoin;

		if (CStatsPred::EstatscmptNEq == escmpt || CStatsPred::EstatscmptIDF == escmpt)
		{
			*pdScaleFactor = DInEqualityJoinScaleFactor(memory_pool, rows, phistOther, dRowsOther);
		}

		return PhistJoin(memory_pool, escmpt, phistOther);
	}

	// if either histogram is not well-defined, the result is not well defined
	if (!FWellDefined() || !phistOther->FWellDefined())
	{
		CHistogram *phistAfter =  GPOS_NEW(memory_pool) CHistogram(GPOS_NEW(memory_pool) DrgPbucket(memory_pool), false /* fWellDefined */);
		(*pdScaleFactor) = CDouble(std::min(rows.Get(), dRowsOther.Get()));

		return phistAfter;
	}
	
	CHistogram *phistAfter = PhistJoin(memory_pool, escmpt, phistOther);
	*pdScaleFactor = phistAfter->DNormalize();

	// based on Ramakrishnan and Gehrke, "Database Management Systems, Third Ed", page 484
	// the scaling factor of equality join is the MAX of the number of distinct
	// values in each of the inputs

	*pdScaleFactor = std::max
						(
						std::max(DMinDistinct.Get(), DDistinct().Get()),
						std::max(DMinDistinct.Get(), phistOther->DDistinct().Get())
						);

	CDouble dCartesianProduct = rows * dRowsOther;
	if (phistAfter->IsEmpty())
	{
		// if join histogram is empty for equality join condition
		// use Cartesian product size as scale factor
		*pdScaleFactor = dCartesianProduct;
	}

	if (CStatsPred::EstatscmptINDF == escmpt)
	{
		// if the predicate is INDF then we must count for the cartesian
		// product of NULL values in both the histograms
		CDouble dExpectedEqJoin = dCartesianProduct / *pdScaleFactor;
		CDouble dNulls = rows * m_null_freq;
		CDouble dNullsOther = dRowsOther * phistOther->DNullFreq();
		CDouble dExpectedINDF = dExpectedEqJoin + (dNulls * dNullsOther);
		*pdScaleFactor = dCartesianProduct / dExpectedINDF;
	}

	// bound scale factor by cross product
	*pdScaleFactor = std::min((*pdScaleFactor).Get(), dCartesianProduct.Get());

	GPOS_ASSERT(phistAfter->IsValid());
	return phistAfter;
}

// scalar factor of inequality (!=) join condition
CDouble
CHistogram::DInEqualityJoinScaleFactor
	(
	IMemoryPool *memory_pool,
	CDouble rows,
	const CHistogram *phistOther,
	CDouble dRowsOther
	)
	const
{
	GPOS_ASSERT(NULL != phistOther);

	CDouble dScaleFactor(1.0);

	// we compute the scale factor of the inequality join (!= aka <>)
	// from the scale factor of equi-join
	CHistogram *phistJoin = PhistJoinNormalized
								(
								memory_pool,
								CStatsPred::EstatscmptEq,
								rows,
								phistOther,
								dRowsOther,
								&dScaleFactor
								);
	GPOS_DELETE(phistJoin);

	CDouble dCartesianProduct = rows * dRowsOther;

	GPOS_ASSERT(CDouble(1.0) <= dScaleFactor);
	CDouble dSelectivityEq = 1 / dScaleFactor;
	CDouble dSelectivityNEq = (1 - dSelectivityEq);

	dScaleFactor = dCartesianProduct;
	if (CStatistics::DEpsilon < dSelectivityNEq)
	{
		dScaleFactor = 1 / dSelectivityNEq;
	}

	return dScaleFactor;
}

// construct new histogram by left anti semi join with another and normalize
// output histogram
CHistogram *
CHistogram::PhistLASJoinNormalized
	(
	IMemoryPool *memory_pool,
	CStatsPred::EStatsCmpType escmpt,
	CDouble rows,
	const CHistogram *phistOther,
	CDouble *pdScaleFactor,
	BOOL fIgnoreLasjHistComputation
	)
	const
{
	// if either histogram is not well-defined, the result is not well defined
	if (!FWellDefined() || !phistOther->FWellDefined())
	{
		CHistogram *phistAfter =  GPOS_NEW(memory_pool) CHistogram(GPOS_NEW(memory_pool) DrgPbucket(memory_pool), false /* fWellDefined */);
		(*pdScaleFactor) = CDouble(1.0);

		return phistAfter;
	}

	if (fIgnoreLasjHistComputation)
	{
		// TODO:  04/14/2012 : LASJ derivation is pretty aggressive.
		// simply return a copy of the histogram with a scale factor corresponding to default selectivity.
		CHistogram *phistAfter = PhistCopy(memory_pool);
		*pdScaleFactor = CDouble(1.0) / CHistogram::DDefaultSelectivity;
		GPOS_ASSERT(phistAfter->IsValid());

		return phistAfter;
	}

	CHistogram *phistAfter = PhistLASJ(memory_pool, escmpt, phistOther);
	*pdScaleFactor = phistAfter->DNormalize();

	if (CStatsPred::EstatscmptEq != escmpt && CStatsPred::EstatscmptINDF != escmpt)
	{
		// TODO: , June 6 2014, we currently only support join histogram computation
		// for equality and INDF predicates, we cannot accurately estimate
		// the scale factor of the other predicates
		*pdScaleFactor = CDouble(1.0) / CHistogram::DDefaultSelectivity;
	}
	*pdScaleFactor = std::min((*pdScaleFactor).Get(), rows.Get());
	GPOS_ASSERT(phistAfter->IsValid());

	return phistAfter;
}

// construct new histogram after applying the filter (no normalization)
CHistogram *
CHistogram::PhistFilter
	(
	IMemoryPool *memory_pool,
	CStatsPred::EStatsCmpType escmpt,
	CPoint *ppoint
	)
	const
{
	CHistogram *phistAfter = NULL;
	GPOS_ASSERT(FSupportsFilter(escmpt));

	switch(escmpt)
	{
		case CStatsPred::EstatscmptEq:
		{
			phistAfter = PhistEqual(memory_pool, ppoint);
			break;
		}
		case CStatsPred::EstatscmptINDF:
		{
			phistAfter = PhistINDF(memory_pool, ppoint);
			break;
		}
		case CStatsPred::EstatscmptL:
		case CStatsPred::EstatscmptLEq:
		{
			phistAfter = PhistLessThanOrLessThanEqual(memory_pool, escmpt, ppoint);
			break;
		}
		case CStatsPred::EstatscmptG:
		case CStatsPred::EstatscmptGEq:
		{
			phistAfter = PhistGreaterThanOrGreaterThanEqual(memory_pool, escmpt, ppoint);
			break;
		}
		case CStatsPred::EstatscmptNEq:
		{
			phistAfter = PhistNEqual(memory_pool, ppoint);
			break;
		}
		case CStatsPred::EstatscmptIDF:
		{
			phistAfter = PhistIDF(memory_pool, ppoint);
			break;
		}
		default:
		{
			GPOS_RTL_ASSERT_MSG(false, "Not supported. Must not be called.");
			break;
		}
	}
	return phistAfter;
}

// construct new histogram by joining with another histogram, no normalization
CHistogram *
CHistogram::PhistJoin
	(
	IMemoryPool *memory_pool,
	CStatsPred::EStatsCmpType escmpt,
	const CHistogram *phist
	)
	const
{
	GPOS_ASSERT(FSupportsJoinPred(escmpt));

	if (CStatsPred::EstatscmptEq == escmpt)
	{
		return PhistJoinEquality(memory_pool, phist);
	}

	if (CStatsPred::EstatscmptINDF == escmpt)
	{
		return PhistJoinINDF(memory_pool, phist);
	}

	// TODO:  Feb 24 2014, We currently only support creation of histogram for equi join
	return  GPOS_NEW(memory_pool) CHistogram( GPOS_NEW(memory_pool) DrgPbucket(memory_pool), false /* fWellDefined */);
}

// construct new histogram by LASJ with another histogram, no normalization
CHistogram *
CHistogram::PhistLASJ
	(
	IMemoryPool *memory_pool,
	CStatsPred::EStatsCmpType escmpt,
	const CHistogram *phist
	)
	const
{
	GPOS_ASSERT(NULL != phist);

	if (CStatsPred::EstatscmptEq != escmpt && CStatsPred::EstatscmptINDF != escmpt)
	{
		// TODO: , June 6 2014, we currently only support join histogram computation
		// for equality and INDF predicates, we return the original histogram
		return PhistCopy(memory_pool);
	}

	DrgPbucket *pdrgppbucketNew = GPOS_NEW(memory_pool) DrgPbucket(memory_pool);

	CBucket *pbucketLowerSplit = NULL;
	CBucket *pbucketUpperSplit = NULL;

	ULONG ul1 = 0; // index into this histogram
	ULONG ul2 = 0; // index into other histogram

	CBucket *pbucketCandidate = NULL;

	const ULONG ulBuckets1 = UlBuckets();
	const ULONG ulBuckets2 = phist->UlBuckets();

	while (ul1 < ulBuckets1 && ul2 < ulBuckets2)
	{
		// bucket from other histogram
		CBucket *pbucket2 = (*phist->m_pdrgppbucket) [ul2];

		// yet to choose a candidate
		GPOS_ASSERT(NULL == pbucketCandidate);

		if (NULL != pbucketUpperSplit)
		{
			pbucketCandidate = pbucketUpperSplit;
		}
		else
		{
			pbucketCandidate = (*m_pdrgppbucket)[ul1]->PbucketCopy(memory_pool); // candidate bucket in result histogram
			ul1++;
		}

		pbucketLowerSplit = NULL;
		pbucketUpperSplit = NULL;

		pbucketCandidate->Difference(memory_pool, pbucket2, &pbucketLowerSplit, &pbucketUpperSplit);

		if (NULL != pbucketLowerSplit)
		{
			pdrgppbucketNew->Append(pbucketLowerSplit);
		}

		// need to find a new candidate
		GPOS_DELETE(pbucketCandidate);
		pbucketCandidate = NULL;

		ul2++;
	}

	pbucketCandidate = pbucketUpperSplit;

	// if we looked at all the buckets from the other histogram, then add remaining buckets from
	// this histogram
	if (ul2 == ulBuckets2)
	{
		// candidate is part of the result
		if (NULL != pbucketCandidate)
		{
			pdrgppbucketNew->Append(pbucketCandidate);
		}

		CStatisticsUtils::AddRemainingBuckets(memory_pool, m_pdrgppbucket, pdrgppbucketNew, &ul1);
	}
	else
	{
		GPOS_DELETE(pbucketCandidate);
	}

	CDouble null_freq = CLeftAntiSemiJoinStatsProcessor::DNullFreqLASJ(escmpt, this, phist);

	return GPOS_NEW(memory_pool) CHistogram(pdrgppbucketNew, true /*fWellDefined*/, null_freq, m_distint_remaining, m_freq_remaining);
}

// scales frequencies on histogram so that they add up to 1.0.
// Returns the scaling factor that was employed. Should not be called on empty histogram.
CDouble
CHistogram::DNormalize()
{
	// trivially normalized
	if (UlBuckets() == 0 && CStatistics::DEpsilon > m_null_freq && CStatistics::DEpsilon > m_distint_remaining)
	{
		return CDouble(GPOS_FP_ABS_MAX);
	}

	CDouble dScaleFactor = std::max(DOUBLE(1.0), (CDouble(1.0) / DFrequency()).Get());

	for (ULONG ul = 0; ul < m_pdrgppbucket->Size(); ul++)
	{
		CBucket *pbucket = (*m_pdrgppbucket)[ul];
		pbucket->SetFrequency(pbucket->DFrequency() * dScaleFactor);
	}

	m_null_freq = m_null_freq * dScaleFactor;

	CDouble freq_remaining = std::min((m_freq_remaining * dScaleFactor).Get(), DOUBLE(1.0));
	if (CStatistics::DEpsilon > m_distint_remaining)
	{
		freq_remaining = 0.0;
	}
	m_freq_remaining = freq_remaining;

	return dScaleFactor;
}

// deep copy of histogram
CHistogram *
CHistogram::PhistCopy
	(
	IMemoryPool *memory_pool
	)
	const
{
	DrgPbucket *pdrgpbucket = GPOS_NEW(memory_pool) DrgPbucket(memory_pool);
	for (ULONG ul = 0; ul < m_pdrgppbucket->Size(); ul++)
	{
		CBucket *pbucket = (*m_pdrgppbucket)[ul];
		pdrgpbucket->Append(pbucket->PbucketCopy(memory_pool));
	}

	CHistogram *phistCopy = GPOS_NEW(memory_pool) CHistogram(pdrgpbucket, m_fWellDefined, m_null_freq, m_distint_remaining, m_freq_remaining);
	if (FScaledNDV())
	{
		phistCopy->SetNDVScaled();
	}

	return phistCopy;
}

// is statistics comparison type supported for filter?
BOOL
CHistogram::FSupportsFilter
	(
	CStatsPred::EStatsCmpType escmpt
	)
{
	// is the scalar comparison type one of <, <=, =, >=, >, <>, IDF, INDF
	switch (escmpt)
	{
		case CStatsPred::EstatscmptL:
		case CStatsPred::EstatscmptLEq:
		case CStatsPred::EstatscmptEq:
		case CStatsPred::EstatscmptGEq:
		case CStatsPred::EstatscmptG:
		case CStatsPred::EstatscmptNEq:
		case CStatsPred::EstatscmptIDF:
		case CStatsPred::EstatscmptINDF:
			return true;
		default:
			return false;
	}
}

// is comparison type supported for join?
BOOL
CHistogram::FSupportsJoinPred
	(
	CStatsPred::EStatsCmpType escmpt
	)
{
	return (CStatsPred::EstatscmptOther != escmpt);
}

// construct a new histogram with equality join
CHistogram *
CHistogram::PhistJoinEquality
	(
	IMemoryPool *memory_pool,
	const CHistogram *phist
	)
	const
{
	ULONG ul1 = 0; // index on buckets from this histogram
	ULONG ul2 = 0; // index on buckets from other histogram

	const ULONG ulBuckets1 = UlBuckets();
	const ULONG ulBuckets2 = phist->UlBuckets();

	CDouble dFreqJoinBuckets1(0.0);
	CDouble dFreqJoinBuckets2(0.0);

	CDouble distinct_remaining(0.0);
	CDouble freq_remaining(0.0);

	BOOL fNDVBasedJoinCardEstimation1 = FNDVBasedCardEstimation(this);
	BOOL fNDVBasedJoinCardEstimation2 = FNDVBasedCardEstimation(phist);

	if (fNDVBasedJoinCardEstimation1 || fNDVBasedJoinCardEstimation2)
	{
		return PhistJoinEqualityNDV(memory_pool, phist);
	}

	DrgPbucket *pdrgppbucketJoin = GPOS_NEW(memory_pool) DrgPbucket(memory_pool);
	while (ul1 < ulBuckets1 && ul2 < ulBuckets2)
	{
		CBucket *pbucket1 = (*m_pdrgppbucket)[ul1];
		CBucket *pbucket2 = (*phist->m_pdrgppbucket)[ul2];

		if (pbucket1->FIntersects(pbucket2))
		{
			CDouble dFreqIntersect1(0.0);
			CDouble dFreqIntersect2(0.0);

			CBucket *pbucketNew = pbucket1->PbucketIntersect(memory_pool, pbucket2, &dFreqIntersect1, &dFreqIntersect2);
			pdrgppbucketJoin->Append(pbucketNew);

			dFreqJoinBuckets1 = dFreqJoinBuckets1 + dFreqIntersect1;
			dFreqJoinBuckets2 = dFreqJoinBuckets2 + dFreqIntersect2;

			INT res = CBucket::ICompareUpperBounds(pbucket1, pbucket2);
			if (0 == res)
			{
				// both ubs are equal
				ul1++;
				ul2++;
			}
			else if (1 > res)
			{
				// pbucket1's ub is smaller than that of the ub of pbucket2
				ul1++;
			}
			else
			{
				ul2++;
			}
		}
		else if (pbucket1->FBefore(pbucket2))
		{
			// buckets do not intersect there one bucket is before the other
			ul1++;
		}
		else
		{
			GPOS_ASSERT(pbucket2->FBefore(pbucket1));
			ul2++;
		}
	}

	ComputeJoinNDVRemainInfo
		(
		this,
		phist,
		pdrgppbucketJoin,
		dFreqJoinBuckets1,
		dFreqJoinBuckets2,
		&distinct_remaining,
		&freq_remaining
		);

	return GPOS_NEW(memory_pool) CHistogram(pdrgppbucketJoin, true /*fWellDefined*/, 0.0 /*null_freq*/, distinct_remaining, freq_remaining);
}

// construct a new histogram for NDV based cardinality estimation
CHistogram *
CHistogram::PhistJoinEqualityNDV
(
 IMemoryPool *memory_pool,
 const CHistogram *phist
 )
const
{
	CDouble distinct_remaining(0.0);
	CDouble freq_remaining(0.0);
	DrgPbucket *pdrgppbucketJoin = GPOS_NEW(memory_pool) DrgPbucket(memory_pool);
	
	// compute the number of non-null distinct values in the input histograms
	CDouble dNDV1 = this->DDistinct();
	CDouble dFreqRemain1 = this->DFrequency();
	CDouble dNullFreq1 = this->DNullFreq();
	if (CStatistics::DEpsilon < dNullFreq1)
	{
		dNDV1 = std::max(CDouble(0.0), (dNDV1 - 1.0));
		dFreqRemain1 = dFreqRemain1 - dNullFreq1;
	}
	
	CDouble dNDV2 = phist->DDistinct();
	CDouble dFreqRemain2 = phist->DFrequency();
	CDouble dNullFreq2 = phist->DNullFreq();
	if (CStatistics::DEpsilon < phist->DNullFreq())
	{
		dNDV2 = std::max(CDouble(0.0), (dNDV2 - 1.0));
		dFreqRemain2 = dFreqRemain2 - dNullFreq2;
	}
	
	// the estimated number of distinct value is the minimum of the non-null
	// distinct values of the two inputs.
	distinct_remaining = std::min(dNDV1, dNDV2);
	
	// the frequency of a tuple in this histogram (with frequency dFreqRemain1) joining with
	// a tuple in another relation (with frequency dFreqRemain2) is a product of the two frequencies divided by
	// the maximum NDV of the two inputs
	
	// Example: consider two relations A and B with 10 tuples each. Let both relations have no nulls.
	// Let A have 2 distinct values, while B have 5 distinct values. Under uniform distribution of NDVs
	// for statistics purposes we can view A = (1,2,1,2,1,2,1,2,1,2) and B = (1,2,3,4,5,1,2,3,4,5)
	// Join Cardinality is 20, with frequency of the join tuple being 0.2 (since cartesian product is 100).
	// dFreqRemain1 = dFreqRemain2 = 1, and std::max(dNDV1, dNDV2) = 5. Therefore freq_remaining = 1/5 = 0.2
	if (CStatistics::DEpsilon < distinct_remaining)
	{
		freq_remaining = dFreqRemain1 * dFreqRemain2 / std::max(dNDV1, dNDV2);
	}

	return GPOS_NEW(memory_pool) CHistogram
	(
	 pdrgppbucketJoin,
	 true /*fWellDefined*/,
	 0.0 /*null_freq*/,
	 distinct_remaining,
	 freq_remaining
	 );
}

// construct a new histogram for an INDF join predicate
CHistogram *
CHistogram::PhistJoinINDF
	(
	IMemoryPool *memory_pool,
	const CHistogram *phist
	)
	const
{
	CHistogram *phistJoin = PhistJoinEquality(memory_pool, phist);

	// compute the null frequency is the same means as how we perform equi-join
	// see CBuckets::PbucketIntersect for details
	CDouble null_freq = DNullFreq() * phist->DNullFreq() *
			DOUBLE(1.0) / std::max(DDistinct(), phist->DDistinct());
	phistJoin->SetNullFrequency(null_freq);

	return phistJoin;
}

// check if we can compute NDVRemain for JOIN histogram for the given input histograms
BOOL
CHistogram::FCanComputeJoinNDVRemain
	(
	const CHistogram *phist1,
	const CHistogram *phist2
	)
{
	GPOS_ASSERT(NULL != phist1);
	GPOS_ASSERT(NULL != phist2);

	BOOL fHasBuckets1 = (0 != phist1->UlBuckets());
	BOOL fHasBuckets2 = (0 != phist2->UlBuckets());
	BOOL fHasDistinctRemain1 = CStatistics::DEpsilon < phist1->DDistinctRemain();
	BOOL fHasDistinctRemain2 = CStatistics::DEpsilon < phist2->DDistinctRemain();

	if (!fHasDistinctRemain1 && !fHasDistinctRemain2)
	{
		// no remaining tuples information hence no need compute NDVRemain for join histogram
		return false;
	}

	if ((fHasDistinctRemain1 || fHasBuckets1) && (fHasDistinctRemain2 || fHasBuckets2))
	{
		//
		return true;
	}

	// insufficient information to compute JoinNDVRemain, we need:
	// 1. for each input either have a histograms or NDVRemain
	// 2. at least one inputs must have NDVRemain
	return false;
}

// compute the effects of the NDV and frequency of the tuples not captured by the histogram
void
CHistogram::ComputeJoinNDVRemainInfo
	(
	const CHistogram *phist1,
	const CHistogram *phist2,
	DrgPbucket *pdrgppbucketJoin,
	CDouble dFreqJoinBuckets1,
	CDouble dFreqJoinBuckets2,
	CDouble *pdDistinctRemain,
	CDouble *pdFreqRemain
	)
{
	GPOS_ASSERT(NULL != phist1);
	GPOS_ASSERT(NULL != phist2);
	GPOS_ASSERT(NULL != pdrgppbucketJoin);

	CDouble dNDVJoin = CStatisticsUtils::DDistinct(pdrgppbucketJoin);
	CDouble dFreqJoin = CStatisticsUtils::DFrequency(pdrgppbucketJoin);

	*pdDistinctRemain = 0.0;
	*pdFreqRemain = 0.0;

	if (!FCanComputeJoinNDVRemain(phist1, phist2))
	{
		return;
	}

	if (CStatistics::DEpsilon >= (1 - dFreqJoin))
	{
		return;
	}

	// we compute the following information for the resulting join histogram
	// 1. NDVRemain and 2. Frequency of the NDVRemain

	// compute the number of non-null distinct values in each input histogram
	CDouble dDistinct1 = phist1->DDistinct();
	CDouble dDistinctNonNull1 = dDistinct1;
	if (CStatistics::DEpsilon < phist1->DNullFreq())
	{
		dDistinctNonNull1 = dDistinctNonNull1 - 1.0;
	}

	CDouble dDistinct2 = phist2->DDistinct();
	CDouble dDistinctNonNull2 = dDistinct2;
	if (CStatistics::DEpsilon < phist2->DNullFreq())
	{
		dDistinctNonNull2 = dDistinctNonNull2 - 1.0;
	}

	// the estimated final number of distinct value for the join is the minimum of the non-null
	// distinct values of the two inputs. This follows the principle of used to estimate join
	// scaling factor -- defined as the maximum NDV of the two inputs
	CDouble dNDVJoinFinal = std::min(dDistinctNonNull1, dDistinctNonNull2);
	CDouble dNDVJoinRemain = dNDVJoinFinal - dNDVJoin;

	// compute the frequency of the non-joining buckets in each input histogram
	CDouble dFreqBuckets1 =  CStatisticsUtils::DFrequency(phist1->ParseDXLToBucketsArray());
	CDouble dFreqBuckets2 =  CStatisticsUtils::DFrequency(phist2->ParseDXLToBucketsArray());
	CDouble dFreqNonJoinBuckets1 = std::max(CDouble(0), (dFreqBuckets1 - dFreqJoinBuckets1));
	CDouble dFreqNonJoinBuckets2 = std::max(CDouble(0), (dFreqBuckets2 - dFreqJoinBuckets2));

	// compute the NDV of the non-joining buckets
	CDouble dNDVNonJoinBuckets1 = CStatisticsUtils::DDistinct(phist1->ParseDXLToBucketsArray()) - dNDVJoin;
	CDouble dNDVNonJoinBuckets2 = CStatisticsUtils::DDistinct(phist2->ParseDXLToBucketsArray()) - dNDVJoin;

	CDouble dFreqRemain1 = phist1->DFreqRemain();
	CDouble dFreqRemain2 = phist2->DFreqRemain();

	// the frequency of the NDVRemain of the join is made of:
	// 1. frequency of the NDV as a result of joining NDVRemain1 and NDVRemain2
	// 2. frequency of the NDV as a results of joining NDVRemain1 and dNDVNonJoinBuckets2
	// 3. frequency of the NDV as a results of joining NDVRemain2 and dNDVNonJoinBuckets1

	CDouble dFreqJoinRemain = dFreqRemain1 * dFreqRemain2 / std::max(phist1->DDistinctRemain(), phist2->DDistinctRemain());
	dFreqJoinRemain = dFreqJoinRemain + dFreqRemain1 * dFreqNonJoinBuckets2 / std::max(dDistinctNonNull1, dNDVNonJoinBuckets2);
	dFreqJoinRemain = dFreqJoinRemain + dFreqRemain2 * dFreqNonJoinBuckets1 / std::max(dDistinctNonNull2, dNDVNonJoinBuckets1);

	*pdDistinctRemain = dNDVJoinRemain;
	*pdFreqRemain = dFreqJoinRemain;
}

// construct new histogram by removing duplicates and normalize output histogram
CHistogram *
CHistogram::PhistGroupByNormalized
	(
	IMemoryPool *memory_pool,
	CDouble, // rows,
	CDouble *pdDistinctValues
	)
	const
{
	// if either histogram is not well-defined, the result is not well defined
	if (!FWellDefined())
	{
		CHistogram *phistAfter =  GPOS_NEW(memory_pool) CHistogram(GPOS_NEW(memory_pool) DrgPbucket(memory_pool), false /* fWellDefined */);
		*pdDistinctValues = DMinDistinct / CHistogram::DDefaultSelectivity;
		return phistAfter;
	}

	// total number of distinct values
	CDouble dDistinct = DDistinct();

	DrgPbucket *pdrgppbucketNew = GPOS_NEW(memory_pool) DrgPbucket(memory_pool);

	const ULONG num_of_buckets = m_pdrgppbucket->Size();
	for (ULONG ul = 0; ul < num_of_buckets; ul++)
	{
		CBucket *pbucket = (*m_pdrgppbucket)[ul];
		CPoint *ppLower = pbucket->PpLower();
		CPoint *ppUpper = pbucket->PpUpper();
		ppLower->AddRef();
		ppUpper->AddRef();

		BOOL fUpperClosed = false;
		if (ppLower->Equals(ppUpper))
		{
			fUpperClosed = true;
		}
		CBucket *pbucketNew = GPOS_NEW(memory_pool) CBucket
										(
										ppLower,
										ppUpper, 
										true /* fClosedLower */, 
										fUpperClosed,
										pbucket->DDistinct() / dDistinct,
										pbucket->DDistinct()
										);
		pdrgppbucketNew->Append(pbucketNew);
	}

	CDouble dNullFreqNew = CDouble(0.0);
	if (CStatistics::DEpsilon < m_null_freq)
	{
		dNullFreqNew = std::min(CDouble(1.0), CDouble(1.0)/dDistinct);
	}


	CDouble freq_remaining = 0.0;
	if (CStatistics::DEpsilon < m_distint_remaining)
	{
		// TODO:  11/22/2013 - currently the NDV of a histogram could be 0 or a decimal number.
		// We may not need the capping if later dDistinct is lower bounded at 1.0 for non-empty histogram
		freq_remaining = std::min(CDouble(1.0), m_distint_remaining/dDistinct);
	}

	CHistogram *phistAfter = GPOS_NEW(memory_pool) CHistogram(pdrgppbucketNew, true /*fWellDefined*/, dNullFreqNew, m_distint_remaining, freq_remaining);
	*pdDistinctValues = phistAfter->DDistinct();

	return phistAfter;
}

// construct new histogram based on union all operation
CHistogram *
CHistogram::PhistUnionAllNormalized
	(
	IMemoryPool *memory_pool,
	CDouble rows,
	const CHistogram *phist,
	CDouble dRowsOther
	)
	const
{
	DrgPbucket *pdrgppbucketNew = GPOS_NEW(memory_pool) DrgPbucket(memory_pool);
	ULONG ul1 = 0; // index on buckets from this histogram
	ULONG ul2 = 0; // index on buckets from other histogram
	CBucket *pbucket1 = (*this) [ul1];
	CBucket *pbucket2 = (*phist) [ul2];
	CDouble dRowsNew = (dRowsOther + rows);

	// flags to determine if the buckets where residue of the bucket-merge operation
	BOOL fbucket1Residual = false;
	BOOL fbucket2Residual = false;

	while (NULL != pbucket1 && NULL != pbucket2)
	{
		if (pbucket1->FBefore(pbucket2))
		{
			pdrgppbucketNew->Append(pbucket1->PbucketUpdateFrequency(memory_pool, rows, dRowsNew));
			CleanupResidualBucket(pbucket1, fbucket1Residual);
			ul1++;
			pbucket1 = (*this) [ul1];
			fbucket1Residual = false;
		}
		else if (pbucket2->FBefore(pbucket1))
		{
			pdrgppbucketNew->Append(pbucket2->PbucketUpdateFrequency(memory_pool, dRowsOther, dRowsNew));
			CleanupResidualBucket(pbucket2, fbucket2Residual);
			ul2++;
			pbucket2 = (*phist) [ul2];
			fbucket2Residual = false;
		}
		else
		{
			GPOS_ASSERT(pbucket1->FIntersects(pbucket2));
			CBucket *pbucket1New = NULL;
			CBucket *pbucket2New = NULL;
			CBucket *pbucketMerge = pbucket1->PbucketMerge(memory_pool, pbucket2, rows, dRowsOther, &pbucket1New, &pbucket2New);
			pdrgppbucketNew->Append(pbucketMerge);

			GPOS_ASSERT(NULL == pbucket1New || NULL == pbucket2New);
			CleanupResidualBucket(pbucket1, fbucket1Residual);
			CleanupResidualBucket(pbucket2, fbucket2Residual);

			pbucket1 = PbucketNext(this, pbucket1New, &fbucket1Residual, &ul1);
			pbucket2 = PbucketNext(phist, pbucket2New, &fbucket2Residual, &ul2);
		}
	}

	const ULONG ulBuckets1 = UlBuckets();
	const ULONG ulBuckets2 = phist->UlBuckets();

	GPOS_ASSERT_IFF(NULL == pbucket1, ul1 == ulBuckets1);
	GPOS_ASSERT_IFF(NULL == pbucket2, ul2 == ulBuckets2);

	ul1 = UlAddResidualUnionAllBucket(memory_pool, pdrgppbucketNew, pbucket1, dRowsOther, dRowsNew, fbucket1Residual, ul1);
	ul2 = UlAddResidualUnionAllBucket(memory_pool, pdrgppbucketNew, pbucket2, rows, dRowsNew, fbucket2Residual, ul2);

	CleanupResidualBucket(pbucket1, fbucket1Residual);
	CleanupResidualBucket(pbucket2, fbucket2Residual);

	// add any leftover buckets from other histogram
	AddBuckets(memory_pool, phist->m_pdrgppbucket, pdrgppbucketNew, dRowsOther, dRowsNew, ul2, ulBuckets2);

	// add any leftover buckets from this histogram
	AddBuckets(memory_pool, m_pdrgppbucket, pdrgppbucketNew, rows, dRowsNew, ul1, ulBuckets1);

	CDouble dNullFreqNew = (m_null_freq * rows + phist->m_null_freq * dRowsOther) / dRowsNew;

	CDouble distinct_remaining = std::max(m_distint_remaining, phist->m_distint_remaining);
	CDouble freq_remaining = (m_freq_remaining * rows + phist->m_freq_remaining * dRowsOther) / dRowsNew;

	CHistogram *phistResult = GPOS_NEW(memory_pool) CHistogram(pdrgppbucketNew, true /*fWellDefined*/, dNullFreqNew, distinct_remaining, freq_remaining);
	(void) phistResult->DNormalize();

	return phistResult;
}

// add residual bucket in the union all operation to the array of buckets in the histogram
ULONG
CHistogram::UlAddResidualUnionAllBucket
	(
	IMemoryPool *memory_pool,
	DrgPbucket *pdrgppbucket,
	CBucket *pbucket,
	CDouble dRowsOld,
	CDouble dRowsNew,
	BOOL fbucketResidual,
	ULONG ulIndex
	)
	const
{
	GPOS_ASSERT(NULL != pdrgppbucket);

	if (fbucketResidual)
	{
		pdrgppbucket->Append(pbucket->PbucketUpdateFrequency(memory_pool, dRowsOld, dRowsNew));
		return ulIndex + 1;
	}

	return ulIndex;
}

// add buckets from one array to another
void
CHistogram::AddBuckets
	(
	IMemoryPool *memory_pool,
	DrgPbucket *pdrgppbucketSrc,
	DrgPbucket *pdrgppbucketDest,
	CDouble dRowsOld,
	CDouble dRowsNew,
	ULONG ulBegin,
	ULONG ulEnd
	)
{
	GPOS_ASSERT(NULL != pdrgppbucketSrc);
	GPOS_ASSERT(NULL != pdrgppbucketDest);
	GPOS_ASSERT(ulBegin <= ulEnd);
	GPOS_ASSERT(ulEnd <= pdrgppbucketSrc->Size());

	for (ULONG ul = ulBegin; ul < ulEnd; ul++)
	{
		pdrgppbucketDest->Append(((*pdrgppbucketSrc)[ul])->PbucketUpdateFrequency(memory_pool, dRowsOld, dRowsNew));
	}
}

// cleanup residual buckets
void
CHistogram::CleanupResidualBucket
	(
	CBucket *pbucket,
	BOOL fbucketResidual
	)
	const
{
	if (NULL != pbucket && fbucketResidual)
	{
		GPOS_DELETE(pbucket);
		pbucket = NULL;
	}
}

// get the next bucket for union / union all
CBucket *
CHistogram::PbucketNext
	(
	const CHistogram *phist,
	CBucket *pbucketNew,
	BOOL *pfbucketResidual,
	ULONG *pulBucketIndex
	)
	const
{
	GPOS_ASSERT(NULL != phist);
	if (NULL != pbucketNew)
	{
		*pfbucketResidual = true;
		return pbucketNew;
	}

	*pulBucketIndex = *pulBucketIndex  + 1;
	*pfbucketResidual = false;

	return (*phist) [*pulBucketIndex];
}

//	construct new histogram based on union operation
CHistogram *
CHistogram::PhistUnionNormalized
	(
	IMemoryPool *memory_pool,
	CDouble rows,
	const CHistogram *phistOther,
	CDouble dRowsOther,
	CDouble *pdRowsOutput
	)
	const
{
	GPOS_ASSERT(NULL != phistOther);

	ULONG ul1 = 0; // index on buckets from this histogram
	ULONG ul2 = 0; // index on buckets from other histogram
	CBucket *pbucket1 = (*this) [ul1];
	CBucket *pbucket2 = (*phistOther) [ul2];

	// flags to determine if the buckets where residue of the bucket-merge operation
	BOOL fbucket1Residual = false;
	BOOL fbucket2Residual = false;

	// array of buckets in the resulting histogram
	DrgPbucket *pdrgppbucket = GPOS_NEW(memory_pool) DrgPbucket(memory_pool);

	// number of tuples in each bucket of the resulting histogram
	DrgPdouble *pdrgpdoubleRows = GPOS_NEW(memory_pool) DrgPdouble(memory_pool);

	CDouble dRowCummulative(0.0);
	while (NULL != pbucket1 && NULL != pbucket2)
	{
		if (pbucket1->FBefore(pbucket2))
		{
			pdrgppbucket->Append(pbucket1->PbucketCopy(memory_pool));
			pdrgpdoubleRows->Append(GPOS_NEW(memory_pool) CDouble(pbucket1->DFrequency() * rows));
			CleanupResidualBucket(pbucket1, fbucket1Residual);
			ul1++;
			pbucket1 = (*this) [ul1];
			fbucket1Residual = false;
		}
		else if (pbucket2->FBefore(pbucket1))
		{
			pdrgppbucket->Append(pbucket2->PbucketCopy(memory_pool));
			pdrgpdoubleRows->Append(GPOS_NEW(memory_pool) CDouble(pbucket2->DFrequency() * dRowsOther));
			CleanupResidualBucket(pbucket2, fbucket2Residual);
			ul2++;
			pbucket2 = (*phistOther) [ul2];
			fbucket2Residual = false;
		}
		else
		{
			GPOS_ASSERT(pbucket1->FIntersects(pbucket2));
			CBucket *pbucket1New = NULL;
			CBucket *pbucket2New = NULL;
			CBucket *pbucketMerge = NULL;

			pbucketMerge = pbucket1->PbucketMerge
									(
									memory_pool,
									pbucket2,
									rows,
									dRowsOther,
									&pbucket1New,
									&pbucket2New,
									false /* fUnionAll */
									);

			// add the estimated number of rows in the merged bucket
			pdrgpdoubleRows->Append(GPOS_NEW(memory_pool) CDouble(pbucketMerge->DFrequency() * rows));
			pdrgppbucket->Append(pbucketMerge);

			GPOS_ASSERT(NULL == pbucket1New || NULL == pbucket2New);
			CleanupResidualBucket(pbucket1, fbucket1Residual);
			CleanupResidualBucket(pbucket2, fbucket2Residual);

			pbucket1 = PbucketNext(this, pbucket1New, &fbucket1Residual, &ul1);
			pbucket2 = PbucketNext(phistOther, pbucket2New, &fbucket2Residual, &ul2);
		}
	}

	const ULONG ulBuckets1 = UlBuckets();
	const ULONG ulBuckets2 = phistOther->UlBuckets();

	GPOS_ASSERT_IFF(NULL == pbucket1, ul1 == ulBuckets1);
	GPOS_ASSERT_IFF(NULL == pbucket2, ul2 == ulBuckets2);

	ul1 = UlAddResidualUnionBucket(memory_pool, pdrgppbucket, pbucket1, dRowsOther, fbucket1Residual, ul1, pdrgpdoubleRows);
	ul2 = UlAddResidualUnionBucket(memory_pool, pdrgppbucket, pbucket2, rows, fbucket2Residual, ul2, pdrgpdoubleRows);

	CleanupResidualBucket(pbucket1, fbucket1Residual);
	CleanupResidualBucket(pbucket2, fbucket2Residual);

	// add any leftover buckets from other histogram
	AddBuckets(memory_pool, phistOther->m_pdrgppbucket, pdrgppbucket, dRowsOther, pdrgpdoubleRows, ul2, ulBuckets2);

	// add any leftover buckets from this histogram
	AddBuckets(memory_pool, m_pdrgppbucket, pdrgppbucket, rows, pdrgpdoubleRows, ul1, ulBuckets1);

	// compute the total number of null values from both histograms
	CDouble dNullRows= std::max( (this->DNullFreq() * rows), (phistOther->DNullFreq() * dRowsOther));

	// compute the total number of distinct values (NDV) that are not captured by the buckets in both the histograms
	CDouble dNDVRemain = std::max(m_distint_remaining, phistOther->DDistinctRemain());

	// compute the total number of rows having distinct values not captured by the buckets in both the histograms
	CDouble dNDVRemainRows = std::max( (this->DFreqRemain() * rows), (phistOther->DFreqRemain() * dRowsOther));

	CHistogram *phistResult = PhistUpdatedFrequency
									(
									memory_pool,
									pdrgppbucket,
									pdrgpdoubleRows,
									pdRowsOutput,
									dNullRows,
									dNDVRemain,
									dNDVRemainRows
									);
	// clean up
	pdrgppbucket->Release();
	pdrgpdoubleRows->Release();

	return phistResult;
}

// create a new histogram with updated bucket frequency
CHistogram *
CHistogram::PhistUpdatedFrequency
	(
	IMemoryPool *memory_pool,
	DrgPbucket *pdrgppbucket,
	DrgPdouble *pdrgpdouble,
	CDouble *pdRowOutput,
	CDouble dNullRows,
	CDouble dNDVRemain,
	CDouble dNDVRemainRows
	)
	const
{
	GPOS_ASSERT(NULL != pdrgppbucket);
	GPOS_ASSERT(NULL != pdrgpdouble);

	const ULONG ulLen = pdrgpdouble->Size();
	GPOS_ASSERT(ulLen == pdrgppbucket->Size());

	CDouble dRowCummulative = dNullRows + dNDVRemainRows;
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		CDouble rows = *(*pdrgpdouble)[ul];
		dRowCummulative = dRowCummulative + rows;
	}

	*pdRowOutput = std::max(CStatistics::DMinRows, dRowCummulative);

	DrgPbucket *pdrgppbucketNew = GPOS_NEW(memory_pool) DrgPbucket(memory_pool);
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		CDouble rows = *(*pdrgpdouble)[ul];
		CBucket *pbucket = (*pdrgppbucket)[ul];

		// reuse the points
		pbucket->PpLower()->AddRef();
		pbucket->PpUpper()->AddRef();

		CDouble dFrequency = rows / *pdRowOutput;

		CBucket *pbucketNew = GPOS_NEW(memory_pool) CBucket
										(
										pbucket->PpLower(),
										pbucket->PpUpper(),
										pbucket->FLowerClosed(),
										pbucket->FUpperClosed(),
										dFrequency,
										pbucket->DDistinct()
										);

		pdrgppbucketNew->Append(pbucketNew);
	}

	CDouble null_freq = dNullRows / *pdRowOutput ;
	CDouble dNDVRemainFreq =  dNDVRemainRows / *pdRowOutput ;

	return GPOS_NEW(memory_pool) CHistogram
							(
							pdrgppbucketNew,
							true /* fWellDefined */,
							null_freq,
							dNDVRemain,
							dNDVRemainFreq,
							false /* is_col_stats_missing */
							);
}

// add residual bucket in an union operation to the array of buckets in the histogram
ULONG
CHistogram::UlAddResidualUnionBucket
	(
	IMemoryPool *memory_pool,
	DrgPbucket *pdrgppbucket,
	CBucket *pbucket,
	CDouble rows,
	BOOL fbucketResidual,
	ULONG ulIndex,
	DrgPdouble *pdrgpdouble
	)
	const
{
	GPOS_ASSERT(NULL != pdrgppbucket);
	GPOS_ASSERT(NULL != pdrgpdouble);

	if (!fbucketResidual)
	{
		return ulIndex;
	}

	CBucket *pbucketNew = pbucket->PbucketCopy(memory_pool);
	pdrgppbucket->Append(pbucketNew);
	pdrgpdouble->Append(GPOS_NEW(memory_pool) CDouble(pbucketNew->DFrequency() * rows));

	return ulIndex + 1;
}

// add buckets from one array to another
void
CHistogram::AddBuckets
	(
	IMemoryPool *memory_pool,
	DrgPbucket *pdrgppbucketSrc,
	DrgPbucket *pdrgppbucketDest,
	CDouble rows,
	DrgPdouble *pdrgpdouble,
	ULONG ulBegin,
	ULONG ulEnd
	)
{
	GPOS_ASSERT(NULL != pdrgppbucketSrc);
	GPOS_ASSERT(NULL != pdrgppbucketDest);
	GPOS_ASSERT(ulBegin <= ulEnd);
	GPOS_ASSERT(ulEnd <= pdrgppbucketSrc->Size());

	for (ULONG ul = ulBegin; ul < ulEnd; ul++)
	{
		CBucket *pbucketNew = ((*pdrgppbucketSrc)[ul])->PbucketCopy(memory_pool);
		pdrgppbucketDest->Append(pbucketNew);
		pdrgpdouble->Append(GPOS_NEW(memory_pool) CDouble(pbucketNew->DFrequency() * rows));
	}
}

// accessor for n-th bucket. Returns NULL if outside bounds
CBucket *
CHistogram::operator []
	(
	ULONG ulPos
	)
	const
{
	if (ulPos < UlBuckets())
	{
		return (*m_pdrgppbucket) [ulPos];
	}
	return NULL;
}

// translate the histogram into a the dxl derived column statistics
CDXLStatsDerivedColumn *
CHistogram::Pdxlstatsdercol
	(
	IMemoryPool *memory_pool,
	CMDAccessor *md_accessor,
	ULONG col_id,
	CDouble width
	)
	const
{
	DrgPdxlbucket *stats_bucket_dxl_array = GPOS_NEW(memory_pool) DrgPdxlbucket(memory_pool);

	const ULONG num_of_buckets = m_pdrgppbucket->Size();
	for (ULONG ul = 0; ul < num_of_buckets; ul++)
	{
		CBucket *pbucket = (*m_pdrgppbucket)[ul];

		CDouble dFreq = pbucket->DFrequency();
		CDouble dDistinct = pbucket->DDistinct();

		CDXLDatum *pdxldatumLower = pbucket->PpLower()->GetDatumVal(memory_pool, md_accessor);
		CDXLDatum *pdxldatumUpper = pbucket->PpUpper()->GetDatumVal(memory_pool, md_accessor);

		CDXLBucket *pdxlbucket = GPOS_NEW(memory_pool) CDXLBucket
											(
											pdxldatumLower,
											pdxldatumUpper,
											pbucket->FLowerClosed(),
											pbucket->FUpperClosed(),
											dFreq,
											dDistinct
											);

		stats_bucket_dxl_array->Append(pdxlbucket);
	}

	return GPOS_NEW(memory_pool) CDXLStatsDerivedColumn(col_id, width, m_null_freq, m_distint_remaining, m_freq_remaining, stats_bucket_dxl_array);
}

// randomly pick a bucket index based on bucket frequency values
ULONG
CHistogram::UlRandomBucketIndex
	(
	ULONG *pulSeed
	)
	const
{
	const ULONG size = m_pdrgppbucket->Size();
	GPOS_ASSERT(0 < size);

	DOUBLE dRandVal = ((DOUBLE) clib::Rand(pulSeed)) / RAND_MAX;
	CDouble dAccFreq = 0;
	for (ULONG ul = 0; ul < size - 1; ul++)
	{
		CBucket *pbucket = (*m_pdrgppbucket)[ul];
		dAccFreq = dAccFreq + pbucket->DFrequency();

		// we compare generated random value with accumulated frequency,
		// this will result in picking a bucket based on its frequency,
		// example: bucket freq {0.1, 0.3, 0.6}
		//			random value in [0,0.1] --> pick bucket 1
		//			random value in [0.1,0.4] --> pick bucket 2
		//			random value in [0.4,1.0] --> pick bucket 3

		if (dRandVal <= dAccFreq)
		{
			return ul;
		}
	}

	return size - 1;
}

// estimate data skew by sampling histogram buckets,
// the estimate value is >= 1.0, where 1.0 indicates no skew
//
// skew is estimated by computing the second and third moments of
// sample distribution: for a sample of size n, where x_bar is sample mean,
// skew is estimated as (m_3/m_2^(1.5)), where m_2 = 1/n Sum ((x -x_bar)^2), and
// m_3 = 1/n Sum ((x -x_bar)^3)
//
// since we use skew as multiplicative factor in cost model, this function
// returns (1.0 + |skew estimate|)
void
CHistogram::ComputeSkew()
{
	m_fSkewMeasured = true;

	if (!FNormalized() || 0 == m_pdrgppbucket->Size() || !(*m_pdrgppbucket)[0]->FCanSample())
	{
		return;
	}

	// generate randomization seed from system time
	TIMEVAL tv;
	syslib::GetTimeOfDay(&tv, NULL/*timezone*/);
	ULONG ulSeed = CombineHashes((ULONG) tv.tv_sec, (ULONG)tv.tv_usec);

	// generate a sample from histogram data, and compute sample mean
	DOUBLE dSampleMean = 0;
	DOUBLE rgdSamples[GPOPT_SKEW_SAMPLE_SIZE];
	for (ULONG ul = 0; ul < GPOPT_SKEW_SAMPLE_SIZE; ul++)
	{
		ULONG ulBucketIndex = UlRandomBucketIndex(&ulSeed);
		CBucket *pbucket = (*m_pdrgppbucket)[ulBucketIndex];
		rgdSamples[ul] = pbucket->DSample(&ulSeed).Get();
		dSampleMean = dSampleMean + rgdSamples[ul];
	}
	dSampleMean = (DOUBLE) dSampleMean / GPOPT_SKEW_SAMPLE_SIZE;

	// compute second and third moments of sample distribution
	DOUBLE d2 = 0;
	DOUBLE d3 = 0;
	for (ULONG ul = 0; ul < GPOPT_SKEW_SAMPLE_SIZE; ul++)
	{
		d2 = d2 + pow((rgdSamples[ul] - dSampleMean) , 2.0);
		d3 = d3 + pow((rgdSamples[ul] - dSampleMean) , 3.0);
	}
	DOUBLE dm2 = (DOUBLE)(d2 / GPOPT_SKEW_SAMPLE_SIZE);
	DOUBLE dm3 = (DOUBLE)(d3 / GPOPT_SKEW_SAMPLE_SIZE);

	// set skew measure
	m_dSkew =  CDouble(1.0 + fabs(dm3 / pow(dm2, 1.5)));
}

// create the default histogram for a given column reference
CHistogram *
CHistogram::PhistDefault
	(
	IMemoryPool *memory_pool,
	CColRef *pcr,
	BOOL is_empty
	)
{
	GPOS_ASSERT(NULL != pcr);

	if (IMDType::EtiBool == pcr->Pmdtype()->Eti() && !is_empty)
	{
		return CHistogram::PhistDefaultBoolColStats(memory_pool);
	}

	return GPOS_NEW(memory_pool) CHistogram(GPOS_NEW(memory_pool) DrgPbucket(memory_pool), false /* fWellDefined */);
}


// create the default non-empty histogram for a boolean column
CHistogram *
CHistogram::PhistDefaultBoolColStats
	(
	IMemoryPool *memory_pool
	)
{
	DrgPbucket *pdrgpbucket = GPOS_NEW(memory_pool) DrgPbucket(memory_pool);

	// a boolean column can at most have 3 values (true, false, and NULL).
	CDouble distinct_remaining = CDouble(3.0);
	CDouble freq_remaining = CDouble(1.0);
	CDouble null_freq = CDouble(0.0);

	return GPOS_NEW(memory_pool) CHistogram
						(
						pdrgpbucket,
						true /* fWellDefined */,
						null_freq,
						distinct_remaining,
						freq_remaining,
						true /*is_col_stats_missing */
						);
}

// check if the join cardinality estimation can be done based on NDV alone
BOOL
CHistogram::FNDVBasedCardEstimation
	(
	const CHistogram *phist
	)
{
	GPOS_ASSERT(NULL != phist);

	if (0 == phist->UlBuckets())
	{
		// no buckets, so join cardinality estimation is based solely on NDV remain
		return true;
	}

	const IBucket *pbucket = (*phist->ParseDXLToBucketsArray())[0];

	IDatum *pdatum = pbucket->PpLower()->Pdatum();

	IMDType::ETypeInfo eti = pdatum->Eti();
	if (IMDType::EtiInt2 == eti ||
		IMDType::EtiInt4 == eti ||
		IMDType::EtiInt8 == eti ||
		IMDType::EtiBool == eti ||
		IMDType::EtiOid == eti )
	{
		return false;
	}

	BOOL fRes = true;
	if (pdatum->FStatsMappable())
	{
		IDatumStatisticsMappable *pdatumMappable = (IDatumStatisticsMappable *) pdatum;

		if (pdatumMappable->IsDatumMappableToDouble())
		{
			fRes = false;
		}
	}

	return fRes;
}

// append given histograms to current object
void
CHistogram::AddHistograms
	(
	IMemoryPool *memory_pool,
	HMUlHist *phmulhistSrc,
	HMUlHist *phmulhistDest
	)
{
	HMIterUlHist hmiterulhist(phmulhistSrc);
	while (hmiterulhist.Advance())
	{
		ULONG col_id = *(hmiterulhist.Key());
		const CHistogram *phist = hmiterulhist.Value();
		CStatisticsUtils::AddHistogram(memory_pool, col_id, phist, phmulhistDest);
	}
}

// add dummy histogram buckets and column information for the array of columns
void
CHistogram::AddDummyHistogramAndWidthInfo
	(
	IMemoryPool *memory_pool,
	CColumnFactory *pcf,
	HMUlHist *phmulhistOutput,
	HMUlDouble *phmuldoubleWidthOutput,
	const ULongPtrArray *pdrgpul,
	BOOL is_empty
	)
{
	GPOS_ASSERT(NULL != pcf);
	GPOS_ASSERT(NULL != phmulhistOutput);
	GPOS_ASSERT(NULL != phmuldoubleWidthOutput);
	GPOS_ASSERT(NULL != pdrgpul);

	const ULONG ulCount = pdrgpul->Size();
	// for computed aggregates, we're not going to be very smart right now
	for (ULONG ul = 0; ul < ulCount; ul++)
	{
		ULONG col_id = *(*pdrgpul)[ul];

		CColRef *pcr = pcf->PcrLookup(col_id);
		GPOS_ASSERT(NULL != pcr);

		CHistogram *phist = CHistogram::PhistDefault(memory_pool, pcr, is_empty);
		phmulhistOutput->Insert(GPOS_NEW(memory_pool) ULONG(col_id), phist);

		CDouble width = CStatisticsUtils::DDefaultColumnWidth(pcr->Pmdtype());
		phmuldoubleWidthOutput->Insert(GPOS_NEW(memory_pool) ULONG(col_id), GPOS_NEW(memory_pool) CDouble(width));
	}
}

//	add empty histogram for the columns in the input histogram
void
CHistogram::AddEmptyHistogram
	(
	IMemoryPool *memory_pool,
	HMUlHist *phmulhistOutput,
	HMUlHist *phmulhistInput
	)
{
	GPOS_ASSERT(NULL != phmulhistOutput);
	GPOS_ASSERT(NULL != phmulhistInput);

	HMIterUlHist hmiterulhist(phmulhistInput);
	while (hmiterulhist.Advance())
	{
		ULONG col_id = *(hmiterulhist.Key());

		// empty histogram
		CHistogram *phist =  GPOS_NEW(memory_pool) CHistogram(GPOS_NEW(memory_pool) DrgPbucket(memory_pool), false /* fWellDefined */);
		phmulhistOutput->Insert(GPOS_NEW(memory_pool) ULONG(col_id), phist);
	}
}


// EOF

