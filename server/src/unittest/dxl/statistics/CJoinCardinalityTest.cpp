//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 Pivotal, Inc.
//
//	@filename:
//		CJoinCardinalityTest.cpp
//
//	@doc:
//		Test for join cardinality estimation
//---------------------------------------------------------------------------

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

#include <stdint.h>

#include "gpos/io/COstreamString.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/statistics/CStatisticsUtils.h"

#include "naucrates/dxl/CDXLUtils.h"

#include "unittest/base.h"
#include "unittest/dxl/statistics/CCardinalityTestUtils.h"
#include "unittest/dxl/statistics/CJoinCardinalityTest.h"
#include "unittest/gpopt/CTestUtils.h"

// unittest for join cardinality estimation
GPOS_RESULT
CJoinCardinalityTest::EresUnittest()
{
	// tests that use shared optimization context
	CUnittest rgutSharedOptCtxt[] =
		{
		GPOS_UNITTEST_FUNC(CJoinCardinalityTest::EresUnittest_Join),
		GPOS_UNITTEST_FUNC(CJoinCardinalityTest::EresUnittest_JoinNDVRemain),
		};

	// run tests with shared optimization context first
	GPOS_RESULT eres = GPOS_FAILED;

	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	// setup a file-based provider
	CMDProviderMemory *pmdp = CTestUtils::m_pmdpf;
	pmdp->AddRef();
	CMDAccessor mda(memory_pool, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc
					(
					memory_pool,
					&mda,
					NULL /* pceeval */,
					CTestUtils::GetCostModel(memory_pool)
					);

	eres = CUnittest::EresExecute(rgutSharedOptCtxt, GPOS_ARRAY_SIZE(rgutSharedOptCtxt));

	return eres;
}

//	test join cardinality estimation over histograms with NDVRemain information
GPOS_RESULT
CJoinCardinalityTest::EresUnittest_JoinNDVRemain()
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	SHistogramTestCase rghisttc[] =
	{
		{0, 0, false, 0}, // empty histogram
		{10, 100, false, 0},  // distinct values only in buckets
		{0, 0, false, 1000},   // distinct values only in NDVRemain
		{5, 100, false, 500} // distinct values spread in both buckets and NDVRemain
	};

	HMUlHist *phmulhist = GPOS_NEW(memory_pool) HMUlHist(memory_pool);

	const ULONG ulHist = GPOS_ARRAY_SIZE(rghisttc);
	for (ULONG ul1 = 0; ul1 < ulHist; ul1++)
	{
		SHistogramTestCase elem = rghisttc[ul1];

		ULONG ulBuckets = elem.m_ulBuckets;
		CDouble dNDVPerBucket = elem.m_dNDVPerBucket;
		BOOL fNullFreq = elem.m_fNullFreq;
		CDouble dNDVRemain = elem.m_dNDVRemain;

		CHistogram *phist = CCardinalityTestUtils::PhistInt4Remain(memory_pool, ulBuckets, dNDVPerBucket, fNullFreq, dNDVRemain);
#ifdef GPOS_DEBUG
			BOOL fResult =
#endif // GPOS_DEBUG
		phmulhist->Insert(GPOS_NEW(memory_pool) ULONG(ul1), phist);
		GPOS_ASSERT(fResult);
	}

	SStatsJoinNDVRemainTestCase rgjoinndvrtc[] =
	{
		// cases where we are joining with an empty histogram
	    // first two columns refer to the histogram entries that are joining
		{0, 0, 0, CDouble(0.0), CDouble(0.0), CDouble(0.0)},
		{0, 1, 0, CDouble(0.0), CDouble(0.0), CDouble(0.0)},
		{0, 2, 0, CDouble(0.0), CDouble(0.0), CDouble(0.0)},
		{0, 3, 0, CDouble(0.0), CDouble(0.0), CDouble(0.0)},

		{1, 0, 0, CDouble(0.0), CDouble(0.0), CDouble(0.0)},
		{2, 0, 0, CDouble(0.0), CDouble(0.0), CDouble(0.0)},
		{3, 0, 0, CDouble(0.0), CDouble(0.0), CDouble(0.0)},

		// cases where one or more input histogram has only buckets and no remaining NDV information
		{1, 1, 10, CDouble(1000.00), CDouble(0.0), CDouble(0.0)},
		{1, 3, 5, CDouble(500.00), CDouble(500.0), CDouble(0.333333)},
		{3, 1, 5, CDouble(500.00), CDouble(500.0), CDouble(0.333333)},

		// cases where for one or more input histogram has only remaining NDV information and no buckets
		{1, 2, 0, CDouble(0.0), CDouble(1000.0), CDouble(1.0)},
		{2, 1, 0, CDouble(0.0), CDouble(1000.0), CDouble(1.0)},
		{2, 2, 0, CDouble(0.0), CDouble(1000.0), CDouble(1.0)},
		{2, 3, 0, CDouble(0.0), CDouble(1000.0), CDouble(1.0)},
		{3, 2, 0, CDouble(0.0), CDouble(1000.0), CDouble(1.0)},

		// cases where both buckets and NDV remain information available for both inputs
		{3, 3, 5, CDouble(500.0), CDouble(500.0), CDouble(0.5)},
	};

	GPOS_RESULT eres = GPOS_OK;
	const ULONG ulTestCases = GPOS_ARRAY_SIZE(rgjoinndvrtc);
	for (ULONG ul2 = 0; ul2 < ulTestCases && (GPOS_FAILED != eres); ul2++)
	{
		SStatsJoinNDVRemainTestCase elem = rgjoinndvrtc[ul2];
		ULONG ulColId1 = elem.m_ulCol1;
		ULONG ulColId2 = elem.m_ulCol2;
		CHistogram *phist1 = phmulhist->Find(&ulColId1);
		CHistogram *phist2 = phmulhist->Find(&ulColId2);

		CHistogram *phistJoin = phist1->PhistJoin(memory_pool, CStatsPred::EstatscmptEq, phist2);

		{
			CAutoTrace at(memory_pool);
			at.Os() <<  std::endl << "Input Histogram 1" <<  std::endl;
			phist1->OsPrint(at.Os());
			at.Os() << "Input Histogram 2" <<  std::endl;
			phist2->OsPrint(at.Os());
			at.Os() << "Join Histogram" <<  std::endl;
			phistJoin->OsPrint(at.Os());

			phistJoin->DNormalize();

			at.Os() <<  std::endl << "Normalized Join Histogram" <<  std::endl;
			phistJoin->OsPrint(at.Os());
		}

		ULONG ulBucketsJoin = elem.m_ulBucketsJoin;
		CDouble dNDVBucketsJoin = elem.m_dNDVBucketsJoin;
		CDouble dNDVRemainJoin = elem.m_dNDVRemainJoin;
		CDouble dFreqRemainJoin = elem.m_dFreqRemainJoin;

		CDouble dDiffNDVJoin(fabs((dNDVBucketsJoin - CStatisticsUtils::DDistinct(phistJoin->ParseDXLToBucketsArray())).Get()));
		CDouble dDiffNDVRemainJoin(fabs((dNDVRemainJoin - phistJoin->DDistinctRemain()).Get()));
		CDouble dDiffFreqRemainJoin(fabs((dFreqRemainJoin - phistJoin->DFreqRemain()).Get()));

		if (phistJoin->UlBuckets() != ulBucketsJoin || (dDiffNDVJoin > CStatistics::DEpsilon)
			|| (dDiffNDVRemainJoin > CStatistics::DEpsilon) || (dDiffFreqRemainJoin > CStatistics::DEpsilon))
		{
			eres = GPOS_FAILED;
		}

		GPOS_DELETE(phistJoin);
	}
	// clean up
	phmulhist->Release();

	return eres;
}

//	join buckets tests
GPOS_RESULT
CJoinCardinalityTest::EresUnittest_Join()
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();

	SStatsJoinSTestCase rgstatsjointc[] =
	{
		{"../data/dxl/statistics/Join-Statistics-Input.xml", "../data/dxl/statistics/Join-Statistics-Output.xml", false, PdrgpstatspredjoinMultiplePredicates},
		{"../data/dxl/statistics/Join-Statistics-Input-Null-Bucket.xml", "../data/dxl/statistics/Join-Statistics-Output-Null-Bucket.xml", false, PdrgpstatspredjoinNullableCols},
		{"../data/dxl/statistics/LOJ-Input.xml", "../data/dxl/statistics/LOJ-Output.xml", true, PdrgpstatspredjoinNullableCols},
		{"../data/dxl/statistics/Join-Statistics-Input-Only-Nulls.xml", "../data/dxl/statistics/Join-Statistics-Output-Only-Nulls.xml", false, PdrgpstatspredjoinNullableCols},
		{"../data/dxl/statistics/Join-Statistics-Input-Only-Nulls.xml", "../data/dxl/statistics/Join-Statistics-Output-LOJ-Only-Nulls.xml", true, PdrgpstatspredjoinNullableCols},
	    {"../data/dxl/statistics/Join-Statistics-DDistinct-Input.xml", "../data/dxl/statistics/Join-Statistics-DDistinct-Output.xml", false, PdrgpstatspredjoinSingleJoinPredicate},
		{"../data/dxl/statistics/Join-Statistics-Text-Input.xml", "../data/dxl/statistics/Join-Statistics-Text-Output.xml", false, PdrgpstatspredjoinSingleJoinPredicate},
	};

	const ULONG ulTestCases = GPOS_ARRAY_SIZE(rgstatsjointc);
	for (ULONG ul = 0; ul < ulTestCases; ul++)
	{
		SStatsJoinSTestCase elem = rgstatsjointc[ul];

		// read input/output DXL file
		CHAR *szDXLInput = CDXLUtils::Read(memory_pool, elem.m_szInputFile);
		CHAR *szDXLOutput = CDXLUtils::Read(memory_pool, elem.m_szOutputFile);
		BOOL fLeftOuterJoin = elem.m_fLeftOuterJoin;

		GPOS_CHECK_ABORT;

		// parse the input statistics objects
		DrgPdxlstatsderrel *dxl_derived_rel_stats_array = CDXLUtils::ParseDXLToStatsDerivedRelArray(memory_pool, szDXLInput, NULL);
		CStatisticsArray *pdrgpstatBefore = CDXLUtils::ParseDXLToOptimizerStatisticObjArray(memory_pool, md_accessor, dxl_derived_rel_stats_array);
		dxl_derived_rel_stats_array->Release();

		GPOS_ASSERT(NULL != pdrgpstatBefore);
		GPOS_ASSERT(2 == pdrgpstatBefore->Size());
		CStatistics *pstats1 = (*pdrgpstatBefore)[0];
		CStatistics *pstats2 = (*pdrgpstatBefore)[1];

		GPOS_CHECK_ABORT;

		// generate the join conditions
		FnPdrgpstatjoin *pf = elem.m_pf;
		GPOS_ASSERT(NULL != pf);
		DrgPstatspredjoin *pdrgpstatspredjoin = pf(memory_pool);

		// calculate the output stats
		CStatistics *pstatsOutput = NULL;
		if (fLeftOuterJoin)
		{
			pstatsOutput = pstats1->PstatsLOJ(memory_pool, pstats2, pdrgpstatspredjoin);
		}
		else
		{
			pstatsOutput = pstats1->PstatsInnerJoin(memory_pool, pstats2, pdrgpstatspredjoin);
		}
		GPOS_ASSERT(NULL != pstatsOutput);

		CStatisticsArray *pdrgpstatOutput = GPOS_NEW(memory_pool) CStatisticsArray(memory_pool);
		pdrgpstatOutput->Append(pstatsOutput);

		// serialize and compare against expected stats
		CWStringDynamic *pstrOutput = CDXLUtils::SerializeStatistics
													(
													memory_pool,
													md_accessor,
													pdrgpstatOutput,
													true /*serialize_header_footer*/,
													true /*indentation*/
													);
		CWStringDynamic dstrExpected(memory_pool);
		dstrExpected.AppendFormat(GPOS_WSZ_LIT("%s"), szDXLOutput);

		GPOS_RESULT eres = GPOS_OK;
		CWStringDynamic str(memory_pool);
		COstreamString oss(&str);

		// compare the two dxls
		if (!pstrOutput->Equals(&dstrExpected))
		{
			oss << "Output does not match expected DXL document" << std::endl;
			oss << "Actual: " << std::endl;
			oss << pstrOutput->GetBuffer() << std::endl;
			oss << "Expected: " << std::endl;
			oss << dstrExpected.GetBuffer() << std::endl;
			GPOS_TRACE(str.GetBuffer());

			eres = GPOS_FAILED;
		}

		// clean up
		pdrgpstatBefore->Release();
		pdrgpstatOutput->Release();
		pdrgpstatspredjoin->Release();

		GPOS_DELETE_ARRAY(szDXLInput);
		GPOS_DELETE_ARRAY(szDXLOutput);
		GPOS_DELETE(pstrOutput);

		if (GPOS_FAILED == eres)
		{
			return eres;
		}
	}

	return GPOS_OK;
}

//	helper method to generate a single join predicate
DrgPstatspredjoin *
CJoinCardinalityTest::PdrgpstatspredjoinSingleJoinPredicate
	(
	IMemoryPool *memory_pool
	)
{
	DrgPstatspredjoin *pdrgpstatspredjoin = GPOS_NEW(memory_pool) DrgPstatspredjoin(memory_pool);
	pdrgpstatspredjoin->Append(GPOS_NEW(memory_pool) CStatsPredJoin(0, CStatsPred::EstatscmptEq, 8));

	return pdrgpstatspredjoin;
}

//	helper method to generate generate multiple join predicates
DrgPstatspredjoin *
CJoinCardinalityTest::PdrgpstatspredjoinMultiplePredicates
	(
	IMemoryPool *memory_pool
	)
{
	DrgPstatspredjoin *pdrgpstatspredjoin = GPOS_NEW(memory_pool) DrgPstatspredjoin(memory_pool);
	pdrgpstatspredjoin->Append(GPOS_NEW(memory_pool) CStatsPredJoin(16, CStatsPred::EstatscmptEq, 32));
	pdrgpstatspredjoin->Append(GPOS_NEW(memory_pool) CStatsPredJoin(0, CStatsPred::EstatscmptEq, 31));
	pdrgpstatspredjoin->Append(GPOS_NEW(memory_pool) CStatsPredJoin(54, CStatsPred::EstatscmptEq, 32));
	pdrgpstatspredjoin->Append(GPOS_NEW(memory_pool) CStatsPredJoin(53, CStatsPred::EstatscmptEq, 31));

	return pdrgpstatspredjoin;
}

// helper method to generate join predicate over columns that contain null values
DrgPstatspredjoin *
CJoinCardinalityTest::PdrgpstatspredjoinNullableCols
	(
	IMemoryPool *memory_pool
	)
{
	DrgPstatspredjoin *pdrgpstatspredjoin = GPOS_NEW(memory_pool) DrgPstatspredjoin(memory_pool);
	pdrgpstatspredjoin->Append(GPOS_NEW(memory_pool) CStatsPredJoin(1, CStatsPred::EstatscmptEq, 2));

	return pdrgpstatspredjoin;
}

// EOF
