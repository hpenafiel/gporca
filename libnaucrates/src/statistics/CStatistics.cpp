//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 Pivotal, Inc.
//
//	@filename:
//		CStatistics.cpp
//
//	@doc:
//		Histograms based statistics
//---------------------------------------------------------------------------

#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/statistics/CStatistics.h"
#include "naucrates/statistics/CStatisticsUtils.h"
#include "naucrates/statistics/CScaleFactorUtils.h"

#include "naucrates/statistics/CJoinStatsProcessor.h"
#include "naucrates/statistics/CLeftOuterJoinStatsProcessor.h"
#include "naucrates/statistics/CLeftSemiJoinStatsProcessor.h"
#include "naucrates/statistics/CLeftAntiSemiJoinStatsProcessor.h"
#include "naucrates/statistics/CInnerJoinStatsProcessor.h"
#include "gpos/common/CBitSet.h"
#include "gpos/sync/CAutoMutex.h"
#include "gpos/memory/CAutoMemoryPool.h"

#include "gpopt/base/CColumnFactory.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CReqdPropRelational.h"
#include "gpopt/mdcache/CMDAccessor.h"


#include "gpopt/engine/CStatisticsConfig.h"
#include "gpopt/optimizer/COptimizerConfig.h"

using namespace gpmd;
using namespace gpdxl;
using namespace gpopt;

// default number of rows in relation
const CDouble CStatistics::DDefaultRelationRows(1000.0);

// epsilon to be used for various computations
const CDouble CStatistics::DEpsilon(0.001);

// minimum number of rows in relation
const CDouble CStatistics::DMinRows(1.0);

// default column width
const CDouble CStatistics::DDefaultColumnWidth(8.0);

// default number of distinct values
const CDouble CStatistics::DDefaultDistinctValues(1000.0);

// the default value for operators that have no cardinality estimation risk
const ULONG CStatistics::ulStatsEstimationNoRisk = 1;

// ctor
CStatistics::CStatistics
	(
	IMemoryPool *memory_pool,
	HMUlHist *phmulhist,
	HMUlDouble *phmuldoubleWidth,
	CDouble dRows,
	BOOL fEmpty,
	ULONG ulNumPredicates
	)
	:
	m_phmulhist(phmulhist),
	m_phmuldoubleWidth(phmuldoubleWidth),
	m_rows(dRows),
	m_ulStatsEstimationRisk(ulStatsEstimationNoRisk),
	m_empty(fEmpty),
	m_dRebinds(1.0), // by default, a stats object is rebound to parameters only once
	m_ulNumPredicates(ulNumPredicates),
	m_pdrgpubndvs(NULL)
{
	GPOS_ASSERT(NULL != m_phmulhist);
	GPOS_ASSERT(NULL != m_phmuldoubleWidth);
	GPOS_ASSERT(CDouble(0.0) <= m_rows);

	// hash map for source id -> max source cardinality mapping
	m_pdrgpubndvs = GPOS_NEW(memory_pool) DrgPubndvs(memory_pool);

	m_pstatsconf = COptCtxt::PoctxtFromTLS()->GetOptimizerConfig()->Pstatsconf();
}

// Dtor
CStatistics::~CStatistics()
{
	m_phmulhist->Release();
	m_phmuldoubleWidth->Release();
	m_pdrgpubndvs->Release();
}

// look up the width of a particular column
const CDouble *
CStatistics::PdWidth
	(
	ULONG col_id
	)
	const
{
	return m_phmuldoubleWidth->Find(&col_id);
}


//	cap the total number of distinct values (NDVs) in buckets to the number of rows
void
CStatistics::CapNDVs
	(
	CDouble dRows,
	HMUlHist *phmulhist
	)
{
	GPOS_ASSERT(NULL != phmulhist);
	HMIterUlHist hmiterulhist(phmulhist);
	while (hmiterulhist.Advance())
	{
		CHistogram *phist = const_cast<CHistogram *>(hmiterulhist.Value());
		phist->CapNDVs(dRows);
	}
}

// helper print function
IOstream &
CStatistics::OsPrint
	(
	IOstream &os
	)
	const
{
	os << "{" << std::endl;
	os << "Rows = " << DRows() << std::endl;
	os << "Rebinds = " << DRebinds() << std::endl;

	HMIterUlHist hmiterulhist(m_phmulhist);
	while (hmiterulhist.Advance())
	{
		ULONG col_id = *(hmiterulhist.Key());
		os << "Col" << col_id << ":" << std::endl;
		const CHistogram *phist = hmiterulhist.Value();
		phist->OsPrint(os);
		os << std::endl;
	}

	HMIterUlDouble hmiteruldouble(m_phmuldoubleWidth);
	while (hmiteruldouble.Advance())
	{
		ULONG col_id = *(hmiteruldouble.Key());
		os << "Col" << col_id << ":" << std::endl;
		const CDouble *pdWidth = hmiteruldouble.Value();
		os << " width " << (*pdWidth) << std::endl;
	}

	const ULONG ulLen = m_pdrgpubndvs->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		const CUpperBoundNDVs *pubndv = (*m_pdrgpubndvs)[ul];
		pubndv->OsPrint(os);
	}
	os << "StatsEstimationRisk = " << UlStatsEstimationRisk() << std::endl;
	os << "}" << std::endl;

	return os;
}

//	return the total number of rows for this statistics object
CDouble
CStatistics::DRows() const
{
	return m_rows;
}

// return the estimated skew of the given column
CDouble
CStatistics::DSkew
	(
	ULONG col_id
	)
	const
{
	CHistogram *phist = m_phmulhist->Find(&col_id);
	if (NULL == phist)
	{
		return CDouble(1.0);
	}

	return phist->DSkew();
}

// return total width in bytes
CDouble
CStatistics::DWidth() const
{
	CDouble dWidth(0.0);
	HMIterUlDouble hmiteruldouble(m_phmuldoubleWidth);
	while (hmiteruldouble.Advance())
	{
		const CDouble *pdWidth = hmiteruldouble.Value();
		dWidth = dWidth + (*pdWidth);
	}
	return dWidth.Ceil();
}

// return the width in bytes of a set of columns
CDouble
CStatistics::DWidth
	(
	ULongPtrArray *pdrgpulColIds
	)
	const
{
	GPOS_ASSERT(NULL != pdrgpulColIds);

	CColumnFactory *pcf = COptCtxt::PoctxtFromTLS()->Pcf();
	CDouble dWidth(0.0);
	const ULONG size = pdrgpulColIds->Size();
	for (ULONG ulIdx = 0; ulIdx < size; ulIdx++)
	{
		ULONG col_id = *((*pdrgpulColIds)[ulIdx]);
		CDouble *pdWidth = m_phmuldoubleWidth->Find(&col_id);
		if (NULL != pdWidth)
		{
			dWidth = dWidth + (*pdWidth);
		}
		else
		{
			CColRef *pcr = pcf->PcrLookup(col_id);
			GPOS_ASSERT(NULL != pcr);

			dWidth = dWidth + CStatisticsUtils::DDefaultColumnWidth(pcr->Pmdtype());
		}
	}
	return dWidth.Ceil();
}

// return width in bytes of a set of columns
CDouble
CStatistics::DWidth
	(
	IMemoryPool *memory_pool,
	CColRefSet *pcrs
	)
	const
{
	GPOS_ASSERT(NULL != pcrs);

	ULongPtrArray *pdrgpulColIds = GPOS_NEW(memory_pool) ULongPtrArray(memory_pool);
	pcrs->ExtractColIds(memory_pool, pdrgpulColIds);

	CDouble dWidth = DWidth(pdrgpulColIds);
	pdrgpulColIds->Release();

	return dWidth;
}

// return dummy statistics object
CStatistics *
CStatistics::PstatsDummy
	(
	IMemoryPool *memory_pool,
	ULongPtrArray *pdrgpulColIds,
	CDouble dRows
	)
{
	GPOS_ASSERT(NULL != pdrgpulColIds);

	// hash map from colid -> histogram for resultant structure
	HMUlHist *phmulhist = GPOS_NEW(memory_pool) HMUlHist(memory_pool);

	// hashmap from colid -> width (double)
	HMUlDouble *phmuldoubleWidth = GPOS_NEW(memory_pool) HMUlDouble(memory_pool);

	CColumnFactory *pcf = COptCtxt::PoctxtFromTLS()->Pcf();

	BOOL fEmpty = (CStatistics::DEpsilon >= dRows);
	CHistogram::AddDummyHistogramAndWidthInfo(memory_pool, pcf, phmulhist, phmuldoubleWidth, pdrgpulColIds, fEmpty);

	CStatistics *pstats = GPOS_NEW(memory_pool) CStatistics(memory_pool, phmulhist, phmuldoubleWidth, dRows, fEmpty);
	CreateAndInsertUpperBoundNDVs(memory_pool, pstats, pdrgpulColIds, dRows);

	return pstats;
}

// add upper bound ndvs information for a given set of columns
void
CStatistics::CreateAndInsertUpperBoundNDVs
	(
	IMemoryPool *memory_pool,
	CStatistics *pstats,
	ULongPtrArray *pdrgpulColIds,
	CDouble dRows
)
{
	GPOS_ASSERT(NULL != pstats);
	GPOS_ASSERT(NULL != pdrgpulColIds);

	CColumnFactory *pcf = COptCtxt::PoctxtFromTLS()->Pcf();
	CColRefSet *pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool);
	const ULONG ulCols = pdrgpulColIds->Size();
	for (ULONG ul = 0; ul < ulCols; ul++)
	{
		ULONG col_id = *(*pdrgpulColIds)[ul];
		const CColRef *pcr = pcf->PcrLookup(col_id);
		if (NULL != pcr)
		{
			pcrs->Include(pcr);
		}
	}

	if (0 < pcrs->Size())
	{
		pstats->AddCardUpperBound(GPOS_NEW(memory_pool) CUpperBoundNDVs(pcrs, dRows));
	}
	else
	{
		pcrs->Release();
	}
}

//	return dummy statistics object
CStatistics *
CStatistics::PstatsDummy
	(
	IMemoryPool *memory_pool,
	ULongPtrArray *pdrgpulHistColIds,
	ULongPtrArray *pdrgpulWidthColIds,
	CDouble dRows
	)
{
	GPOS_ASSERT(NULL != pdrgpulHistColIds);
	GPOS_ASSERT(NULL != pdrgpulWidthColIds);

	BOOL fEmpty = (CStatistics::DEpsilon >= dRows);
	CColumnFactory *pcf = COptCtxt::PoctxtFromTLS()->Pcf();

	// hash map from colid -> histogram for resultant structure
	HMUlHist *phmulhist = GPOS_NEW(memory_pool) HMUlHist(memory_pool);

	const ULONG ulHistCol = pdrgpulHistColIds->Size();
	for (ULONG ul = 0; ul < ulHistCol; ul++)
	{
		ULONG col_id = *(*pdrgpulHistColIds)[ul];

		CColRef *pcr = pcf->PcrLookup(col_id);
		GPOS_ASSERT(NULL != pcr);

		// empty histogram
		CHistogram *phist = CHistogram::PhistDefault(memory_pool, pcr, fEmpty);
		phmulhist->Insert(GPOS_NEW(memory_pool) ULONG(col_id), phist);
	}

	// hashmap from colid -> width (double)
	HMUlDouble *phmuldoubleWidth = GPOS_NEW(memory_pool) HMUlDouble(memory_pool);

	const ULONG ulWidthCol = pdrgpulWidthColIds->Size();
	for (ULONG ul = 0; ul < ulWidthCol; ul++)
	{
		ULONG col_id = *(*pdrgpulWidthColIds)[ul];

		CColRef *pcr = pcf->PcrLookup(col_id);
		GPOS_ASSERT(NULL != pcr);

		CDouble dWidth = CStatisticsUtils::DDefaultColumnWidth(pcr->Pmdtype());
		phmuldoubleWidth->Insert(GPOS_NEW(memory_pool) ULONG(col_id), GPOS_NEW(memory_pool) CDouble(dWidth));
	}

	CStatistics *pstats = GPOS_NEW(memory_pool) CStatistics(memory_pool, phmulhist, phmuldoubleWidth, dRows, false /* fEmpty */);
	CreateAndInsertUpperBoundNDVs(memory_pool, pstats, pdrgpulHistColIds, dRows);

	return pstats;
}


//	check if the input statistics from join statistics computation empty
BOOL
CStatistics::FEmptyJoinInput
	(
	const CStatistics *pstatsOuter,
	const CStatistics *pstatsInner,
	BOOL fLASJ
	)
{
	GPOS_ASSERT(NULL != pstatsOuter);
	GPOS_ASSERT(NULL != pstatsInner);

	if (fLASJ)
	{
		return pstatsOuter->IsEmpty();
	}

	return pstatsOuter->IsEmpty() || pstatsInner->IsEmpty();
}

// Currently, Pstats[Join type] are thin wrappers the C[Join type]StatsProcessor class's method
// for deriving the stat objects for the corresponding join operator

//	return statistics object after performing LOJ operation with another statistics structure
CStatistics *
CStatistics::PstatsLOJ
	(
	IMemoryPool *memory_pool,
	const IStatistics *pstatsOther,
	DrgPstatspredjoin *pdrgpstatspredjoin
	)
	const
{
	return CLeftOuterJoinStatsProcessor::PstatsLOJStatic(memory_pool, this, pstatsOther, pdrgpstatspredjoin);
}



//	return statistics object after performing semi-join with another statistics structure
CStatistics *
CStatistics::PstatsLSJoin
	(
	IMemoryPool *memory_pool,
	const IStatistics *pstatsInner,
	DrgPstatspredjoin *pdrgpstatspredjoin
	)
	const
{
	return CLeftSemiJoinStatsProcessor::PstatsLSJoinStatic(memory_pool, this, pstatsInner, pdrgpstatspredjoin);
}



// return statistics object after performing inner join
CStatistics *
CStatistics::PstatsInnerJoin
	(
	IMemoryPool *memory_pool,
	const IStatistics *pistatsOther,
	DrgPstatspredjoin *pdrgpstatspredjoin
	)
	const
{
	return CInnerJoinStatsProcessor::PstatsInnerJoinStatic(memory_pool, this, pistatsOther, pdrgpstatspredjoin);
}

// return statistics object after performing LASJ
CStatistics *
CStatistics::PstatsLASJoin
	(
	IMemoryPool *memory_pool,
	const IStatistics *pistatsOther,
	DrgPstatspredjoin *pdrgpstatspredjoin,
	BOOL fIgnoreLasjHistComputation
	)
	const
{
	return CLeftAntiSemiJoinStatsProcessor::PstatsLASJoinStatic(memory_pool, this, pistatsOther, pdrgpstatspredjoin, fIgnoreLasjHistComputation);
}

//	helper method to copy statistics on columns that are not excluded by bitset
void
CStatistics::AddNotExcludedHistograms
	(
	IMemoryPool *memory_pool,
	CBitSet *pbsExcludedColIds,
	HMUlHist *phmulhist
	)
	const
{
	GPOS_ASSERT(NULL != pbsExcludedColIds);
	GPOS_ASSERT(NULL != phmulhist);

	HMIterUlHist hmiterulhist(m_phmulhist);
	while (hmiterulhist.Advance())
	{
		ULONG col_id = *(hmiterulhist.Key());
		if (!pbsExcludedColIds->Get(col_id))
		{
			const CHistogram *phist = hmiterulhist.Value();
			CStatisticsUtils::AddHistogram(memory_pool, col_id, phist, phmulhist);
		}

		GPOS_CHECK_ABORT;
	}
}

HMUlDouble *
CStatistics::CopyWidths
	(
	IMemoryPool *memory_pool
	)
	const
{
	HMUlDouble *phmuldoubleCoopy = GPOS_NEW(memory_pool) HMUlDouble(memory_pool);
	CStatisticsUtils::AddWidthInfo(memory_pool, m_phmuldoubleWidth, phmuldoubleCoopy);

	return phmuldoubleCoopy;
}

void
CStatistics::CopyWidthsInto
	(
	IMemoryPool *memory_pool,
	HMUlDouble *phmuldouble
	)
	const
{
	CStatisticsUtils::AddWidthInfo(memory_pool, m_phmuldoubleWidth, phmuldouble);
}

HMUlHist *
CStatistics::CopyHistograms
	(
	IMemoryPool *memory_pool
	)
	const
{
	// create hash map from colid -> histogram for resultant structure
	HMUlHist *phmulhistCopy = GPOS_NEW(memory_pool) HMUlHist(memory_pool);

	BOOL fEmpty = IsEmpty();

	HMIterUlHist hmiterulhist(m_phmulhist);
	while (hmiterulhist.Advance())
	{
		ULONG col_id = *(hmiterulhist.Key());
		const CHistogram *phist = hmiterulhist.Value();
		CHistogram *phistCopy = NULL;
		if (fEmpty)
		{
			phistCopy =  GPOS_NEW(memory_pool) CHistogram(GPOS_NEW(memory_pool) DrgPbucket(memory_pool), false /* fWellDefined */);
		}
		else
		{
			phistCopy = phist->PhistCopy(memory_pool);
		}

		phmulhistCopy->Insert(GPOS_NEW(memory_pool) ULONG(col_id), phistCopy);
	}

	return phmulhistCopy;
}



//	return required props associated with statistics object
CReqdPropRelational *
CStatistics::Prprel
	(
	IMemoryPool *memory_pool
	)
	const
{
	CColumnFactory *pcf = COptCtxt::PoctxtFromTLS()->Pcf();
	GPOS_ASSERT(NULL != pcf);

	CColRefSet *pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool);

	// add columns from histogram map
	HMIterUlHist hmiterulhist(m_phmulhist);
	while (hmiterulhist.Advance())
	{
		ULONG col_id = *(hmiterulhist.Key());
		CColRef *pcr = pcf->PcrLookup(col_id);
		GPOS_ASSERT(NULL != pcr);

		pcrs->Include(pcr);
	}

	return GPOS_NEW(memory_pool) CReqdPropRelational(pcrs);
}

// append given statistics to current object
void
CStatistics::AppendStats
	(
	IMemoryPool *memory_pool,
	IStatistics *pstatsInput
	)
{
	CStatistics *pstats = CStatistics::PstatsConvert(pstatsInput);

	CHistogram::AddHistograms(memory_pool, pstats->m_phmulhist, m_phmulhist);
	GPOS_CHECK_ABORT;

	CStatisticsUtils::AddWidthInfo(memory_pool, pstats->m_phmuldoubleWidth, m_phmuldoubleWidth);
	GPOS_CHECK_ABORT;
}

// copy statistics object
IStatistics *
CStatistics::PstatsCopy
	(
	IMemoryPool *memory_pool
	)
	const
{
	return PstatsScale(memory_pool, CDouble(1.0) /*dFactor*/);
}

// return a copy of this statistics object scaled by a given factor
IStatistics *
CStatistics::PstatsScale
	(
	IMemoryPool *memory_pool,
	CDouble dFactor
	)
	const
{
	HMUlHist *phmulhistNew = GPOS_NEW(memory_pool) HMUlHist(memory_pool);
	HMUlDouble *phmuldoubleNew = GPOS_NEW(memory_pool) HMUlDouble(memory_pool);

	CHistogram::AddHistograms(memory_pool, m_phmulhist, phmulhistNew);
	GPOS_CHECK_ABORT;

	CStatisticsUtils::AddWidthInfo(memory_pool, m_phmuldoubleWidth, phmuldoubleNew);
	GPOS_CHECK_ABORT;

	CDouble dRowsScaled = m_rows * dFactor;

	// create a scaled stats object
	CStatistics *pstatsScaled = GPOS_NEW(memory_pool) CStatistics
												(
												memory_pool,
												phmulhistNew,
												phmuldoubleNew,
												dRowsScaled,
												IsEmpty(),
												m_ulNumPredicates
												);

	// In the output statistics object, the upper bound source cardinality of the scaled column
	// cannot be greater than the the upper bound source cardinality information maintained in the input
	// statistics object. Therefore we choose CStatistics::EcbmMin the bounding method which takes
	// the minimum of the cardinality upper bound of the source column (in the input hash map)
	// and estimated output cardinality.

	// modify source id to upper bound card information
	CStatisticsUtils::ComputeCardUpperBounds(memory_pool, this, pstatsScaled, dRowsScaled, CStatistics::EcbmMin /* ecbm */);

	return pstatsScaled;
}

//	copy statistics object with re-mapped column ids
IStatistics *
CStatistics::PstatsCopyWithRemap
	(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr,
	BOOL fMustExist
	)
	const
{
	GPOS_ASSERT(NULL != phmulcr);
	HMUlHist *phmulhistNew = GPOS_NEW(memory_pool) HMUlHist(memory_pool);
	HMUlDouble *phmuldoubleNew = GPOS_NEW(memory_pool) HMUlDouble(memory_pool);

	AddHistogramsWithRemap(memory_pool, m_phmulhist, phmulhistNew, phmulcr, fMustExist);
	AddWidthInfoWithRemap(memory_pool, m_phmuldoubleWidth, phmuldoubleNew, phmulcr, fMustExist);

	// create a copy of the stats object
	CStatistics *pstatsCopy = GPOS_NEW(memory_pool) CStatistics
											(
											memory_pool,
											phmulhistNew,
											phmuldoubleNew,
											m_rows,
											IsEmpty(),
											m_ulNumPredicates
											);

	// In the output statistics object, the upper bound source cardinality of the join column
	// cannot be greater than the the upper bound source cardinality information maintained in the input
	// statistics object.

	// copy the upper bound ndv information
	const ULONG ulLen = m_pdrgpubndvs->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		const CUpperBoundNDVs *pubndv = (*m_pdrgpubndvs)[ul];
 	 	CUpperBoundNDVs *pubndvCopy = pubndv->PubndvCopyWithRemap(memory_pool, phmulcr);

		if (NULL != pubndvCopy)
	 	{
			pstatsCopy->AddCardUpperBound(pubndvCopy);
	 	}
	}

	return pstatsCopy;
}

//	return the column identifiers of all columns whose statistics are
//	maintained by the statistics object
ULongPtrArray *
CStatistics::PdrgulColIds
	(
	IMemoryPool *memory_pool
	)
	const
{
	ULongPtrArray *pdrgpul = GPOS_NEW(memory_pool) ULongPtrArray(memory_pool);

	HMIterUlHist hmiterulhist(m_phmulhist);
	while (hmiterulhist.Advance())
	{
		ULONG col_id = *(hmiterulhist.Key());
		pdrgpul->Append(GPOS_NEW(memory_pool) ULONG(col_id));
	}

	return pdrgpul;
}

// return the set of column references we have statistics for
CColRefSet *
CStatistics::Pcrs
	(
	IMemoryPool *memory_pool
	)
	const
{
	CColRefSet *pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool);
	CColumnFactory *pcf = COptCtxt::PoctxtFromTLS()->Pcf();

	HMIterUlHist hmiterulhist(m_phmulhist);
	while (hmiterulhist.Advance())
	{
		ULONG col_id = *(hmiterulhist.Key());
		CColRef *pcr = pcf->PcrLookup(col_id);
		GPOS_ASSERT(NULL != pcr);

		pcrs->Include(pcr);
	}

	return pcrs;
}

//	append given histograms to current object where the column ids have been re-mapped
void
CStatistics::AddHistogramsWithRemap
	(
	IMemoryPool *memory_pool,
	HMUlHist *phmulhistSrc,
	HMUlHist *phmulhistDest,
	HMUlCr *phmulcr,
	BOOL
#ifdef GPOS_DEBUG
	fMustExist
#endif //GPOS_DEBUG
	)
{
	HMIterUlCr hmiterulcr(phmulcr);
	while (hmiterulcr.Advance())
	{
		ULONG ulColIdSrc = *(hmiterulcr.Key());
		const CColRef *pcrDest = hmiterulcr.Value();
		GPOS_ASSERT_IMP(fMustExist, NULL != pcrDest);

		ULONG ulColIdDest = pcrDest->Id();

		const CHistogram *phistSrc = phmulhistSrc->Find(&ulColIdSrc);
		if (NULL != phistSrc)
		{
			CStatisticsUtils::AddHistogram(memory_pool, ulColIdDest, phistSrc, phmulhistDest);
		}
	}
}

// add width information where the column ids have been re-mapped
void
CStatistics::AddWidthInfoWithRemap
		(
		IMemoryPool *memory_pool,
		HMUlDouble *phmuldoubleSrc,
		HMUlDouble *phmuldoubleDest,
		HMUlCr *phmulcr,
		BOOL fMustExist
		)
{
	HMIterUlDouble hmiteruldouble(phmuldoubleSrc);
	while (hmiteruldouble.Advance())
	{
		ULONG col_id = *(hmiteruldouble.Key());
		CColRef *pcrNew = phmulcr->Find(&col_id);
		if (fMustExist && NULL == pcrNew)
		{
			continue;
		}

		if (NULL != pcrNew)
		{
			col_id = pcrNew->Id();
		}

		if (NULL == phmuldoubleDest->Find(&col_id))
		{
			const CDouble *pdWidth = hmiteruldouble.Value();
			CDouble *pdWidthCopy = GPOS_NEW(memory_pool) CDouble(*pdWidth);
#ifdef GPOS_DEBUG
			BOOL fResult =
#endif // GPOS_DEBUG
					phmuldoubleDest->Insert(GPOS_NEW(memory_pool) ULONG(col_id), pdWidthCopy);
			GPOS_ASSERT(fResult);
		}
	}
}

// return the index of the array of upper bound ndvs to which column reference belongs
ULONG
CStatistics::UlIndexUpperBoundNDVs
	(
	const CColRef *pcr
	)
{
	GPOS_ASSERT(NULL != pcr);
 	CAutoMutex am(m_mutexCardUpperBoundAccess);
 	am.Lock();

 	const ULONG ulLen = m_pdrgpubndvs->Size();
 	for (ULONG ul = 0; ul < ulLen; ul++)
 	{
 		const CUpperBoundNDVs *pubndv = (*m_pdrgpubndvs)[ul];
 	 	if (pubndv->FPresent(pcr))
 	 	{
 	 		return ul;
 	 	}
	}

	return ULONG_MAX;
}

// add upper bound of source cardinality
void
CStatistics::AddCardUpperBound
	(
	CUpperBoundNDVs *pubndv
	)
{
	GPOS_ASSERT(NULL != pubndv);

	CAutoMutex am(m_mutexCardUpperBoundAccess);
	am.Lock();

	m_pdrgpubndvs->Append(pubndv);
}

// return the dxl representation of the statistics object
CDXLStatsDerivedRelation *
CStatistics::GetDxlStatsDrvdRelation
	(
	IMemoryPool *memory_pool,
	CMDAccessor *md_accessor
	)
	const
{
	DrgPdxlstatsdercol *dxl_stats_derived_col_array = GPOS_NEW(memory_pool) DrgPdxlstatsdercol(memory_pool);

	HMIterUlHist hmiterulhist(m_phmulhist);
	while (hmiterulhist.Advance())
	{
		ULONG col_id = *(hmiterulhist.Key());
		const CHistogram *phist = hmiterulhist.Value();

		CDouble *pdWidth = m_phmuldoubleWidth->Find(&col_id);
		GPOS_ASSERT(pdWidth);

		CDXLStatsDerivedColumn *dxl_derived_col_stats = phist->Pdxlstatsdercol(memory_pool, md_accessor, col_id, *pdWidth);
		dxl_stats_derived_col_array->Append(dxl_derived_col_stats);
	}

	return GPOS_NEW(memory_pool) CDXLStatsDerivedRelation(m_rows, IsEmpty(), dxl_stats_derived_col_array);
}

// return the upper bound of ndvs for a column reference
CDouble
CStatistics::DUpperBoundNDVs
	(
	const CColRef *pcr
	)
{
	GPOS_ASSERT(NULL != pcr);

	CAutoMutex am(m_mutexCardUpperBoundAccess);
	am.Lock();

	const ULONG ulLen = m_pdrgpubndvs->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		const CUpperBoundNDVs *pubndv = (*m_pdrgpubndvs)[ul];
		if (pubndv->FPresent(pcr))
		{
			return pubndv->DUpperBoundNDVs();
		}
	}

	return DDefaultDistinctValues;
}


// look up the number of distinct values of a particular column
CDouble
CStatistics::DNDV
	(
	const CColRef *pcr
	)
{
	ULONG col_id = pcr->Id();
	CHistogram *phistCol = m_phmulhist->Find(&col_id);
	if (NULL != phistCol)
	{
		return std::min(phistCol->DDistinct(), DUpperBoundNDVs(pcr));
	}

#ifdef GPOS_DEBUG
	{
		// the case of no histogram available for requested column signals
		// something wrong with computation of required statistics columns,
		// we print a debug message to log this case

		CAutoMemoryPool amp;
		CAutoTrace at(amp.Pmp());

		at.Os() << "\nREQUESTED NDVs FOR COL (" << pcr->Id()  << ") WITH A MISSING HISTOGRAM";
	}
#endif //GPOS_DEBUG

	// if no histogram is available for required column, we use
	// the number of rows as NDVs estimate
	return std::min(m_rows, DUpperBoundNDVs(pcr));
}


// EOF
