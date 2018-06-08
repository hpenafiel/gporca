//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 Pivotal Inc.
//
//	@filename:
//		CHistogramTest.cpp
//
//	@doc:
//		Testing merging most common values (MCV) and histograms
//---------------------------------------------------------------------------

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

#include <stdint.h>

#include "gpos/io/COstreamString.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/statistics/CPoint.h"
#include "naucrates/statistics/CHistogram.h"
#include "naucrates/statistics/CStatistics.h"
#include "naucrates/statistics/CStatisticsUtils.h"

#include "naucrates/dxl/CDXLUtils.h"

#include "unittest/base.h"
#include "unittest/dxl/statistics/CCardinalityTestUtils.h"
#include "unittest/dxl/statistics/CMCVTest.h"
#include "unittest/gpopt/CTestUtils.h"

using namespace gpopt;

// DXL files
const CHAR *
szMCVSortExpectedFileName = "../data/dxl/statistics/MCV-Sort-Output.xml";


// unittest for statistics objects
GPOS_RESULT
CMCVTest::EresUnittest()
{
	// tests that use shared optimization context
	CUnittest rgutSharedOptCtxt[] =
		{
		GPOS_UNITTEST_FUNC(CMCVTest::EresUnittest_SortInt4MCVs),
		GPOS_UNITTEST_FUNC(CMCVTest::EresUnittest_MergeHistMCV),
		};

	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	// setup a file-based provider
	CMDProviderMemory *pmdp = CTestUtils::m_pmdpf;
	pmdp->AddRef();
	CMDAccessor mda(memory_pool, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc(memory_pool, &mda, NULL /* pceeval */, CTestUtils::GetCostModel(memory_pool));

	return CUnittest::EresExecute(rgutSharedOptCtxt, GPOS_ARRAY_SIZE(rgutSharedOptCtxt));
}

// test sorting of Int4 MCVs and their associated frequencies
GPOS_RESULT
CMCVTest::EresUnittest_SortInt4MCVs()
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	CMDIdGPDB *mdid = GPOS_NEW(memory_pool) CMDIdGPDB(CMDIdGPDB::m_mdidInt4);
	const IMDType *pmdtype = md_accessor->Pmdtype(mdid);

	// create three integer MCVs
	CPoint *ppoint1 = CTestUtils::PpointInt4(memory_pool, 5);
	CPoint *ppoint2 = CTestUtils::PpointInt4(memory_pool, 1);
	CPoint *ppoint3 = CTestUtils::PpointInt4(memory_pool, 10);
	DrgPdatum *pdrgpdatumMCV = GPOS_NEW(memory_pool) DrgPdatum(memory_pool);
	IDatum *pdatum1 = ppoint1->Pdatum();
	IDatum *pdatum2 = ppoint2->Pdatum();
	IDatum *pdatum3 = ppoint3->Pdatum();
	pdatum1->AddRef();
	pdatum2->AddRef();
	pdatum3->AddRef();
	pdrgpdatumMCV->Append(pdatum1);
	pdrgpdatumMCV->Append(pdatum2);
	pdrgpdatumMCV->Append(pdatum3);

	// create corresponding frequencies
	DrgPdouble *pdrgpdFreq = GPOS_NEW(memory_pool) DrgPdouble(memory_pool);
	// in GPDB, MCVs are stored in descending frequencies
	pdrgpdFreq->Append(GPOS_NEW(memory_pool) CDouble(0.4));
	pdrgpdFreq->Append(GPOS_NEW(memory_pool) CDouble(0.2));
	pdrgpdFreq->Append(GPOS_NEW(memory_pool) CDouble(0.1));

	// exercise MCV sorting function
	CHistogram *phistMCV = CStatisticsUtils::PhistTransformMCV
								(
								memory_pool,
								pmdtype,
								pdrgpdatumMCV,
								pdrgpdFreq,
								pdrgpdatumMCV->Size()
								);

	// create hash map from colid -> histogram
	HMUlHist *phmulhist = GPOS_NEW(memory_pool) HMUlHist(memory_pool);

	// generate int histogram for column 1
	phmulhist->Insert(GPOS_NEW(memory_pool) ULONG(1), phistMCV);

	// column width for int4
	HMUlDouble *phmuldoubleWidth = GPOS_NEW(memory_pool) HMUlDouble(memory_pool);
	phmuldoubleWidth->Insert(GPOS_NEW(memory_pool) ULONG(1), GPOS_NEW(memory_pool) CDouble(4.0));

	CStatistics *pstats = GPOS_NEW(memory_pool) CStatistics
									(
									memory_pool,
									phmulhist,
									phmuldoubleWidth,
									1000.0 /* rows */,
									false /* is_empty */
									);

	// put stats object in an array in order to serialize
	CStatisticsArray *pdrgpstats = GPOS_NEW(memory_pool) CStatisticsArray(memory_pool);
	pdrgpstats->Append(pstats);

	// serialize stats object
	CWStringDynamic *pstrOutput = CDXLUtils::SerializeStatistics(memory_pool, md_accessor, pdrgpstats, true, true);
	GPOS_TRACE(pstrOutput->GetBuffer());

	// get expected output
	CWStringDynamic str(memory_pool);
	COstreamString oss(&str);
	CHAR *szDXLExpected = CDXLUtils::Read(memory_pool, szMCVSortExpectedFileName);
	CWStringDynamic dstrExpected(memory_pool);
	dstrExpected.AppendFormat(GPOS_WSZ_LIT("%s"), szDXLExpected);

	GPOS_RESULT eres = CTestUtils::EresCompare
								(
								oss,
								pstrOutput,
								&dstrExpected,
								false // mismatch will not be ignored
								);

	// cleanup
	GPOS_DELETE(pstrOutput);
	GPOS_DELETE_ARRAY(szDXLExpected);
	pdrgpdatumMCV->Release();
	pdrgpdFreq->Release();
	pdrgpstats->Release();
	ppoint1->Release();
	ppoint2->Release();
	ppoint3->Release();
	mdid->Release();

	return eres;
}

// test merging MCVs and histogram
GPOS_RESULT
CMCVTest::EresUnittest_MergeHistMCV()
{
	SMergeTestElem rgMergeTestElem[] =
	{
		{
		"../data/dxl/statistics/Merge-Input-MCV-Int.xml",
		"../data/dxl/statistics/Merge-Input-Histogram-Int.xml",
		"../data/dxl/statistics/Merge-Output-Int.xml"
		},

		{
		"../data/dxl/statistics/Merge-Input-MCV-Numeric.xml",
		"../data/dxl/statistics/Merge-Input-Histogram-Numeric.xml",
		"../data/dxl/statistics/Merge-Output-Numeric.xml"
		},

		{
		"../data/dxl/statistics/Merge-Input-MCV-BPChar.xml",
		"../data/dxl/statistics/Merge-Input-Histogram-BPChar.xml",
		"../data/dxl/statistics/Merge-Output-BPChar.xml"
		}
	};

	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	ULONG ulLen = GPOS_ARRAY_SIZE(rgMergeTestElem);
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		// read input MCVs DXL file
		CHAR *szDXLInputMCV = CDXLUtils::Read(memory_pool, rgMergeTestElem[ul].szInputMCVFile);
		// read input histogram DXL file
		CHAR *szDXLInputHist = CDXLUtils::Read(memory_pool, rgMergeTestElem[ul].szInputHistFile);

		GPOS_CHECK_ABORT;

		CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();

		// parse the stats objects
		DrgPdxlstatsderrel *pdrgpdxlstatsderrelMCV = CDXLUtils::ParseDXLToStatsDerivedRelArray(memory_pool, szDXLInputMCV, NULL);
		DrgPdxlstatsderrel *pdrgpdxlstatsderrelHist = CDXLUtils::ParseDXLToStatsDerivedRelArray(memory_pool, szDXLInputHist, NULL);

		GPOS_CHECK_ABORT;

		CDXLStatsDerivedRelation *pdxlstatsderrelMCV = (*pdrgpdxlstatsderrelMCV)[0];
		const DrgPdxlstatsdercol *pdrgpdxlstatsdercolMCV = pdxlstatsderrelMCV->Pdrgpdxlstatsdercol();
		CDXLStatsDerivedColumn *pdxlstatsdercolMCV = (*pdrgpdxlstatsdercolMCV)[0];
		DrgPbucket *pdrgppbucketMCV = CDXLUtils::ParseDXLToBucketsArray(memory_pool, md_accessor, pdxlstatsdercolMCV);
		CHistogram *phistMCV =  GPOS_NEW(memory_pool) CHistogram(pdrgppbucketMCV);

		CDXLStatsDerivedRelation *pdxlstatsderrelHist = (*pdrgpdxlstatsderrelHist)[0];
		const DrgPdxlstatsdercol *pdrgpdxlstatsdercolHist = pdxlstatsderrelHist->Pdrgpdxlstatsdercol();
		CDXLStatsDerivedColumn *pdxlstatsdercolHist = (*pdrgpdxlstatsdercolHist)[0];
		DrgPbucket *pdrgppbucketHist = CDXLUtils::ParseDXLToBucketsArray(memory_pool, md_accessor, pdxlstatsdercolHist);
		CHistogram *phistHist =  GPOS_NEW(memory_pool) CHistogram(pdrgppbucketHist);

		GPOS_CHECK_ABORT;

		// exercise the merge
		CHistogram *phistMerged = CStatisticsUtils::PhistMergeMcvHist(memory_pool, phistMCV, phistHist);

		// create hash map from colid -> histogram
		HMUlHist *phmulhist = GPOS_NEW(memory_pool) HMUlHist(memory_pool);

		// generate int histogram for column 1
		ULONG col_id = pdxlstatsdercolMCV->GetColId();
		phmulhist->Insert(GPOS_NEW(memory_pool) ULONG(col_id), phistMerged);

		// column width for int4
		HMUlDouble *phmuldoubleWidth = GPOS_NEW(memory_pool) HMUlDouble(memory_pool);
		CDouble width = pdxlstatsdercolMCV->Width();
		phmuldoubleWidth->Insert(GPOS_NEW(memory_pool) ULONG(col_id), GPOS_NEW(memory_pool) CDouble(width));

		CStatistics *pstats = GPOS_NEW(memory_pool) CStatistics
										(
										memory_pool,
										phmulhist,
										phmuldoubleWidth,
										pdxlstatsderrelMCV->Rows(),
										pdxlstatsderrelMCV->IsEmpty()
										);

		// put stats object in an array in order to serialize
		CStatisticsArray *pdrgpstats = GPOS_NEW(memory_pool) CStatisticsArray(memory_pool);
		pdrgpstats->Append(pstats);

		// serialize stats object
		CWStringDynamic *pstrOutput = CDXLUtils::SerializeStatistics(memory_pool, md_accessor, pdrgpstats, true, true);
		GPOS_TRACE(pstrOutput->GetBuffer());

		// get expected output
		CWStringDynamic str(memory_pool);
		COstreamString oss(&str);
		CHAR *szDXLExpected = CDXLUtils::Read(memory_pool, rgMergeTestElem[ul].szMergedFile);
		CWStringDynamic dstrExpected(memory_pool);
		dstrExpected.AppendFormat(GPOS_WSZ_LIT("%s"), szDXLExpected);

		GPOS_RESULT eres = CTestUtils::EresCompare
									(
									oss,
									pstrOutput,
									&dstrExpected,
									false // mismatch will not be ignored
									);

		// cleanup
		GPOS_DELETE_ARRAY(szDXLInputMCV);
		GPOS_DELETE_ARRAY(szDXLInputHist);
		GPOS_DELETE_ARRAY(szDXLExpected);
		GPOS_DELETE(pstrOutput);
		pdrgpdxlstatsderrelMCV->Release();
		pdrgpdxlstatsderrelHist->Release();
		GPOS_DELETE(phistMCV);
		GPOS_DELETE(phistHist);
		pdrgpstats->Release();

		if (GPOS_OK != eres)
		{
			return eres;
		}
	}

	return GPOS_OK;
}

// EOF

