//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 Pivotal Inc.
//
//	@filename:
//		CHistogramTest.cpp
//
//	@doc:
//		Testing operations on histogram objects
//---------------------------------------------------------------------------

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

#include <stdint.h>

#include "gpos/io/COstreamString.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/statistics/CPoint.h"
#include "naucrates/statistics/CHistogram.h"

#include "unittest/base.h"
#include "unittest/dxl/statistics/CCardinalityTestUtils.h"
#include "unittest/dxl/statistics/CHistogramTest.h"
#include "unittest/gpopt/CTestUtils.h"

using namespace gpopt;

// unittest for statistics objects
GPOS_RESULT
CHistogramTest::EresUnittest()
{
	// tests that use shared optimization context
	CUnittest rgutSharedOptCtxt[] =
		{
		GPOS_UNITTEST_FUNC(CHistogramTest::EresUnittest_CHistogramInt4),
		GPOS_UNITTEST_FUNC(CHistogramTest::EresUnittest_CHistogramBool),
		GPOS_UNITTEST_FUNC(CHistogramTest::EresUnittest_Skew),
		GPOS_UNITTEST_FUNC(CHistogramTest::EresUnittest_CHistogramValid)
		};

	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	// setup a file-based provider
	CMDProviderMemory *pmdp = CTestUtils::m_pmdpf;
	pmdp->AddRef();
	CMDAccessor mda(memory_pool, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc(memory_pool, &mda, NULL /* pceeval */, CTestUtils::Pcm(memory_pool));

	return CUnittest::EresExecute(rgutSharedOptCtxt, GPOS_ARRAY_SIZE(rgutSharedOptCtxt));
}

// histogram of int4
GPOS_RESULT
CHistogramTest::EresUnittest_CHistogramInt4()
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	// original histogram
	CHistogram *phist = CCardinalityTestUtils::PhistExampleInt4(memory_pool);
	CCardinalityTestUtils::PrintHist(memory_pool, "phist", phist);

	// test edge case of PbucketGreaterThan
	CPoint *ppoint0 = CTestUtils::PpointInt4(memory_pool, 9);
	CHistogram *phist0 = phist->PhistFilter(memory_pool, CStatsPred::EstatscmptG, ppoint0);
	CCardinalityTestUtils::PrintHist(memory_pool, "phist0", phist0);
	GPOS_RTL_ASSERT(phist0->UlBuckets() == 9);

	CPoint *ppoint1 = CTestUtils::PpointInt4(memory_pool, 35);
	CHistogram *phist1 = phist->PhistFilter(memory_pool, CStatsPred::EstatscmptL, ppoint1);
	CCardinalityTestUtils::PrintHist(memory_pool, "phist1", phist1);
	GPOS_RTL_ASSERT(phist1->UlBuckets() == 4);

	// edge case where point is equal to upper bound
	CPoint *ppoint2 = CTestUtils::PpointInt4(memory_pool, 50);
	CHistogram *phist2 = phist->PhistFilter(memory_pool, CStatsPred::EstatscmptL,ppoint2);
	CCardinalityTestUtils::PrintHist(memory_pool, "phist2", phist2);
	GPOS_RTL_ASSERT(phist2->UlBuckets() == 5);

	// equality check
	CPoint *ppoint3 = CTestUtils::PpointInt4(memory_pool, 100);
	CHistogram *phist3 = phist->PhistFilter(memory_pool, CStatsPred::EstatscmptEq, ppoint3);
	CCardinalityTestUtils::PrintHist(memory_pool, "phist3", phist3);
	GPOS_RTL_ASSERT(phist3->UlBuckets() == 1);

	// normalized output after filter
	CPoint *ppoint4 = CTestUtils::PpointInt4(memory_pool, 100);
	CDouble dScaleFactor(0.0);
	CHistogram *phist4 = phist->PhistFilterNormalized(memory_pool, CStatsPred::EstatscmptEq, ppoint4, &dScaleFactor);
	CCardinalityTestUtils::PrintHist(memory_pool, "phist4", phist4);
	GPOS_RTL_ASSERT(phist4->IsValid());

	// lasj
	CHistogram *phist5 = phist->PhistLASJ(memory_pool, CStatsPred::EstatscmptEq, phist2);
	CCardinalityTestUtils::PrintHist(memory_pool, "phist5", phist5);
	GPOS_RTL_ASSERT(phist5->UlBuckets() == 5);

	// inequality check
	CHistogram *phist6 = phist->PhistFilter(memory_pool, CStatsPred::EstatscmptNEq, ppoint2);
	CCardinalityTestUtils::PrintHist(memory_pool, "phist6", phist6);
	GPOS_RTL_ASSERT(phist6->UlBuckets() == 10);

	// histogram with null fraction and remaining tuples
	CHistogram *phist7 = PhistExampleInt4Remain(memory_pool);
	CCardinalityTestUtils::PrintHist(memory_pool, "phist7", phist7);
	CPoint *ppoint5 = CTestUtils::PpointInt4(memory_pool, 20);

	// equality check, hitting remaining tuples
	CHistogram *phist8 = phist7->PhistFilter(memory_pool, CStatsPred::EstatscmptEq, ppoint3);
	GPOS_RTL_ASSERT(fabs((phist8->DFrequency() - 0.2).Get()) < CStatistics::DEpsilon);
	GPOS_RTL_ASSERT(fabs((phist8->DDistinct() - 1.0).Get()) < CStatistics::DEpsilon);

	// greater than, hitting remaining tuples
	CHistogram *phist9 = phist7->PhistFilter(memory_pool, CStatsPred::EstatscmptG, ppoint1);
	CCardinalityTestUtils::PrintHist(memory_pool, "phist9", phist9);
	GPOS_RTL_ASSERT(fabs((phist9->DFrequency() - 0.26).Get()) < CStatistics::DEpsilon);
	GPOS_RTL_ASSERT(fabs((phist9->DDistinct() - 1.8).Get()) < CStatistics::DEpsilon);

	// equality join, hitting remaining tuples
	CHistogram *phist10 = phist7->PhistJoin(memory_pool, CStatsPred::EstatscmptEq, phist7);
	GPOS_RTL_ASSERT(phist10->UlBuckets() == 5);
	GPOS_RTL_ASSERT(fabs((phist10->DDistinctRemain() - 2.0).Get()) < CStatistics::DEpsilon);
	GPOS_RTL_ASSERT(fabs((phist10->DFreqRemain() - 0.08).Get()) < CStatistics::DEpsilon);

	// clean up
	ppoint0->Release();
	ppoint1->Release();
	ppoint2->Release();
	ppoint3->Release();
	ppoint4->Release();
	ppoint5->Release();
	GPOS_DELETE(phist);
	GPOS_DELETE(phist0);
	GPOS_DELETE(phist1);
	GPOS_DELETE(phist2);
	GPOS_DELETE(phist3);
	GPOS_DELETE(phist4);
	GPOS_DELETE(phist5);
	GPOS_DELETE(phist6);
	GPOS_DELETE(phist7);
	GPOS_DELETE(phist8);
	GPOS_DELETE(phist9);
	GPOS_DELETE(phist10);

	return GPOS_OK;
}

// histogram on bool
GPOS_RESULT
CHistogramTest::EresUnittest_CHistogramBool()
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	// generate histogram of the form [false, false), [true,true)
	DrgPbucket *pdrgppbucket = GPOS_NEW(memory_pool) DrgPbucket(memory_pool);
	CBucket *pbucketFalse = CCardinalityTestUtils::PbucketSingletonBoolVal(memory_pool, false, 0.1);
	CBucket *pbucketTrue = CCardinalityTestUtils::PbucketSingletonBoolVal(memory_pool, false, 0.9);
	pdrgppbucket->Append(pbucketFalse);
	pdrgppbucket->Append(pbucketTrue);
	CHistogram *phist =  GPOS_NEW(memory_pool) CHistogram(pdrgppbucket);

	// equality check
	CPoint *ppoint1 = CTestUtils::PpointBool(memory_pool, false);
	CDouble dScaleFactor(0.0);
	CHistogram *phist1 = phist->PhistFilterNormalized(memory_pool, CStatsPred::EstatscmptEq, ppoint1, &dScaleFactor);
	CCardinalityTestUtils::PrintHist(memory_pool, "phist1", phist1);
	GPOS_RTL_ASSERT(phist1->UlBuckets() == 1);

	// clean up
	ppoint1->Release();
	GPOS_DELETE(phist);
	GPOS_DELETE(phist1);

	return GPOS_OK;
}


// check for well-formed histogram. Expected to fail
GPOS_RESULT
CHistogramTest::EresUnittest_CHistogramValid()
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	DrgPbucket *pdrgppbucket = GPOS_NEW(memory_pool) DrgPbucket(memory_pool);

	// generate histogram of the form [0, 10), [9, 20)
	CBucket *pbucket1 = CCardinalityTestUtils::PbucketIntegerClosedLowerBound(memory_pool, 0, 10, 0.1, 2.0);
	pdrgppbucket->Append(pbucket1);
	CBucket *pbucket2 = CCardinalityTestUtils::PbucketIntegerClosedLowerBound(memory_pool, 9, 20, 0.1, 2.0);
	pdrgppbucket->Append(pbucket2);

	// original histogram
	CHistogram *phist =  GPOS_NEW(memory_pool) CHistogram(pdrgppbucket);

	// create an auto object
	CAutoP<CHistogram> ahist;
	ahist = phist;

	{
		CAutoTrace at(memory_pool);
		at.Os() << std::endl << "Invalid Histogram"<< std::endl;
		phist->OsPrint(at.Os());
	}

	if(phist->IsValid())
	{
		return GPOS_FAILED;
	}

	return GPOS_OK;
}

// generates example int histogram having tuples not covered by buckets,
// including null fraction and nDistinctRemain
CHistogram*
CHistogramTest::PhistExampleInt4Remain
	(
	IMemoryPool *memory_pool
	)
{
	// generate histogram of the form [0, 0], [10, 10], [20, 20] ...
	DrgPbucket *pdrgppbucket = GPOS_NEW(memory_pool) DrgPbucket(memory_pool);
	for (ULONG ulIdx = 0; ulIdx < 5; ulIdx++)
	{
		INT iLower = INT(ulIdx * 10);
		INT iUpper = iLower;
		CDouble dFrequency(0.1);
		CDouble dDistinct(1.0);
		CBucket *pbucket = CCardinalityTestUtils::PbucketIntegerClosedLowerBound(memory_pool, iLower, iUpper, dFrequency, dDistinct);
		pdrgppbucket->Append(pbucket);
	}

	return GPOS_NEW(memory_pool) CHistogram(pdrgppbucket, true, 0.1 /*dNullFreq*/, 2.0 /*dDistinctRemain*/, 0.4 /*dFreqRemain*/);
}

// basis skew test
GPOS_RESULT
CHistogramTest::EresUnittest_Skew()
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	CBucket *pbucket1 = CCardinalityTestUtils::PbucketIntegerClosedLowerBound(memory_pool, 1, 100, CDouble(0.6), CDouble(100.0));
	CBucket *pbucket2 = CCardinalityTestUtils::PbucketIntegerClosedLowerBound(memory_pool, 101, 200, CDouble(0.2), CDouble(100.0));
	CBucket *pbucket3 = CCardinalityTestUtils::PbucketIntegerClosedLowerBound(memory_pool, 201, 300, CDouble(0.2), CDouble(100.0));
	CBucket *pbucket4 = CCardinalityTestUtils::PbucketIntegerClosedLowerBound(memory_pool, 301, 400, CDouble(0.2), CDouble(100.0));
	CBucket *pbucket5 = CCardinalityTestUtils::PbucketIntegerClosedLowerBound(memory_pool, 401, 500, CDouble(0.2), CDouble(100.0));
	CBucket *pbucket6 = CCardinalityTestUtils::PbucketIntegerClosedLowerBound(memory_pool, 501, 600, CDouble(0.2), CDouble(100.0));
	CBucket *pbucket7 = CCardinalityTestUtils::PbucketIntegerClosedLowerBound(memory_pool, 601, 700, CDouble(0.2), CDouble(100.0));
	CBucket *pbucket8 = CCardinalityTestUtils::PbucketIntegerClosedLowerBound(memory_pool, 701, 800, CDouble(0.2), CDouble(100.0));

	DrgPbucket *pdrgppbucket1 = GPOS_NEW(memory_pool) DrgPbucket(memory_pool);
	pdrgppbucket1->Append(pbucket1);
	pdrgppbucket1->Append(pbucket2);
	pdrgppbucket1->Append(pbucket3);
	CHistogram *phist1 =  GPOS_NEW(memory_pool) CHistogram(pdrgppbucket1);

	DrgPbucket *pdrgppbucket2 = GPOS_NEW(memory_pool) DrgPbucket(memory_pool);
	pdrgppbucket2->Append(pbucket4);
	pdrgppbucket2->Append(pbucket5);
	pdrgppbucket2->Append(pbucket6);
	pdrgppbucket2->Append(pbucket7);
	pdrgppbucket2->Append(pbucket8);
	CHistogram *phist2 =  GPOS_NEW(memory_pool) CHistogram(pdrgppbucket2);
	GPOS_ASSERT(phist1->DSkew() > phist2->DSkew());

	{
		CAutoTrace at(memory_pool);
		phist1->OsPrint(at.Os());
		phist2->OsPrint(at.Os());
	}

	GPOS_DELETE(phist1);
	GPOS_DELETE(phist2);

	return GPOS_OK;
}

// EOF

