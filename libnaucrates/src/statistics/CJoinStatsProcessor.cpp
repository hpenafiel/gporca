//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright 2018 Pivotal, Inc.
//
//	@filename:
//		CJoinStatsProcessor.cpp
//
//	@doc:
//		Statistics helper routines for processing all join types
//---------------------------------------------------------------------------

#include "gpopt/operators/ops.h"
#include "gpopt/optimizer/COptimizerConfig.h"

#include "naucrates/statistics/CStatisticsUtils.h"
#include "naucrates/statistics/CJoinStatsProcessor.h"
#include "naucrates/statistics/CLeftAntiSemiJoinStatsProcessor.h"
#include "naucrates/statistics/CFilterStatsProcessor.h"
#include "naucrates/statistics/CScaleFactorUtils.h"

using namespace gpopt;

// helper for joining histograms
void
CJoinStatsProcessor::JoinHistograms
			(
			IMemoryPool *memory_pool,
			const CHistogram *phist1,
			const CHistogram *phist2,
			CStatsPredJoin *pstatsjoin,
			CDouble dRows1,
			CDouble dRows2,
			CHistogram **pphist1, // output: histogram 1 after join
			CHistogram **pphist2, // output: histogram 2 after join
			CDouble *pdScaleFactor, // output: scale factor based on the join
			BOOL fEmptyInput,
			IStatistics::EStatsJoinType eStatsJoinType,
			BOOL fIgnoreLasjHistComputation
			)
{
	GPOS_ASSERT(NULL != phist1);
	GPOS_ASSERT(NULL != phist2);
	GPOS_ASSERT(NULL != pstatsjoin);
	GPOS_ASSERT(NULL != pphist1);
	GPOS_ASSERT(NULL != pphist2);
	GPOS_ASSERT(NULL != pdScaleFactor);

	if (IStatistics::EsjtLeftAntiSemiJoin == eStatsJoinType)
	{
		CLeftAntiSemiJoinStatsProcessor::JoinHistogramsLASJ
				(
				memory_pool,
				phist1,
				phist2,
				pstatsjoin,
				dRows1,
				dRows2,
				pphist1,
				pphist2,
				pdScaleFactor,
				fEmptyInput,
				eStatsJoinType,
				fIgnoreLasjHistComputation
				);

		return;
	}

	if (fEmptyInput)
	{
		// use Cartesian product as scale factor
		*pdScaleFactor = dRows1 * dRows2;
		*pphist1 =  GPOS_NEW(memory_pool) CHistogram(GPOS_NEW(memory_pool) DrgPbucket(memory_pool));
		*pphist2 =  GPOS_NEW(memory_pool) CHistogram(GPOS_NEW(memory_pool) DrgPbucket(memory_pool));

		return;
	}

	*pdScaleFactor = CScaleFactorUtils::DDefaultScaleFactorJoin;

	CStatsPred::EStatsCmpType escmpt = pstatsjoin->Escmpt();
	BOOL fEmptyHistograms = phist1->IsEmpty() || phist2->IsEmpty();

	if (fEmptyHistograms)
	{
		// if one more input has no histograms (due to lack of statistics
		// for table columns or computed columns), we estimate
		// the join cardinality to be the max of the two rows.
		// In other words, the scale factor is equivalent to the
		// min of the two rows.
		*pdScaleFactor = std::min(dRows1, dRows2);
	}
	else if (CHistogram::FSupportsJoinPred(escmpt))
	{
		CHistogram *phistJoin = phist1->PhistJoinNormalized
				(
				memory_pool,
				escmpt,
				dRows1,
				phist2,
				dRows2,
				pdScaleFactor
				);

		if (CStatsPred::EstatscmptEq == escmpt || CStatsPred::EstatscmptINDF == escmpt || CStatsPred::EstatscmptEqNDV == escmpt)
		{
			if (phist1->FScaledNDV())
			{
				phistJoin->SetNDVScaled();
			}
			*pphist1 = phistJoin;
			*pphist2 = (*pphist1)->PhistCopy(memory_pool);
			if (phist2->FScaledNDV())
			{
				(*pphist2)->SetNDVScaled();
			}
			return;
		}

		// note that for IDF and Not Equality predicate, we do not generate histograms but
		// just the scale factors.

		GPOS_ASSERT(phistJoin->IsEmpty());
		GPOS_DELETE(phistJoin);

		// TODO:  Feb 21 2014, for all join condition except for "=" join predicate
		// we currently do not compute new histograms for the join columns
	}

	// for an unsupported join predicate operator or in the case of
	// missing histograms, copy input histograms and use default scale factor
	*pphist1 = phist1->PhistCopy(memory_pool);
	*pphist2 = phist2->PhistCopy(memory_pool);
}

//	derive statistics for the given join's predicate(s)
IStatistics *
CJoinStatsProcessor::PstatsJoinArray
		(
		IMemoryPool *memory_pool,
		DrgPstat *statistics_array,
		CExpression *pexprScalar,
		IStatistics::EStatsJoinType eStatsJoinType
		)
{
	GPOS_ASSERT(NULL != pexprScalar);
	GPOS_ASSERT(NULL != statistics_array);
	GPOS_ASSERT(0 < statistics_array->Size());
	BOOL fLeftOuterJoin = IStatistics::EsjtLeftOuterJoin == eStatsJoinType;
	GPOS_ASSERT_IMP(fLeftOuterJoin, 2 == statistics_array->Size());


	// create an empty set of outer references for statistics derivation
	CColRefSet *pcrsOuterRefs = GPOS_NEW(memory_pool) CColRefSet(memory_pool);

	// join statistics objects one by one using relevant predicates in given scalar expression
	const ULONG ulStats = statistics_array->Size();
	IStatistics *pstats = (*statistics_array)[0]->PstatsCopy(memory_pool);
	CDouble dRowsOuter = pstats->DRows();

	for (ULONG ul = 1; ul < ulStats; ul++)
	{
		IStatistics *pstatsCurrent = (*statistics_array)[ul];

		DrgPcrs *pdrgpcrsOutput= GPOS_NEW(memory_pool) DrgPcrs(memory_pool);
		pdrgpcrsOutput->Append(pstats->Pcrs(memory_pool));
		pdrgpcrsOutput->Append(pstatsCurrent->Pcrs(memory_pool));

		CStatsPred *pstatspredUnsupported = NULL;
		DrgPstatspredjoin *pdrgpstatspredjoin = CStatsPredUtils::PdrgpstatspredjoinExtract
				(
						memory_pool,
						pexprScalar,
						pdrgpcrsOutput,
						pcrsOuterRefs,
						&pstatspredUnsupported
				);
		IStatistics *pstatsNew = NULL;
		if (fLeftOuterJoin)
		{
			pstatsNew = pstats->PstatsLOJ(memory_pool, pstatsCurrent, pdrgpstatspredjoin);
		}
		else
		{
			pstatsNew = pstats->PstatsInnerJoin(memory_pool, pstatsCurrent, pdrgpstatspredjoin);
		}
		pstats->Release();
		pstats = pstatsNew;

		if (NULL != pstatspredUnsupported)
		{
			// apply the unsupported join filters as a filter on top of the join results.
			// TODO,  June 13 2014 we currently only cap NDVs for filters
			// immediately on top of tables.
			IStatistics *pstatsAfterJoinFilter = CFilterStatsProcessor::PstatsFilter(memory_pool, dynamic_cast<CStatistics *>(pstats), pstatspredUnsupported, false /* fCapNdvs */);

			// If it is outer join and the cardinality after applying the unsupported join
			// filters is less than the cardinality of outer child, we don't use this stats.
			// Because we need to make sure that Card(LOJ) >= Card(Outer child of LOJ).
			if (fLeftOuterJoin && pstatsAfterJoinFilter->DRows() < dRowsOuter)
			{
				pstatsAfterJoinFilter->Release();
			}
			else
			{
				pstats->Release();
				pstats = pstatsAfterJoinFilter;
			}

			pstatspredUnsupported->Release();
		}

		pdrgpstatspredjoin->Release();
		pdrgpcrsOutput->Release();
	}

	// clean up
	pcrsOuterRefs->Release();

	return pstats;
}


// main driver to generate join stats
CStatistics *
CJoinStatsProcessor::PstatsJoinDriver
		(
		IMemoryPool *memory_pool,
		CStatisticsConfig *pstatsconf,
		const IStatistics *pistatsOuter,
		const IStatistics *pistatsInner,
		DrgPstatspredjoin *pdrgppredInfo,
		IStatistics::EStatsJoinType eStatsJoinType,
		BOOL fIgnoreLasjHistComputation
		)
{
	GPOS_ASSERT(NULL != memory_pool);
	GPOS_ASSERT(NULL != pistatsInner);
	GPOS_ASSERT(NULL != pistatsOuter);

	GPOS_ASSERT(NULL != pdrgppredInfo);

	BOOL fLASJ = (IStatistics::EsjtLeftAntiSemiJoin == eStatsJoinType);
	BOOL fSemiJoin = IStatistics::FSemiJoin(eStatsJoinType);

	// Extract stat objects for inner and outer child.
	// Historically, IStatistics was meant to have multiple derived classes
	// However, currently only CStatistics implements IStatistics
	// Until this changes, the interfaces have been designed to take IStatistics as parameters
	// In the future, IStatistics should be removed, as it is not being utilized as designed
	const CStatistics *pstatsOuter = dynamic_cast<const CStatistics *> (pistatsOuter);
	const CStatistics *pstatsInner = dynamic_cast<const CStatistics *> (pistatsInner);

	// create hash map from colid -> histogram for resultant structure
	HMUlHist *phmulhistJoin = GPOS_NEW(memory_pool) HMUlHist(memory_pool);

	// build a bitset with all join columns
	CBitSet *pbsJoinColIds = GPOS_NEW(memory_pool) CBitSet(memory_pool);
	for (ULONG ul = 0; ul < pdrgppredInfo->Size(); ul++)
	{
		CStatsPredJoin *pstatsjoin = (*pdrgppredInfo)[ul];

		(void) pbsJoinColIds->ExchangeSet(pstatsjoin->UlColId1());
		if (!fSemiJoin)
		{
			(void) pbsJoinColIds->ExchangeSet(pstatsjoin->UlColId2());
		}
	}

	// histograms on columns that do not appear in join condition will
	// be copied over to the result structure
	pstatsOuter->AddNotExcludedHistograms(memory_pool, pbsJoinColIds, phmulhistJoin);
	if (!fSemiJoin)
	{
		pstatsInner->AddNotExcludedHistograms(memory_pool, pbsJoinColIds, phmulhistJoin);
	}

	DrgPdouble *pdrgpd = GPOS_NEW(memory_pool) DrgPdouble(memory_pool);
	const ULONG ulJoinConds = pdrgppredInfo->Size();

	BOOL fEmptyOutput = false;
	CDouble dRowsJoin = 0;
	// iterate over join's predicate(s)
	for (ULONG ul = 0; ul < ulJoinConds; ul++)
	{
		CStatsPredJoin *ppredInfo = (*pdrgppredInfo)[ul];
		ULONG ulColId1 = ppredInfo->UlColId1();
		ULONG ulColId2 = ppredInfo->UlColId2();
		GPOS_ASSERT(ulColId1 != ulColId2);
		// find the histograms corresponding to the two columns
		const CHistogram *phistOuter = pstatsOuter->Phist(ulColId1);
		// are column id1 and 2 always in the order of outer inner?
		const CHistogram *phistInner = pstatsInner->Phist(ulColId2);
		GPOS_ASSERT(NULL != phistOuter);
		GPOS_ASSERT(NULL != phistInner);
		BOOL fEmptyInput = CStatistics::FEmptyJoinInput(pstatsOuter, pstatsInner, fLASJ);

		CDouble dScaleFactorLocal(1.0);
		CHistogram *phistOuterAfter = NULL;
		CHistogram *phistInnerAfter = NULL;
		JoinHistograms
				(
						memory_pool,
						phistOuter,
						phistInner,
						ppredInfo,
						pstatsOuter->DRows(),
						pstatsInner->DRows(),
						&phistOuterAfter,
						&phistInnerAfter,
						&dScaleFactorLocal,
						fEmptyInput,
						eStatsJoinType,
						fIgnoreLasjHistComputation
				);

		fEmptyOutput = FEmptyJoinStats(pstatsOuter->IsEmpty(), fEmptyOutput, phistOuter, phistInner, phistOuterAfter, eStatsJoinType);

		CStatisticsUtils::AddHistogram(memory_pool, ulColId1, phistOuterAfter, phmulhistJoin);
		if (!fSemiJoin)
		{
			CStatisticsUtils::AddHistogram(memory_pool, ulColId2, phistInnerAfter, phmulhistJoin);
		}
		GPOS_DELETE(phistOuterAfter);
		GPOS_DELETE(phistInnerAfter);

		pdrgpd->Append(GPOS_NEW(memory_pool) CDouble(dScaleFactorLocal));
	}


	dRowsJoin = CStatistics::DMinRows;
	if (!fEmptyOutput)
	{
		dRowsJoin = DJoinCardinality(pstatsconf, pstatsOuter->DRows(), pstatsInner->DRows(), pdrgpd, eStatsJoinType);
	}

	// clean up
	pdrgpd->Release();
	pbsJoinColIds->Release();

	HMUlDouble *phmuldoubleWidthResult = pstatsOuter->CopyWidths(memory_pool);
	if (!fSemiJoin)
	{
		pstatsInner->CopyWidthsInto(memory_pool, phmuldoubleWidthResult);
	}

	// create an output stats object
	CStatistics *pstatsJoin = GPOS_NEW(memory_pool) CStatistics
			(
					memory_pool,
					phmulhistJoin,
					phmuldoubleWidthResult,
					dRowsJoin,
					fEmptyOutput,
					pstatsOuter->UlNumberOfPredicates()
			);

	// In the output statistics object, the upper bound source cardinality of the join column
	// cannot be greater than the upper bound source cardinality information maintained in the input
	// statistics object. Therefore we choose CStatistics::EcbmMin the bounding method which takes
	// the minimum of the cardinality upper bound of the source column (in the input hash map)
	// and estimated join cardinality.

	CStatisticsUtils::ComputeCardUpperBounds(memory_pool, pstatsOuter, pstatsJoin, dRowsJoin, CStatistics::EcbmMin /* ecbm */);
	if (!fSemiJoin)
	{
		CStatisticsUtils::ComputeCardUpperBounds(memory_pool, pstatsInner, pstatsJoin, dRowsJoin, CStatistics::EcbmMin /* ecbm */);
	}

	return pstatsJoin;
}


// return join cardinality based on scaling factor and join type
CDouble
CJoinStatsProcessor::DJoinCardinality
		(
		CStatisticsConfig *pstatsconf,
		CDouble dRowsLeft,
		CDouble dRowsRight,
		DrgPdouble *pdrgpd,
		IStatistics::EStatsJoinType eStatsJoinType
		)
{
	GPOS_ASSERT(NULL != pstatsconf);
	GPOS_ASSERT(NULL != pdrgpd);

	CDouble dScaleFactor = CScaleFactorUtils::DCumulativeJoinScaleFactor(pstatsconf, pdrgpd);
	CDouble dCartesianProduct = dRowsLeft * dRowsRight;

	if (IStatistics::EsjtLeftAntiSemiJoin == eStatsJoinType || IStatistics::EsjtLeftSemiJoin == eStatsJoinType)
	{
		CDouble dRows = dRowsLeft;

		if (IStatistics::EsjtLeftAntiSemiJoin == eStatsJoinType)
		{
			dRows = dRowsLeft / dScaleFactor;
		}
		else
		{
			// semi join results cannot exceed size of outer side
			dRows = std::min(dRowsLeft.Get(), (dCartesianProduct / dScaleFactor).Get());
		}

		return std::max(DOUBLE(1.0), dRows.Get());
	}

	GPOS_ASSERT(CStatistics::DMinRows <= dScaleFactor);

	return std::max(CStatistics::DMinRows.Get(), (dCartesianProduct / dScaleFactor).Get());
}



// check if the join statistics object is empty output based on the input
// histograms and the join histograms
BOOL
CJoinStatsProcessor::FEmptyJoinStats
		(
		BOOL fEmptyOuter,
		BOOL fEmptyOutput,
		const CHistogram *phistOuter,
		const CHistogram *phistInner,
		CHistogram *phistJoin,
		IStatistics::EStatsJoinType eStatsJoinType
		)
{
	GPOS_ASSERT(NULL != phistOuter);
	GPOS_ASSERT(NULL != phistInner);
	GPOS_ASSERT(NULL != phistJoin);
	BOOL fLASJ = IStatistics::EsjtLeftAntiSemiJoin == eStatsJoinType;
	return fEmptyOutput ||
		   (!fLASJ && fEmptyOuter) ||
		   (!phistOuter->IsEmpty() && !phistInner->IsEmpty() && phistJoin->IsEmpty());
}

// Derive statistics for join operation given array of statistics object
IStatistics *
CJoinStatsProcessor::PstatsJoin
		(
		IMemoryPool *memory_pool,
		CExpressionHandle &exprhdl,
		DrgPstat *pdrgpstatCtxt
		)
{
	GPOS_ASSERT(CLogical::EspNone < CLogical::PopConvert(exprhdl.Pop())->Esp(exprhdl));

	DrgPstat *statistics_array = GPOS_NEW(memory_pool) DrgPstat(memory_pool);
	const ULONG ulArity = exprhdl.UlArity();
	for (ULONG ul = 0; ul < ulArity - 1; ul++)
	{
		IStatistics *pstatsChild = exprhdl.Pstats(ul);
		pstatsChild->AddRef();
		statistics_array->Append(pstatsChild);
	}

	CExpression *pexprJoinPred = NULL;
	if (exprhdl.Pdpscalar(ulArity - 1)->FHasSubquery())
	{
		// in case of subquery in join predicate, assume join condition is True
		pexprJoinPred = CUtils::PexprScalarConstBool(memory_pool, true /*fVal*/);
	}
	else
	{
		// remove implied predicates from join condition to avoid cardinality under-estimation
		pexprJoinPred = CPredicateUtils::PexprRemoveImpliedConjuncts(memory_pool, exprhdl.PexprScalarChild(ulArity - 1), exprhdl);
	}

	// split join predicate into local predicate and predicate involving outer references
	CExpression *pexprLocal = NULL;
	CExpression *pexprOuterRefs = NULL;

	// get outer references from expression handle
	CColRefSet *pcrsOuter = exprhdl.Pdprel()->PcrsOuter();

	CPredicateUtils::SeparateOuterRefs(memory_pool, pexprJoinPred, pcrsOuter, &pexprLocal, &pexprOuterRefs);
	pexprJoinPred->Release();

	COperator::EOperatorId eopid = exprhdl.Pop()->Eopid();
	GPOS_ASSERT(COperator::EopLogicalLeftOuterJoin == eopid ||
				COperator::EopLogicalInnerJoin == eopid ||
				COperator::EopLogicalNAryJoin == eopid);

	// we use Inner Join semantics here except in the case of Left Outer Join
	IStatistics::EStatsJoinType eStatsJoinType = IStatistics::EsjtInnerJoin;
	if (COperator::EopLogicalLeftOuterJoin == eopid)
	{
		eStatsJoinType = IStatistics::EsjtLeftOuterJoin;
	}

	// derive stats based on local join condition
	IStatistics *pstatsJoin = CJoinStatsProcessor::PstatsJoinArray(memory_pool, statistics_array, pexprLocal, eStatsJoinType);

	if (exprhdl.FHasOuterRefs() && 0 < pdrgpstatCtxt->Size())
	{
		// derive stats based on outer references
		IStatistics *pstats = PstatsDeriveWithOuterRefs(memory_pool, exprhdl, pexprOuterRefs, pstatsJoin, pdrgpstatCtxt, eStatsJoinType);
		pstatsJoin->Release();
		pstatsJoin = pstats;
	}

	pexprLocal->Release();
	pexprOuterRefs->Release();

	statistics_array->Release();

	return pstatsJoin;
}


// Derives statistics when the scalar expression contains one or more outer references.
// This stats derivation mechanism passes around a context array onto which
// operators append their stats objects as they get derived. The context array is
// filled as we derive stats on the children of a given operator. This gives each
// operator access to the stats objects of its previous siblings as well as to the outer
// operators in higher levels.
// For example, in this expression:
//
// JOIN
//   |--Get(R)
//   +--Select(R.r=S.s)
//       +-- Get(S)
//
// We start by deriving stats on JOIN's left child (Get(R)) and append its
// stats to the context. Then, we call stats derivation on JOIN's right child
// (SELECT), passing it the current context.  This gives SELECT access to the
// histogram on column R.r--which is an outer reference in this example. After
// JOIN's children's stats are computed, JOIN combines them into a parent stats
// object, which is passed upwards to JOIN's parent. This mechanism gives any
// operator access to the histograms of outer references defined anywhere in
// the logical tree. For example, we also support the case where outer
// reference R.r is defined two levels upwards:
//
//    JOIN
//      |---Get(R)
//      +--JOIN
//         |--Get(T)
//         +--Select(R.r=S.s)
//               +--Get(S)
//
//
//
// The next step is to combine the statistics objects of the outer references
// with those of the local columns. You can think of this as a correlated
// expression, where for each outer tuple, we need to extract the outer ref
// value and re-execute the inner expression using the current outer ref value.
// This has the same semantics as a Join from a statistics perspective.
//
// We pull statistics for outer references from the passed statistics context,
// using Join statistics derivation in this case.
//
// For example:
//
// 			Join
// 			 |--Get(R)
// 			 +--Join
// 				|--Get(S)
// 				+--Select(T.t=R.r)
// 					+--Get(T)
//
// when deriving statistics on 'Select(T.t=R.r)', we join T with the cross
// product (R x S) based on the condition (T.t=R.r)
IStatistics *
CJoinStatsProcessor::PstatsDeriveWithOuterRefs
		(
		IMemoryPool *memory_pool,
		CExpressionHandle &
#ifdef GPOS_DEBUG
		exprhdl // handle attached to the logical expression we want to derive stats for
#endif // GPOS_DEBUG
,
		CExpression *pexprScalar, // scalar condition to be used for stats derivation
		IStatistics *pstats, // statistics object of the attached expression
		DrgPstat *pdrgpstatOuter, // array of stats objects where outer references are defined
		IStatistics::EStatsJoinType eStatsJoinType
		)
{
	GPOS_ASSERT(exprhdl.FHasOuterRefs() && "attached expression does not have outer references");
	GPOS_ASSERT(NULL != pexprScalar);
	GPOS_ASSERT(NULL != pstats);
	GPOS_ASSERT(NULL != pdrgpstatOuter);
	GPOS_ASSERT(0 < pdrgpstatOuter->Size());
	GPOS_ASSERT(IStatistics::EstiSentinel != eStatsJoinType);

	// join outer stats object based on given scalar expression,
	// we use inner join semantics here to consider all relevant combinations of outer tuples
	IStatistics *pstatsOuter = CJoinStatsProcessor::PstatsJoinArray(memory_pool, pdrgpstatOuter, pexprScalar, IStatistics::EsjtInnerJoin);
	CDouble dRowsOuter = pstatsOuter->DRows();

	// join passed stats object and outer stats based on the passed join type
	DrgPstat *statistics_array = GPOS_NEW(memory_pool) DrgPstat(memory_pool);
	statistics_array->Append(pstatsOuter);
	pstats->AddRef();
	statistics_array->Append(pstats);
	IStatistics *pstatsJoined = CJoinStatsProcessor::PstatsJoinArray(memory_pool, statistics_array, pexprScalar, eStatsJoinType);
	statistics_array->Release();

	// scale result using cardinality of outer stats and set number of rebinds of returned stats
	IStatistics *pstatsResult = pstatsJoined->PstatsScale(memory_pool, CDouble(1.0/dRowsOuter));
	pstatsResult->SetRebinds(dRowsOuter);
	pstatsJoined->Release();

	return pstatsResult;
}

// EOF
