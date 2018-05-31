//---------------------------------------------------------------------------
//	Pivotal Software, Inc
//	Copyright (C) 2017 Pivotal Software, Inc
//---------------------------------------------------------------------------
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CColumnFactory.h"
#include "gpopt/mdcache/CMDCache.h"
#include "gpopt/base/CQueryContext.h"
#include "gpopt/eval/CConstExprEvaluatorDefault.h"

#include "unittest/base.h"
#include "unittest/gpopt/CTestUtils.h"
#include "unittest/gpopt/base/CEquivalenceClassesTest.h"
#include "unittest/gpopt/translate/CTranslatorExprToDXLTest.h"

#include "naucrates/md/IMDTypeInt4.h"
#include "naucrates/md/CMDProviderMemory.h"


// Unittest for bit vectors
GPOS_RESULT
CEquivalenceClassesTest::EresUnittest()
{
	CUnittest rgut[] =
		{
		GPOS_UNITTEST_FUNC(CEquivalenceClassesTest::EresUnittest_NotDisjointEquivalanceClasses),
		GPOS_UNITTEST_FUNC(CEquivalenceClassesTest::EresUnittest_IntersectEquivalanceClasses)
		};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

// Check disjoint equivalence classes are detected
GPOS_RESULT
CEquivalenceClassesTest::EresUnittest_NotDisjointEquivalanceClasses()
{
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	CColRefSet *pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool);

	// Setup an MD cache with a file-based provider
	CMDProviderMemory *pmdp = CTestUtils::m_pmdpf;
	pmdp->AddRef();
	CMDAccessor mda(memory_pool, CMDCache::Pcache());
	mda.RegisterProvider(CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc
				(
				memory_pool,
				&mda,
				NULL, /* pceeval */
				CTestUtils::GetCostModel(memory_pool)
				);

	// get column factory from optimizer context object
	CColumnFactory *pcf = COptCtxt::PoctxtFromTLS()->Pcf();

	CWStringConst strName(GPOS_WSZ_LIT("Test Column"));
	CName name(&strName);

	const IMDTypeInt4 *pmdtypeint4 = mda.PtMDType<IMDTypeInt4>();

	ULONG ulCols = 10;
	for (ULONG i = 0; i < ulCols; i++)
	{
		CColRef *pcr = pcf->PcrCreate(pmdtypeint4, IDefaultTypeModifier, name);
		pcrs->Include(pcr);

		GPOS_ASSERT(pcrs->FMember(pcr));
	}

	GPOS_ASSERT(pcrs->Size() == ulCols);

	CColRefSet *pcrsTwo = GPOS_NEW(memory_pool) CColRefSet(memory_pool, *pcrs);
	GPOS_ASSERT(pcrsTwo->Size() == ulCols);

	CColRefSet *pcrsThree = GPOS_NEW(memory_pool) CColRefSet(memory_pool);
	GPOS_ASSERT(pcrsThree->Size() == 0);
	CColRef *pcrThree = pcf->PcrCreate(pmdtypeint4, IDefaultTypeModifier, name);
	pcrsThree->Include(pcrThree);
	GPOS_ASSERT(pcrsThree->Size() == 1);

	DrgPcrs *pdrgpcrs = GPOS_NEW(memory_pool) DrgPcrs(memory_pool);
	pcrs->AddRef();
	pcrsTwo->AddRef();
	pdrgpcrs->Append(pcrs);
	pdrgpcrs->Append(pcrsTwo);
	GPOS_ASSERT(!CUtils::FEquivalanceClassesDisjoint(memory_pool,pdrgpcrs));
	
	DrgPcrs *pdrgpcrsTwo = GPOS_NEW(memory_pool) DrgPcrs(memory_pool);
	pcrs->AddRef();
	pcrsThree->AddRef();
	pdrgpcrsTwo->Append(pcrs);
	pdrgpcrsTwo->Append(pcrsThree);
	GPOS_ASSERT(CUtils::FEquivalanceClassesDisjoint(memory_pool,pdrgpcrsTwo));
	
	pcrsThree->Release();
	pcrsTwo->Release();
	pcrs->Release();
	pdrgpcrs->Release();
	pdrgpcrsTwo->Release();

	return GPOS_OK;
}

// Check disjoint equivalence classes are detected
GPOS_RESULT
CEquivalenceClassesTest::EresUnittest_IntersectEquivalanceClasses()
{
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	CColRefSet *pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool);

	// Setup an MD cache with a file-based provider
	CMDProviderMemory *pmdp = CTestUtils::m_pmdpf;
	pmdp->AddRef();
	CMDAccessor mda(memory_pool, CMDCache::Pcache());
	mda.RegisterProvider(CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc
	(
	 memory_pool,
	 &mda,
	 NULL, /* pceeval */
	 CTestUtils::GetCostModel(memory_pool)
	 );

	// get column factory from optimizer context object
	CColumnFactory *pcf = COptCtxt::PoctxtFromTLS()->Pcf();

	CWStringConst strName(GPOS_WSZ_LIT("Test Column"));
	CName name(&strName);

	const IMDTypeInt4 *pmdtypeint4 = mda.PtMDType<IMDTypeInt4>();

	ULONG ulCols = 10;
	for (ULONG i = 0; i < ulCols; i++)
	{
		CColRef *pcr = pcf->PcrCreate(pmdtypeint4, IDefaultTypeModifier, name);
		pcrs->Include(pcr);

		GPOS_ASSERT(pcrs->FMember(pcr));
	}

	GPOS_ASSERT(pcrs->Size() == ulCols);

	// Generate equivalence classes
	INT setBoundaryFirst[] = {2,5,7};
	DrgPcrs *pdrgpFirst = CTestUtils::createEquivalenceClasses(memory_pool, pcrs, setBoundaryFirst);

	INT setBoundarySecond[] = {1,4,5,6};
	DrgPcrs *pdrgpSecond = CTestUtils::createEquivalenceClasses(memory_pool, pcrs, setBoundarySecond);

	INT setBoundaryExpected[] = {1,2,4,5,6,7};
	DrgPcrs *pdrgpIntersectExpectedOp = CTestUtils::createEquivalenceClasses(memory_pool, pcrs, setBoundaryExpected);

	DrgPcrs *pdrgpResult = CUtils::PdrgpcrsIntersectEquivClasses(memory_pool, pdrgpFirst, pdrgpSecond);
	GPOS_ASSERT(CUtils::FEquivalanceClassesDisjoint(memory_pool,pdrgpResult));
	GPOS_ASSERT(CUtils::FEquivalanceClassesEqual(memory_pool, pdrgpResult, pdrgpIntersectExpectedOp));

	pcrs->Release();
	pdrgpFirst->Release();
	pdrgpResult->Release();
	pdrgpSecond->Release();
	pdrgpIntersectExpectedOp->Release();

	return GPOS_OK;
}
// EOF
