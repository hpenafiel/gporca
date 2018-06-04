//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 Pivotal, Inc.
//
//	@filename:
//		CFilterCardinalityTest.cpp
//
//	@doc:
//		Test for filter cardinality estimation
//---------------------------------------------------------------------------

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

#include <stdint.h>

#include "gpos/io/COstreamString.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/statistics/CStatisticsUtils.h"
#include "naucrates/statistics/CFilterStatsProcessor.h"
#include "naucrates/dxl/CDXLUtils.h"

#include "unittest/base.h"
#include "unittest/dxl/statistics/CCardinalityTestUtils.h"
#include "unittest/dxl/statistics/CStatisticsTest.h"
#include "unittest/dxl/statistics/CFilterCardinalityTest.h"

#include "unittest/gpopt/CTestUtils.h"

using namespace gpopt;

// DXL files
const CHAR *
szInputDXLFileName = "../data/dxl/statistics/Basic-Statistics-Input.xml";
const CHAR *
szOutputDXLFileName = "../data/dxl/statistics/Basic-Statistics-Output.xml";

const CTestUtils::STestCase rgtcStatistics[] =
{
	{"../data/dxl/statistics/Numeric-Input.xml", "../data/dxl/statistics/Numeric-Output-LT-MinBoundary.xml"},
	{"../data/dxl/statistics/Numeric-Input.xml", "../data/dxl/statistics/Numeric-Output-LTE-MinBoundary.xml"},
	{"../data/dxl/statistics/Numeric-Input.xml", "../data/dxl/statistics/Numeric-Output-E-MinBoundary.xml"},
	{"../data/dxl/statistics/Numeric-Input.xml", "../data/dxl/statistics/Numeric-Output-GT-MaxBoundary.xml"},
	{"../data/dxl/statistics/Numeric-Input.xml", "../data/dxl/statistics/Numeric-Output-GTE-MaxBoundary.xml"},
	{"../data/dxl/statistics/Numeric-Input.xml", "../data/dxl/statistics/Numeric-Output-E-MaxBoundary.xml"},

	{"../data/dxl/statistics/Numeric-Input2.xml", "../data/dxl/statistics/Numeric-Output-2-LT-MinBoundary.xml"},
	{"../data/dxl/statistics/Numeric-Input2.xml", "../data/dxl/statistics/Numeric-Output-2-LTE-MinBoundary.xml"},
	{"../data/dxl/statistics/Numeric-Input2.xml", "../data/dxl/statistics/Numeric-Output-2-E-MinBoundary.xml"},
	{"../data/dxl/statistics/Numeric-Input2.xml", "../data/dxl/statistics/Numeric-Output-2-GT-MinBoundary.xml"},
	{"../data/dxl/statistics/Numeric-Input2.xml", "../data/dxl/statistics/Numeric-Output-2-GTE-MinBoundary.xml"},

	{"../data/dxl/statistics/Numeric-Input2.xml", "../data/dxl/statistics/Numeric-Output-2-LT-MaxBoundary.xml"},
	{"../data/dxl/statistics/Numeric-Input2.xml", "../data/dxl/statistics/Numeric-Output-2-LTE-MaxBoundary.xml"},
	{"../data/dxl/statistics/Numeric-Input2.xml", "../data/dxl/statistics/Numeric-Output-2-GT-MaxBoundary.xml"},
	{"../data/dxl/statistics/Numeric-Input2.xml", "../data/dxl/statistics/Numeric-Output-2-GTE-MaxBoundary.xml"},
	{"../data/dxl/statistics/Numeric-Input2.xml", "../data/dxl/statistics/Numeric-Output-2-E-MaxBoundary.xml"},
};

// unittest for statistics objects
GPOS_RESULT
CFilterCardinalityTest::EresUnittest()
{
	// tests that use shared optimization context
	CUnittest rgutSharedOptCtxt[] =
		{
		GPOS_UNITTEST_FUNC(CFilterCardinalityTest::EresUnittest_CStatisticsBasicsFromDXLNumeric),
		GPOS_UNITTEST_FUNC(CFilterCardinalityTest::EresUnittest_CStatisticsFilter),
		GPOS_UNITTEST_FUNC(CFilterCardinalityTest::EresUnittest_CStatisticsFilterConj),
		GPOS_UNITTEST_FUNC(CFilterCardinalityTest::EresUnittest_CStatisticsFilterDisj),
		GPOS_UNITTEST_FUNC(CFilterCardinalityTest::EresUnittest_CStatisticsNestedPred),
		GPOS_UNITTEST_FUNC(CFilterCardinalityTest::EresUnittest_CStatisticsBasicsFromDXL),
		GPOS_UNITTEST_FUNC(CFilterCardinalityTest::EresUnittest_CStatisticsAccumulateCard)
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

// reads a DXL document, generates the statistics object, performs a
// filter operation on it, serializes it into a DXL document and
// compares the generated DXL document with the expected DXL document.
GPOS_RESULT
CFilterCardinalityTest::EresUnittest_CStatistics
	(
	SStatsFilterSTestCase rgstatsdisjtc[],
	ULONG ulTestCases
	)
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	CWStringDynamic str(memory_pool);
	COstreamString oss(&str);

	CMDAccessor *pmda = COptCtxt::PoctxtFromTLS()->Pmda();

	for (ULONG ul = 0; ul < ulTestCases; ul++)
	{
		SStatsFilterSTestCase elem = rgstatsdisjtc[ul];

		// read input/output DXL file
		CHAR *szDXLInput = CDXLUtils::Read(memory_pool, elem.m_szInputFile);
		CHAR *szDXLOutput = CDXLUtils::Read(memory_pool, elem.m_szOutputFile);

		GPOS_CHECK_ABORT;

		// parse the statistics objects
		DrgPdxlstatsderrel *dxl_derived_rel_stats_array = CDXLUtils::ParseDXLToStatsDerivedRelArray(memory_pool, szDXLInput, NULL);
		CStatisticsArray *pdrgpstatBefore = CDXLUtils::ParseDXLToOptimizerStatisticObjArray(memory_pool, pmda, dxl_derived_rel_stats_array);
		dxl_derived_rel_stats_array->Release();
		GPOS_ASSERT(NULL != pdrgpstatBefore);

		GPOS_CHECK_ABORT;

		// generate the disjunctive predicate
		FnPstatspredDisj *pf = elem.m_pf;
		GPOS_ASSERT(NULL != pf);
		CStatsPred *pstatspredDisj = pf(memory_pool);

		GPOS_RESULT eres = EresUnittest_CStatisticsCompare
									(
									memory_pool,
									pmda,
									pdrgpstatBefore,
									pstatspredDisj,
									szDXLOutput
									);

		// clean up
		pdrgpstatBefore->Release();
		pstatspredDisj->Release();
		GPOS_DELETE_ARRAY(szDXLInput);
		GPOS_DELETE_ARRAY(szDXLOutput);

		if (GPOS_FAILED == eres)
		{
			return eres;
		}
	}

	return GPOS_OK;
}

// reads a DXL document, generates the statistics object, performs a
// filter operation on it, serializes it into a DXL document and
// compares the generated DXL document with the expected DXL document.
GPOS_RESULT
CFilterCardinalityTest::EresUnittest_CStatisticsFilter()
{
	SStatsFilterSTestCase rgstatstc[] =
	{
		{"../data/dxl/statistics/Select-Statistics-Input-Null-Bucket.xml", "../data/dxl/statistics/Select-Statistics-Output-Null-Bucket.xml", PstatspredNullableCols},
		{"../data/dxl/statistics/Select-Statistics-Input-Null-Bucket.xml", "../data/dxl/statistics/Select-Statistics-Output-Null-Constant.xml", PstatspredWithNullConstant},
		{"../data/dxl/statistics/Select-Statistics-Input-Null-Bucket.xml", "../data/dxl/statistics/Select-Statistics-Output-NotNull-Constant.xml", PstatspredNotNull},
	};

	const ULONG ulTestCases = GPOS_ARRAY_SIZE(rgstatstc);

	return EresUnittest_CStatistics(rgstatstc, ulTestCases);
}

// create a filter on a column with null values
CStatsPred *
CFilterCardinalityTest::PstatspredNullableCols
	(
	IMemoryPool *memory_pool
	)
{
	DrgPstatspred *pdrgpstatspred = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);

	pdrgpstatspred->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptLEq, CTestUtils::PpointInt4(memory_pool, 1)));

	return GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspred);
}

// create a point filter where the constant is null
CStatsPred *
CFilterCardinalityTest::PstatspredWithNullConstant
	(
	IMemoryPool *memory_pool
	)
{
	DrgPstatspred *pdrgpstatspred = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);

	pdrgpstatspred->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptEq,  CTestUtils::PpointInt4NullVal(memory_pool)));

	return GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspred);
}

// create an 'is not null' point filter
CStatsPred *
CFilterCardinalityTest::PstatspredNotNull
	(
	IMemoryPool *memory_pool
	)
{
	DrgPstatspred *pdrgpstatspred = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);

	pdrgpstatspred->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptNEq,  CTestUtils::PpointInt4NullVal(memory_pool)));

	return GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspred);
}

// reads a DXL document, generates the statistics object, performs a
// filter operation on it, serializes it into a DXL document and
// compares the generated DXL document with the expected DXL document.
GPOS_RESULT
CFilterCardinalityTest::EresUnittest_CStatisticsFilterDisj()
{
	SStatsFilterSTestCase rgstatsdisjtc[] =
	{
		{"../data/dxl/statistics/Disj-Input-1.xml", "../data/dxl/statistics/Disj-Output-1.xml", PstatspredDisj1},
		{"../data/dxl/statistics/Disj-Input-1.xml", "../data/dxl/statistics/Disj-Output-1.xml", PstatspredDisj2},
		{"../data/dxl/statistics/Disj-Input-1.xml", "../data/dxl/statistics/Disj-Output-1.xml", PstatspredDisj3},
		{"../data/dxl/statistics/Disj-Input-2.xml", "../data/dxl/statistics/Disj-Output-2-1.xml", PstatspredDisj4},
		{"../data/dxl/statistics/Disj-Input-2.xml", "../data/dxl/statistics/Disj-Output-2-2.xml", PstatspredDisj5},
		{"../data/dxl/statistics/Disj-Input-2.xml", "../data/dxl/statistics/Disj-Output-2-3.xml", PstatspredDisj6},
		{"../data/dxl/statistics/Disj-Input-2.xml", "../data/dxl/statistics/Disj-Output-2-4.xml", PstatspredDisj7},
		{"../data/dxl/statistics/NestedPred-Input-10.xml", "../data/dxl/statistics/Disj-Output-8.xml", PstatspredDisj8},
	};

	const ULONG ulTestCases = GPOS_ARRAY_SIZE(rgstatsdisjtc);

	return EresUnittest_CStatistics(rgstatsdisjtc, ulTestCases);
}

// create an or filter (no duplicate)
CStatsPred *
CFilterCardinalityTest::PstatspredDisj1
	(
	IMemoryPool *memory_pool
	)
{
	// predicate col_1 in (13, 25, 47, 49);
	INT rgiVal[] = {13, 25, 47, 49};
	const ULONG ulVals = GPOS_ARRAY_SIZE(rgiVal);
	DrgPstatspred *pdrgpstatspredDisj = PdrgpstatspredInteger(memory_pool, 1, CStatsPred::EstatscmptEq, rgiVal, ulVals);

	return GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspredDisj);
}

// create an or filter (one duplicate constant)
CStatsPred *
CFilterCardinalityTest::PstatspredDisj2
	(
	IMemoryPool *memory_pool
	)
{

	// predicate col_1 in (13, 13, 25, 47, 49);
	INT rgiVal[] = {13, 13, 25, 47, 49};
	const ULONG ulVals = GPOS_ARRAY_SIZE(rgiVal);
	DrgPstatspred *pdrgpstatspredDisj = PdrgpstatspredInteger(memory_pool, 1, CStatsPred::EstatscmptEq, rgiVal, ulVals);

	return GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspredDisj);
}

//	create an or filter (multiple duplicate constants)
CStatsPred *
CFilterCardinalityTest::PstatspredDisj3
	(
	IMemoryPool *memory_pool
	)
{
	// predicate col_1 in (13, 25, 47, 47, 47, 49, 13);
	INT rgiVal[] = {13, 25, 47, 47, 47, 49, 13};
	const ULONG ulVals = GPOS_ARRAY_SIZE(rgiVal);
	DrgPstatspred *pdrgpstatspredDisj = PdrgpstatspredInteger(memory_pool, 1, CStatsPred::EstatscmptEq, rgiVal, ulVals);

	return GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspredDisj);
}

// create an or filter
CStatsPred *
CFilterCardinalityTest::PstatspredDisj4
	(
	IMemoryPool *memory_pool
	)
{
	// the predicate is (x <= 5 or x <= 10 or x <= 13) (domain [0 -- 20])
	INT rgiVal[] = {5, 10, 13};
	const ULONG ulVals = GPOS_ARRAY_SIZE(rgiVal);
	DrgPstatspred *pdrgpstatspredDisj = PdrgpstatspredInteger(memory_pool, 1, CStatsPred::EstatscmptLEq, rgiVal, ulVals);

	return GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspredDisj);
}

//	create an or filter (multiple LEQ)
CStatsPred *
CFilterCardinalityTest::PstatspredDisj5
	(
	IMemoryPool *memory_pool
	)
{
	// the predicate is (x >= 5 or x >= 13) (domain [0 -- 20])
	INT rgiVal[] = {5, 13};
	const ULONG ulVals = GPOS_ARRAY_SIZE(rgiVal);
	DrgPstatspred *pdrgpstatspredDisj = PdrgpstatspredInteger(memory_pool, 1, CStatsPred::EstatscmptGEq, rgiVal, ulVals);

	return GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspredDisj);
}

//	create an or filter
CStatsPred *
CFilterCardinalityTest::PstatspredDisj6
	(
	IMemoryPool *memory_pool
	)
{
	// the predicate is (x > 10 or x < 5) (domain [0 -- 20])
	DrgPstatspred *pdrgpstatspredDisj = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);

	pdrgpstatspredDisj->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptG, CTestUtils::PpointInt4(memory_pool, 10)));
	pdrgpstatspredDisj->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptL, CTestUtils::PpointInt4(memory_pool, 5)));

	return GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspredDisj);
}

// create an or filter
CStatsPred *
CFilterCardinalityTest::PstatspredDisj7
	(
	IMemoryPool *memory_pool
	)
{
	// the predicate is (x <= 15 or x >= 5 or x > = 10) (domain [0 -- 20])
	INT rgiVal[] = {5, 10};
	const ULONG ulVals = GPOS_ARRAY_SIZE(rgiVal);
	DrgPstatspred *pdrgpstatspredDisj = PdrgpstatspredInteger(memory_pool, 1, CStatsPred::EstatscmptGEq, rgiVal, ulVals);
	pdrgpstatspredDisj->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptLEq, CTestUtils::PpointInt4(memory_pool, 15)));

	return GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspredDisj);
}

// create disjunctive predicate on same columns
CStatsPred *
CFilterCardinalityTest::PstatspredDisj8
	(
	IMemoryPool *memory_pool
	)
{
	// predicate is b = 2001 OR b == 2002
	INT rgiVal[] = {2001, 2002};
	const ULONG ulVals = GPOS_ARRAY_SIZE(rgiVal);
	DrgPstatspred *pdrgpstatspredDisj = PdrgpstatspredInteger(memory_pool, 61, CStatsPred::EstatscmptEq, rgiVal, ulVals);

	return GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspredDisj);
}

// reads a DXL document, generates the statistics object, performs a
// filter operation on it, serializes it into a DXL document and
// compares the generated DXL document with the expected DXL document.
GPOS_RESULT
CFilterCardinalityTest::EresUnittest_CStatisticsFilterConj()
{
	SStatsFilterSTestCase rgstatsdisjtc[] =
	{
			{"../data/dxl/statistics/NestedPred-Input-9.xml", "../data/dxl/statistics/NestedPred-Output-9.xml", PstatspredConj},
	};

	const ULONG ulTestCases = GPOS_ARRAY_SIZE(rgstatsdisjtc);

	return EresUnittest_CStatistics(rgstatsdisjtc, ulTestCases);
}

// create conjunctive predicate
CStatsPred *
CFilterCardinalityTest::PstatspredConj
	(
	IMemoryPool *memory_pool
	)
{
	CWStringDynamic *pstrW = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool, GPOS_WSZ_LIT("AAAABXc="));

	// predicate is a == 'w' AND b == 2001 AND c > 0
	DrgPstatspred *pdrgpstatspredConj3 = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);
	pdrgpstatspredConj3->Append(GPOS_NEW(memory_pool) CStatsPredPoint(594, CStatsPred::EstatscmptEq, CCardinalityTestUtils::PpointGeneric(memory_pool, GPDB_TEXT, pstrW, 160621100)));
	pdrgpstatspredConj3->Append(GPOS_NEW(memory_pool) CStatsPredPoint(592, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 2001)));
	pdrgpstatspredConj3->Append(GPOS_NEW(memory_pool) CStatsPredPoint(593, CStatsPred::EstatscmptG, CTestUtils::PpointInt4(memory_pool, 0)));

	GPOS_DELETE(pstrW);

	return GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspredConj3);
}

// reads a DXL document, generates the statistics object, performs a
// filter operation on it, serializes it into a DXL document and
// compares the generated DXL document with the expected DXL document.
GPOS_RESULT
CFilterCardinalityTest::EresUnittest_CStatisticsNestedPred()
{
	SStatsFilterSTestCase rgstatsdisjtc[] =
	{
	    {"../data/dxl/statistics/NestedPred-Input-1.xml", "../data/dxl/statistics/NestedPred-Output-1.xml", PstatspredNestedPredDiffCol1},
		{"../data/dxl/statistics/NestedPred-Input-1.xml", "../data/dxl/statistics/NestedPred-Output-1.xml", PstatspredNestedPredDiffCol2},
		{"../data/dxl/statistics/NestedPred-Input-2.xml", "../data/dxl/statistics/NestedPred-Output-2.xml", PstatspredNestedPredCommonCol1},
		{"../data/dxl/statistics/NestedPred-Input-1.xml", "../data/dxl/statistics/NestedPred-Output-3.xml", PstatspredNestedSharedCol},
		{"../data/dxl/statistics/NestedPred-Input-3.xml", "../data/dxl/statistics/NestedPred-Output-4.xml", PstatspredDisjOverConjSameCol1},
		{"../data/dxl/statistics/NestedPred-Input-3.xml", "../data/dxl/statistics/NestedPred-Input-3.xml", PstatspredDisjOverConjSameCol2},
		{"../data/dxl/statistics/NestedPred-Input-1.xml", "../data/dxl/statistics/NestedPred-Output-5.xml", PstatspredDisjOverConjDifferentCol1},
		{"../data/dxl/statistics/NestedPred-Input-1.xml", "../data/dxl/statistics/NestedPred-Output-6.xml", PstatspredDisjOverConjMultipleIdenticalCols},
		{"../data/dxl/statistics/NestedPred-Input-2.xml", "../data/dxl/statistics/NestedPred-Output-7.xml", PstatspredNestedPredCommonCol2},
		{"../data/dxl/statistics/NestedPred-Input-8.xml", "../data/dxl/statistics/NestedPred-Output-8.xml", PstatspredDisjOverConjSameCol3},
		{"../data/dxl/statistics/NestedPred-Input-10.xml", "../data/dxl/statistics/NestedPred-Output-10.xml", PstatspredDisjOverConjSameCol4},
	};

	const ULONG ulTestCases = GPOS_ARRAY_SIZE(rgstatsdisjtc);

	return EresUnittest_CStatistics(rgstatsdisjtc, ulTestCases);
}

//		Create nested AND and OR predicates where the AND and OR predicates
//		are on different columns
CStatsPred *
CFilterCardinalityTest::PstatspredNestedPredDiffCol1
	(
	IMemoryPool *memory_pool
	)
{
	DrgPstatspred *pdrgpstatspredConj = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);

	// predicate col_1 <> 3
	pdrgpstatspredConj->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptNEq, CTestUtils::PpointInt4(memory_pool, 3)));

	// predicate col_2 in (15, 20, 22, 24, 31, 39, 42, 46);
	INT rgiVal[] = {15, 20, 22, 24, 31, 39, 42, 46};
	const ULONG ulVals = GPOS_ARRAY_SIZE(rgiVal);
	DrgPstatspred *pdrgpstatspredDisj = PdrgpstatspredInteger(memory_pool, 2, CStatsPred::EstatscmptEq, rgiVal, ulVals);

	CStatsPredDisj *pstatspredDisj = GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspredDisj);
	pdrgpstatspredConj->Append(pstatspredDisj);

	return GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspredConj);
}

// create nested AND and OR predicates where the AND and OR predicates
// are on different columns. note: the order of the predicates in
// reversed as in PstatspredNestedPredDiffCol1
CStatsPred *
CFilterCardinalityTest::PstatspredNestedPredDiffCol2
	(
	IMemoryPool *memory_pool
	)
{
	DrgPstatspred *pdrgpstatspredConj = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);

	// predicate col_2 in (15, 20, 22, 24, 31, 39, 42, 46);
	INT rgiVal[] = {15, 20, 22, 24, 31, 39, 42, 46};
	const ULONG ulVals = GPOS_ARRAY_SIZE(rgiVal);
	DrgPstatspred *pdrgpstatspredDisj = PdrgpstatspredInteger(memory_pool, 2, CStatsPred::EstatscmptEq, rgiVal, ulVals);

	CStatsPredDisj *pstatspredDisj = GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspredDisj);
	pdrgpstatspredConj->Append(pstatspredDisj);

	// predicate col_1 <> 3
	pdrgpstatspredConj->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptNEq, CTestUtils::PpointInt4(memory_pool, 3)));

	return GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspredConj);
}

// create nested AND and OR predicates where the AND and OR predicates
// are on the same columns
CStatsPred *
CFilterCardinalityTest::PstatspredNestedPredCommonCol1
	(
	IMemoryPool *memory_pool
	)
{
	// predicate is col_2 in (39, 31, 24, 22, 46, 20, 42, 15) AND col_2 == 2
	DrgPstatspred *pdrgpstatspredConj = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);

	// predicate col_2 in (15, 20, 22, 24, 31, 39, 42, 46);
	INT rgiVal[] = {15, 20, 22, 24, 31, 39, 42, 46};
	const ULONG ulVals = GPOS_ARRAY_SIZE(rgiVal);
	DrgPstatspred *pdrgpstatspredDisj = PdrgpstatspredInteger(memory_pool, 2, CStatsPred::EstatscmptEq, rgiVal, ulVals);

	CStatsPredDisj *pstatspredDisj = GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspredDisj);
	pdrgpstatspredConj->Append(pstatspredDisj);

	// predicate col_2 == 2
	pdrgpstatspredConj->Append(GPOS_NEW(memory_pool) CStatsPredPoint(2, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 2)));

	return GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspredConj);
}

// create nested AND and OR predicates where the AND and OR predicates
// are on the same columns
CStatsPred *
CFilterCardinalityTest::PstatspredNestedPredCommonCol2
	(
	IMemoryPool *memory_pool
	)
{
	// predicate is col_2 in (2, 39, 31, 24, 22, 46, 20, 42, 15) AND col_2 == 2
	DrgPstatspred *pdrgpstatspredConj = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);

	// IN predicate: col_2 in (2, 39, 31, 24, 22, 46, 20, 42, 15);
	INT rgiVal[] = {2, 15, 20, 22, 24, 31, 39, 42, 46};
	const ULONG ulVals = GPOS_ARRAY_SIZE(rgiVal);
	DrgPstatspred *pdrgpstatspredDisj = PdrgpstatspredInteger(memory_pool, 2, CStatsPred::EstatscmptEq, rgiVal, ulVals);

	CStatsPredDisj *pstatspredDisj = GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspredDisj);
	pdrgpstatspredConj->Append(pstatspredDisj);

	// predicate col_2 == 2
	pdrgpstatspredConj->Append(GPOS_NEW(memory_pool) CStatsPredPoint(2, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 2)));

	return GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspredConj);
}

// create nested AND and OR predicates where the AND and OR predicates
// share common columns
CStatsPred *
CFilterCardinalityTest::PstatspredNestedSharedCol
	(
	IMemoryPool *memory_pool
	)
{
	DrgPstatspred *pdrgpstatspredConj = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);

	// predicate col_1 <> 3
	pdrgpstatspredConj->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptNEq, CTestUtils::PpointInt4(memory_pool, 3)));

	// predicate col_2 in (15, 20, 22, 24, 31, 39, 42, 46) OR (col_1 == 4));

	INT rgiVal[] = {15, 20, 22, 24, 31, 39, 42, 46};
	const ULONG ulVals = GPOS_ARRAY_SIZE(rgiVal);
	DrgPstatspred *pdrgpstatspredDisj = PdrgpstatspredInteger(memory_pool, 2, CStatsPred::EstatscmptEq, rgiVal, ulVals);

	pdrgpstatspredDisj->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 4)));

	CStatsPredDisj *pstatspredDisj = GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspredDisj);
	pdrgpstatspredConj->Append(pstatspredDisj);

	return GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspredConj);
}

// create nested AND and OR predicates where the AND and OR predicates share common columns
CStatsPred *
CFilterCardinalityTest::PstatspredDisjOverConjSameCol1
	(
	IMemoryPool *memory_pool
	)
{
	DrgPstatspred *pdrgpstatspredConj = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);

	// predicate col_1 = 3 AND col_1 >=3
	pdrgpstatspredConj->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 3)));
	pdrgpstatspredConj->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptGEq, CTestUtils::PpointInt4(memory_pool, 3)));

	CStatsPredConj *pstatspredConj = GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspredConj);

	// predicate (col_1 = 1);
	DrgPstatspred *pdrgpstatspredDisj = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);
	pdrgpstatspredDisj->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 1)));
	pdrgpstatspredDisj->Append(pstatspredConj);

	return GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspredDisj);
}

// create nested AND and OR predicates where the AND and OR predicates share common columns
CStatsPred *
CFilterCardinalityTest::PstatspredDisjOverConjSameCol2
	(
	IMemoryPool *memory_pool
	)
{
	DrgPstatspred *pdrgpstatspredConj = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);

	// predicate col_1 <= 5 AND col_1 >=1
	pdrgpstatspredConj->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptLEq, CTestUtils::PpointInt4(memory_pool, 5)));
	pdrgpstatspredConj->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptGEq, CTestUtils::PpointInt4(memory_pool, 1)));

	CStatsPredConj *pstatspredConj = GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspredConj);

	// predicate (col_1 = 1);
	DrgPstatspred *pdrgpstatspredDisj = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);
	pdrgpstatspredDisj->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 1)));
	pdrgpstatspredDisj->Append(pstatspredConj);

	return GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspredDisj);
}

// create disjunctive predicate over conjunctions on same columns
CStatsPred *
CFilterCardinalityTest::PstatspredDisjOverConjSameCol3
	(
	IMemoryPool *memory_pool
	)
{
	DrgPstatspred *pdrgpstatspredDisj = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);

	CWStringDynamic *pstrS = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool, GPOS_WSZ_LIT("AAAABXM="));
	CWStringDynamic *pstrW = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool, GPOS_WSZ_LIT("AAAABXc="));

	// predicate is a == 's' AND b == 2001
	DrgPstatspred *pdrgpstatspredConj1 = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);
	pdrgpstatspredConj1->Append(GPOS_NEW(memory_pool) CStatsPredPoint(142, CStatsPred::EstatscmptEq, CCardinalityTestUtils::PpointGeneric(memory_pool, GPDB_TEXT, pstrS, 160588332)));
	pdrgpstatspredConj1->Append(GPOS_NEW(memory_pool) CStatsPredPoint(113, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 2001)));
	pdrgpstatspredDisj->Append(GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspredConj1));

	// predicate is a == 's' AND b == 2002
	DrgPstatspred *pdrgpstatspredConj2 = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);
	pdrgpstatspredConj2->Append(GPOS_NEW(memory_pool) CStatsPredPoint(142, CStatsPred::EstatscmptEq, CCardinalityTestUtils::PpointGeneric(memory_pool, GPDB_TEXT, pstrS, 160588332)));
	pdrgpstatspredConj2->Append(GPOS_NEW(memory_pool) CStatsPredPoint(113, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 2002)));
	pdrgpstatspredDisj->Append(GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspredConj2));

	// predicate is a == 'w' AND b == 2001
	DrgPstatspred *pdrgpstatspredConj3 = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);
	pdrgpstatspredConj3->Append(GPOS_NEW(memory_pool) CStatsPredPoint(142, CStatsPred::EstatscmptEq, CCardinalityTestUtils::PpointGeneric(memory_pool, GPDB_TEXT, pstrW, 160621100)));
	pdrgpstatspredConj3->Append(GPOS_NEW(memory_pool) CStatsPredPoint(113, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 2001)));
	pdrgpstatspredDisj->Append(GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspredConj3));

	// predicate is a == 'w' AND b == 2002
	DrgPstatspred *pdrgpstatspredConj4 = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);
	pdrgpstatspredConj4->Append(GPOS_NEW(memory_pool) CStatsPredPoint(142, CStatsPred::EstatscmptEq, CCardinalityTestUtils::PpointGeneric(memory_pool, GPDB_TEXT, pstrW, 160621100)));
	pdrgpstatspredConj4->Append(GPOS_NEW(memory_pool) CStatsPredPoint(113, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 2002)));
	pdrgpstatspredDisj->Append(GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspredConj4));

	GPOS_DELETE(pstrS);
	GPOS_DELETE(pstrW);

	return GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspredDisj);
}

// create disjunctive predicate over conjunctions on same columns
CStatsPred *
CFilterCardinalityTest::PstatspredDisjOverConjSameCol4
	(
	IMemoryPool *memory_pool
	)
{
	DrgPstatspred *pdrgpstatspredDisj = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);

	CWStringDynamic *pstrS = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool, GPOS_WSZ_LIT("AAAABXM="));
	CWStringDynamic *pstrW = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool, GPOS_WSZ_LIT("AAAABXc="));

	// predicate is a == 's' AND b == 2001 AND c > 0
	DrgPstatspred *pdrgpstatspredConj1 = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);
	pdrgpstatspredConj1->Append(GPOS_NEW(memory_pool) CStatsPredPoint(91, CStatsPred::EstatscmptEq, CCardinalityTestUtils::PpointGeneric(memory_pool, GPDB_TEXT, pstrS, 160588332)));
	pdrgpstatspredConj1->Append(GPOS_NEW(memory_pool) CStatsPredPoint(61, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 2001)));
	pdrgpstatspredConj1->Append(GPOS_NEW(memory_pool) CStatsPredPoint(90, CStatsPred::EstatscmptG, CTestUtils::PpointInt4(memory_pool, 0)));
	pdrgpstatspredDisj->Append(GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspredConj1));

	// predicate is a == 's' AND b == 2002
	DrgPstatspred *pdrgpstatspredConj2 = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);
	pdrgpstatspredConj2->Append(GPOS_NEW(memory_pool) CStatsPredPoint(91, CStatsPred::EstatscmptEq, CCardinalityTestUtils::PpointGeneric(memory_pool, GPDB_TEXT, pstrS, 160588332)));
	pdrgpstatspredConj2->Append(GPOS_NEW(memory_pool) CStatsPredPoint(61, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 2002)));
	pdrgpstatspredDisj->Append(GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspredConj2));

	// predicate is a == 'w' AND b == 2001 AND c > 0
	DrgPstatspred *pdrgpstatspredConj3 = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);
	pdrgpstatspredConj3->Append(GPOS_NEW(memory_pool) CStatsPredPoint(91, CStatsPred::EstatscmptEq, CCardinalityTestUtils::PpointGeneric(memory_pool, GPDB_TEXT, pstrW, 160621100)));
	pdrgpstatspredConj3->Append(GPOS_NEW(memory_pool) CStatsPredPoint(61, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 2001)));
	pdrgpstatspredConj1->Append(GPOS_NEW(memory_pool) CStatsPredPoint(90, CStatsPred::EstatscmptG, CTestUtils::PpointInt4(memory_pool, 0)));
	pdrgpstatspredDisj->Append(GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspredConj3));

	// predicate is a == 'w' AND b == 2002
	DrgPstatspred *pdrgpstatspredConj4 = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);
	pdrgpstatspredConj4->Append(GPOS_NEW(memory_pool) CStatsPredPoint(91, CStatsPred::EstatscmptEq, CCardinalityTestUtils::PpointGeneric(memory_pool, GPDB_TEXT, pstrW, 160621100)));
	pdrgpstatspredConj4->Append(GPOS_NEW(memory_pool) CStatsPredPoint(61, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 2002)));
	pdrgpstatspredDisj->Append(GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspredConj4));

	GPOS_DELETE(pstrS);
	GPOS_DELETE(pstrW);

	return GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspredDisj);
}

// create nested AND and OR predicates where the AND and OR predicates share common columns
CStatsPred *
CFilterCardinalityTest::PstatspredDisjOverConjDifferentCol1
	(
	IMemoryPool *memory_pool
	)
{
	DrgPstatspred *pdrgpstatspredConj = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);

	// predicate col_1 = 3 AND col_2 >=3
	pdrgpstatspredConj->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 3)));
	pdrgpstatspredConj->Append(GPOS_NEW(memory_pool) CStatsPredPoint(2, CStatsPred::EstatscmptGEq, CTestUtils::PpointInt4(memory_pool, 3)));

	CStatsPredConj *pstatspredConj = GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspredConj);

	// predicate (col_1 = 1);
	DrgPstatspred *pdrgpstatspredDisj = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);
	pdrgpstatspredDisj->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 1)));
	pdrgpstatspredDisj->Append(pstatspredConj);

	return GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspredDisj);
}

// create nested AND and OR predicates where the AND and OR predicates
CStatsPred *
CFilterCardinalityTest::PstatspredDisjOverConjMultipleIdenticalCols
	(
	IMemoryPool *memory_pool
	)
{
	DrgPstatspred *pdrgpstatspredConj1 = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);

	// predicate col_1 = 1 AND col_2 = 1
	pdrgpstatspredConj1->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 1)));
	pdrgpstatspredConj1->Append(GPOS_NEW(memory_pool) CStatsPredPoint(2, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 1)));

	CStatsPredConj *pstatspredConj1 = GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspredConj1);

	DrgPstatspred *pdrgpstatspredConj2 = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);

	// predicate col_1 = 2 AND col_2 = 2
	pdrgpstatspredConj2->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 2)));
	pdrgpstatspredConj2->Append(GPOS_NEW(memory_pool) CStatsPredPoint(2, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 2)));

	CStatsPredConj *pstatspredConj2 = GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspredConj2);
	DrgPstatspred *pdrgpstatspredDisj = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);

	pdrgpstatspredDisj->Append(pstatspredConj1);
	pdrgpstatspredDisj->Append(pstatspredConj2);

	return GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspredDisj);
}

// reads a DXL document, generates the statistics object, performs a
// filter operation on it, serializes it into a DXL document and
// compares the generated DXL document with the expected DXL document.
GPOS_RESULT
CFilterCardinalityTest::EresUnittest_CStatisticsBasicsFromDXLNumeric()
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	SStatsCmpValElem rgStatsCmpValElem[] =
	{
		{CStatsPred::EstatscmptL,    GPOS_WSZ_LIT("AAAACgAAAgABAA=="), CDouble(1.0)},
		{CStatsPred::EstatscmptLEq,  GPOS_WSZ_LIT("AAAACgAAAgABAA=="), CDouble(1.0)},
		{CStatsPred::EstatscmptEq,   GPOS_WSZ_LIT("AAAACgAAAgABAA=="), CDouble(1.0)},

		{CStatsPred::EstatscmptG,    GPOS_WSZ_LIT("AAAACgAAAgAyAA=="), CDouble(50.0)},
		{CStatsPred::EstatscmptGEq,  GPOS_WSZ_LIT("AAAACgAAAgAyAA=="), CDouble(50.0)},
		{CStatsPred::EstatscmptEq,   GPOS_WSZ_LIT("AAAACgAAAgAyAA=="), CDouble(50.0)},

		{CStatsPred::EstatscmptL,    GPOS_WSZ_LIT("AAAACgAAAgABAA=="), CDouble(1.0)},
		{CStatsPred::EstatscmptLEq,  GPOS_WSZ_LIT("AAAACgAAAgABAA=="), CDouble(1.0)},
		{CStatsPred::EstatscmptEq,   GPOS_WSZ_LIT("AAAACgAAAgABAA=="), CDouble(1.0)},
		{CStatsPred::EstatscmptG,    GPOS_WSZ_LIT("AAAACgAAAgABAA=="), CDouble(1.0)},
		{CStatsPred::EstatscmptGEq,  GPOS_WSZ_LIT("AAAACgAAAgABAA=="), CDouble(1.0)},

		{CStatsPred::EstatscmptL,    GPOS_WSZ_LIT("AAAACgAAAgAyAA=="), CDouble(50.0)},
		{CStatsPred::EstatscmptLEq,  GPOS_WSZ_LIT("AAAACgAAAgAyAA=="), CDouble(50.0)},
		{CStatsPred::EstatscmptG,    GPOS_WSZ_LIT("AAAACgAAAgAyAA=="), CDouble(50.0)},
		{CStatsPred::EstatscmptGEq,  GPOS_WSZ_LIT("AAAACgAAAgAyAA=="), CDouble(50.0)},
		{CStatsPred::EstatscmptEq,   GPOS_WSZ_LIT("AAAACgAAAgAyAA=="), CDouble(50.0)},
	};

	const ULONG ulLen = GPOS_ARRAY_SIZE(rgStatsCmpValElem);
	GPOS_ASSERT(ulLen == GPOS_ARRAY_SIZE(rgtcStatistics));
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		// read input DXL file
		CHAR *szDXLInput = CDXLUtils::Read(memory_pool, rgtcStatistics[ul].szInputFile);
		// read output DXL file
		CHAR *szDXLOutput = CDXLUtils::Read(memory_pool, rgtcStatistics[ul].szOutputFile);

		GPOS_CHECK_ABORT;

		CMDAccessor *pmda = COptCtxt::PoctxtFromTLS()->Pmda();
		// parse the statistics objects
		DrgPdxlstatsderrel *dxl_derived_rel_stats_array = CDXLUtils::ParseDXLToStatsDerivedRelArray(memory_pool, szDXLInput, NULL);
		CStatisticsArray *pdrgpstatBefore = CDXLUtils::ParseDXLToOptimizerStatisticObjArray(memory_pool, pmda, dxl_derived_rel_stats_array);
		dxl_derived_rel_stats_array->Release();

		GPOS_ASSERT(NULL != pdrgpstatBefore);

		GPOS_CHECK_ABORT;

		SStatsCmpValElem statsCmpValElem = rgStatsCmpValElem[ul];

		DrgPstatspred *pdrgpstatspred = PdrgppredfilterNumeric(memory_pool, 1 /*col_id*/, statsCmpValElem);
		CStatsPredConj *pstatspred = GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspred);
		GPOS_RESULT eres = EresUnittest_CStatisticsCompare
							(
							memory_pool,
							pmda,
							pdrgpstatBefore,
							pstatspred,
							szDXLOutput,
							true /*fApplyTwice*/
							);

		// clean up
		pdrgpstatBefore->Release();
		pstatspred->Release();
		GPOS_DELETE_ARRAY(szDXLInput);
		GPOS_DELETE_ARRAY(szDXLOutput);

		if (GPOS_OK != eres)
		{
			return eres;
		}
	}

	return GPOS_OK;
}

// generate an array of filter given a column identifier, comparison type,
// and array of integer point
DrgPstatspred *
CFilterCardinalityTest::PdrgpstatspredInteger
	(
	IMemoryPool *memory_pool,
	ULONG col_id,
	CStatsPred::EStatsCmpType escmpt,
	INT *piVals,
	ULONG ulVals
	)
{
	GPOS_ASSERT(0 < ulVals);

	DrgPstatspred *pdrgpstatspred = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);
	for (ULONG ul = 0; ul < ulVals; ul++)
	{
		pdrgpstatspred->Append(GPOS_NEW(memory_pool) CStatsPredPoint(col_id, escmpt, CTestUtils::PpointInt4(memory_pool, piVals[ul])));
	}

	return pdrgpstatspred;
}

// generate a numeric filter on the column specified and the literal value
DrgPstatspred *
CFilterCardinalityTest::PdrgppredfilterNumeric
	(
	IMemoryPool *memory_pool,
	ULONG col_id,
	SStatsCmpValElem statsCmpValElem
	)
{
	// create a filter
	DrgPstatspred *pdrgpstatspred = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);
	CWStringDynamic *pstrNumeric = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool, statsCmpValElem.m_wsz);
	CStatsPredPoint *pstatspred = GPOS_NEW(memory_pool) CStatsPredPoint
													(
													col_id,
													statsCmpValElem.m_escmpt,
													CCardinalityTestUtils::PpointNumeric(memory_pool, pstrNumeric, statsCmpValElem.m_value)
													);
	pdrgpstatspred->Append(pstatspred);
	GPOS_DELETE(pstrNumeric);

	return pdrgpstatspred;
}

// reads a DXL document, generates the statistics object, performs a
// filter operation on it, serializes it into a DXL document and
// compares the generated DXL document with the expected DXL document.
GPOS_RESULT
CFilterCardinalityTest::EresUnittest_CStatisticsBasicsFromDXL()
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	CWStringDynamic str(memory_pool);
	COstreamString oss(&str);

	// read input DXL file
	CHAR *szDXLInput = CDXLUtils::Read(memory_pool, szInputDXLFileName);
	// read output DXL file
	CHAR *szDXLOutput = CDXLUtils::Read(memory_pool, szOutputDXLFileName);

	GPOS_CHECK_ABORT;

	CMDAccessor *pmda = COptCtxt::PoctxtFromTLS()->Pmda();

	// parse the statistics objects
	DrgPdxlstatsderrel *dxl_derived_rel_stats_array = CDXLUtils::ParseDXLToStatsDerivedRelArray(memory_pool, szDXLInput, NULL);
	CStatisticsArray *pdrgpstatsBefore = CDXLUtils::ParseDXLToOptimizerStatisticObjArray
									(
									memory_pool,
									pmda,
									dxl_derived_rel_stats_array
									);
	dxl_derived_rel_stats_array->Release();
	GPOS_ASSERT(NULL != pdrgpstatsBefore);

	GPOS_CHECK_ABORT;

	// create a filter
	CStatsPredConj *pstatspred = GPOS_NEW(memory_pool) CStatsPredConj(CStatisticsTest::Pdrgpstatspred2(memory_pool));
	GPOS_RESULT eres = EresUnittest_CStatisticsCompare
							(
							memory_pool,
							pmda,
							pdrgpstatsBefore,
							pstatspred,
							szDXLOutput
							);

	// clean up
	pdrgpstatsBefore->Release();
	pstatspred->Release();
	GPOS_DELETE_ARRAY(szDXLInput);
	GPOS_DELETE_ARRAY(szDXLOutput);

	return eres;
}

// performs a filter operation on it, serializes it into a DXL document
// and compares the generated DXL document with the expected DXL document
GPOS_RESULT
CFilterCardinalityTest::EresUnittest_CStatisticsCompare
	(
	IMemoryPool *memory_pool,
	CMDAccessor *pmda,
	CStatisticsArray *pdrgpstatBefore,
	CStatsPred *pstatspred,
	const CHAR *szDXLOutput,
	BOOL fApplyTwice
	)
{
	CWStringDynamic str(memory_pool);
	COstreamString oss(&str);

	CStatistics *pstatsInput = (* pdrgpstatBefore)[0];

	GPOS_TRACE(GPOS_WSZ_LIT("Statistics before"));
	CCardinalityTestUtils::PrintStats(memory_pool, pstatsInput);

	CStatistics *pstatsOutput = CFilterStatsProcessor::PstatsFilter(memory_pool, pstatsInput, pstatspred, true /* fCapNdvs */);

	GPOS_TRACE(GPOS_WSZ_LIT("Statistics after"));
	CCardinalityTestUtils::PrintStats(memory_pool, pstatsOutput);

	// output array of stats objects
	CStatisticsArray *pdrgpstatOutput = GPOS_NEW(memory_pool) CStatisticsArray(memory_pool);
	pdrgpstatOutput->Append(pstatsOutput);

	oss << "Serializing Input Statistics Objects (Before Filter)" << std::endl;
	CWStringDynamic *pstrInput = CDXLUtils::SerializeStatistics
												(
												memory_pool,
												pmda,
												pdrgpstatBefore,
												true /*serialize_header_footer*/,
												true /*indentation*/
												);
	GPOS_TRACE(pstrInput->GetBuffer());
	GPOS_DELETE(pstrInput);

	oss << "Serializing Output Statistics Objects (After Filter)" << std::endl;
	CWStringDynamic *pstrOutput = CDXLUtils::SerializeStatistics
												(
												memory_pool,
												pmda,
												pdrgpstatOutput,
												true /*serialize_header_footer*/,
												true /*indentation*/
												);
	GPOS_TRACE(pstrOutput->GetBuffer());

	CWStringDynamic dstrExpected(memory_pool);
	dstrExpected.AppendFormat(GPOS_WSZ_LIT("%s"), szDXLOutput);

	GPOS_RESULT eres = CTestUtils::EresCompare
							(
							oss,
							pstrOutput,
							&dstrExpected,
							false /* ignore mismatch */
							);

	if (fApplyTwice && GPOS_OK == eres)
	{
		CStatistics *pstatsOutput2 = CFilterStatsProcessor::PstatsFilter(memory_pool, pstatsOutput, pstatspred, true /* fCapNdvs */);
		pstatsOutput2->DRows();
		GPOS_TRACE(GPOS_WSZ_LIT("Statistics after another filter"));
		CCardinalityTestUtils::PrintStats(memory_pool, pstatsOutput2);

		// output array of stats objects
		CStatisticsArray *pdrgpstatOutput2 = GPOS_NEW(memory_pool) CStatisticsArray(memory_pool);
		pdrgpstatOutput2->Append(pstatsOutput2);

		CWStringDynamic *pstrOutput2 = CDXLUtils::SerializeStatistics
													(
													memory_pool,
													pmda,
													pdrgpstatOutput2,
													true /*serialize_header_footer*/,
													true /*indentation*/
													);
		eres = CTestUtils::EresCompare
					(
					oss,
					pstrOutput2,
					&dstrExpected,
					false /* ignore mismatch */
					);

		pdrgpstatOutput2->Release();
		GPOS_DELETE(pstrOutput2);
	}

	pdrgpstatOutput->Release();
	GPOS_DELETE(pstrOutput);

	return eres;
}

// test for accumulating cardinality in disjunctive and conjunctive predicates
GPOS_RESULT
CFilterCardinalityTest::EresUnittest_CStatisticsAccumulateCard()
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	// create hash map from colid -> histogram
	HMUlHist *phmulhist = GPOS_NEW(memory_pool) HMUlHist(memory_pool);

	// array capturing columns for which width information is available
	HMUlDouble *phmuldoubleWidth = GPOS_NEW(memory_pool) HMUlDouble(memory_pool);

	const ULONG ulCols = 3;
	for (ULONG ul = 0; ul < ulCols; ul ++)
	{
		// generate histogram of the form [0, 10), [10, 20), [20, 30), [80, 90), [100,100]
		phmulhist->Insert(GPOS_NEW(memory_pool) ULONG(ul), CCardinalityTestUtils::PhistExampleInt4(memory_pool));

		// width for int
		phmuldoubleWidth->Insert(GPOS_NEW(memory_pool) ULONG(ul), GPOS_NEW(memory_pool) CDouble(4.0));
	}

	CStatistics *pstats = GPOS_NEW(memory_pool) CStatistics
									(
									memory_pool,
									phmulhist,
									phmuldoubleWidth,
									CDouble(1000.0) /* dRows */,
									false /* fEmpty() */
									);
	GPOS_TRACE(GPOS_WSZ_LIT("\nOriginal Stats:\n"));
	CCardinalityTestUtils::PrintStats(memory_pool, pstats);

	// (1)
	// create disjunctive filter
	DrgPstatspred *pdrgpstatspred = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);
	pdrgpstatspred->Append(GPOS_NEW(memory_pool) CStatsPredPoint(0, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 5)));
	pdrgpstatspred->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 200)));
	pdrgpstatspred->Append(GPOS_NEW(memory_pool) CStatsPredPoint(2, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 200)));
	CStatsPredDisj *pstatspredDisj = GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspred);

	// apply filter and print resulting stats
	CStatistics *pstats1 = CFilterStatsProcessor::PstatsFilter(memory_pool, pstats, pstatspredDisj, true /* fCapNdvs */);
	CDouble dRows1 = pstats1->DRows();
	GPOS_TRACE(GPOS_WSZ_LIT("\n\nStats after disjunctive filter [Col0=5 OR Col1=200 OR Col2=200]:\n"));
	CCardinalityTestUtils::PrintStats(memory_pool, pstats1);

	pstatspredDisj->Release();

	// (2)
	// create point filter
	DrgPstatspred *pdrgpstatspred1 = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);
	pdrgpstatspred1->Append(GPOS_NEW(memory_pool) CStatsPredPoint(0, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 5)));
	CStatsPredConj *pstatspredConj1 = GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspred1);

	// apply filter and print resulting stats
	CStatistics *pstats2 = CFilterStatsProcessor::PstatsFilter(memory_pool, pstats, pstatspredConj1, true /* fCapNdvs */);
	CDouble dRows2 = pstats2->DRows();
	GPOS_TRACE(GPOS_WSZ_LIT("\n\nStats after point filter [Col0=5]:\n"));
	CCardinalityTestUtils::PrintStats(memory_pool, pstats2);

	pstatspredConj1->Release();

	GPOS_RTL_ASSERT(dRows1 - dRows2 < 10 && "Disjunctive filter and point filter have very different row estimates");

	// (3)
	// create conjunctive filter
	DrgPstatspred *pdrgpstatspred2 = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);
	pdrgpstatspred2->Append(GPOS_NEW(memory_pool) CStatsPredPoint(0, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 5)));
	pdrgpstatspred2->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 200)));
	pdrgpstatspred2->Append(GPOS_NEW(memory_pool) CStatsPredPoint(2, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 200)));

	CStatsPredConj *pstatspredConj2 = GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspred2);

	// apply filter and print resulting stats
	CStatistics *pstats3 = CFilterStatsProcessor::PstatsFilter(memory_pool, pstats, pstatspredConj2, true /* fCapNdvs */);
	CDouble dRows3 = pstats3->DRows();
	GPOS_TRACE(GPOS_WSZ_LIT("\n\nStats after conjunctive filter [Col0=5 AND Col1=200 AND Col2=200]:\n"));
	CCardinalityTestUtils::PrintStats(memory_pool, pstats3);

	pstatspredConj2->Release();
	GPOS_RTL_ASSERT(dRows3 < dRows2  && "Conjunctive filter passes more rows than than point filter");

	// (4)
	// create selective disjunctive filter that pass no rows
	DrgPstatspred *pdrgpstatspred3 = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);
	pdrgpstatspred3->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 200)));
	pdrgpstatspred3->Append(GPOS_NEW(memory_pool) CStatsPredPoint(2, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 200)));
	CStatsPredDisj *pstatspredDisj1 = GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspred3);

	// apply filter and print resulting stats
	CStatistics *pstats4 = CFilterStatsProcessor::PstatsFilter(memory_pool, pstats, pstatspredDisj1, true /* fCapNdvs */);
	CDouble dRows4 = pstats4->DRows();
	GPOS_TRACE(GPOS_WSZ_LIT("\n\nStats after disjunctive filter [Col1=200 OR Col2=200]:\n"));
	CCardinalityTestUtils::PrintStats(memory_pool, pstats4);

	pstatspredDisj1->Release();

	GPOS_RTL_ASSERT(dRows4 < dRows2  && "Selective disjunctive filter passes more rows than than point filter");

	// (5)
	// create selective conjunctive filter that pass no rows
	DrgPstatspred *pdrgpstatspred4 = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);
	pdrgpstatspred4->Append(GPOS_NEW(memory_pool) CStatsPredPoint(0, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 5)));
	pdrgpstatspred4->Append(GPOS_NEW(memory_pool) CStatsPredPoint(1, CStatsPred::EstatscmptEq, CTestUtils::PpointInt4(memory_pool, 200)));
	CStatsPredConj *pstatspredConj3 = GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspred4);

	// apply filter and print resulting stats
	CStatistics *pstats5 = CFilterStatsProcessor::PstatsFilter(memory_pool, pstats, pstatspredConj3, true /* fCapNdvs */);
	CDouble dRows5 = pstats5->DRows();
	GPOS_TRACE(GPOS_WSZ_LIT("\n\nStats after conjunctive filter [Col0=5 AND Col1=200]:\n"));
	CCardinalityTestUtils::PrintStats(memory_pool, pstats5);

	pstatspredConj3->Release();

	GPOS_RTL_ASSERT(dRows5 < dRows2  && "Selective conjunctive filter passes more rows than than point filter");

	// clean up
	pstats->Release();
	pstats1->Release();
	pstats2->Release();
	pstats3->Release();
	pstats4->Release();
	pstats5->Release();

	return GPOS_OK;
}

// EOF
