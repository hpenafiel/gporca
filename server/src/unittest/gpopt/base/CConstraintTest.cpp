//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CConstraintTest.cpp
//
//	@doc:
//		Test for constraint
//---------------------------------------------------------------------------

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

#include <stdint.h>

#include "gpos/task/CAutoTraceFlag.h"

#include "unittest/base.h"
#include "unittest/gpopt/base/CConstraintTest.h"
#include "unittest/gpopt/CConstExprEvaluatorForDates.h"

#include "naucrates/base/CDatumInt8GPDB.h"

#include "gpopt/base/CDefaultComparator.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/eval/CConstExprEvaluatorDefault.h"
#include "gpopt/operators/CPredicateUtils.h"

#include "naucrates/md/CMDIdGPDB.h"
#include "naucrates/md/IMDScalarOp.h"

// number of microseconds in one day
const LINT CConstraintTest::lMicrosecondsPerDay = 24 * 60 * 60 * INT64_C(1000000);

// date for '01-01-2012'
const LINT CConstraintTest::lInternalRepresentationFor2012_01_01 =
		LINT(4383) * CConstraintTest::lMicrosecondsPerDay;

// date for '01-21-2012'
const LINT CConstraintTest::lInternalRepresentationFor2012_01_21 =
		LINT(5003) * CConstraintTest::lMicrosecondsPerDay;

// date for '01-02-2012'
const LINT CConstraintTest::lInternalRepresentationFor2012_01_02 =
		LINT(4384) * CConstraintTest::lMicrosecondsPerDay;

// date for '01-22-2012'
const LINT CConstraintTest::lInternalRepresentationFor2012_01_22 =
		LINT(5004) * CConstraintTest::lMicrosecondsPerDay;

// byte representation for '01-01-2012'
const WCHAR *CConstraintTest::wszInternalRepresentationFor2012_01_01 =
		GPOS_WSZ_LIT("HxEAAA==");

// byte representation for '01-21-2012'
const WCHAR *CConstraintTest::wszInternalRepresentationFor2012_01_21 =
		GPOS_WSZ_LIT("MxEAAA==");

// byte representation for '01-02-2012'
const WCHAR *CConstraintTest::wszInternalRepresentationFor2012_01_02 =
		GPOS_WSZ_LIT("IBEAAA==");

// byte representation for '01-22-2012'
const WCHAR *CConstraintTest::wszInternalRepresentationFor2012_01_22 =
		GPOS_WSZ_LIT("MhEAAA==");

static GPOS_RESULT EresUnittest_CConstraintIntervalFromArrayExprIncludesNull();

//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::EresUnittest
//
//	@doc:
//		Unittest for ranges
//
//---------------------------------------------------------------------------
GPOS_RESULT
CConstraintTest::EresUnittest()
{
	CUnittest rgut[] =
		{
		GPOS_UNITTEST_FUNC(EresUnittest_CConstraintIntervalFromArrayExprIncludesNull),
		GPOS_UNITTEST_FUNC(CConstraintTest::EresUnittest_CInterval),
		GPOS_UNITTEST_FUNC(CConstraintTest::EresUnittest_CIntervalFromScalarExpr),
		GPOS_UNITTEST_FUNC(CConstraintTest::EresUnittest_CConjunction),
		GPOS_UNITTEST_FUNC(CConstraintTest::EresUnittest_CDisjunction),
		GPOS_UNITTEST_FUNC(CConstraintTest::EresUnittest_CNegation),
		GPOS_UNITTEST_FUNC(CConstraintTest::EresUnittest_CConstraintFromScalarExpr),
		GPOS_UNITTEST_FUNC(CConstraintTest::EresUnittest_CConstraintIntervalConvertsTo),
		GPOS_UNITTEST_FUNC(CConstraintTest::EresUnittest_CConstraintIntervalPexpr),
		GPOS_UNITTEST_FUNC(CConstraintTest::EresUnittest_CConstraintIntervalFromArrayExpr),
#ifdef GPOS_DEBUG
		GPOS_UNITTEST_FUNC_THROW
			(
			CConstraintTest::EresUnittest_NegativeTests,
			gpos::CException::ExmaSystem,
			gpos::CException::ExmiAssert
			),
#endif // GPOS_DEBUG
		GPOS_UNITTEST_FUNC(CConstraintTest::EresUnittest_ConstraintsOnDates),
		};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::EresUnittest_CInterval
//
//	@doc:
//		Interval tests
//
//---------------------------------------------------------------------------
GPOS_RESULT
CConstraintTest::EresUnittest_CInterval()
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	// setup a file-based provider
	CMDProviderMemory *pmdp = CTestUtils::m_pmdpf;
	pmdp->AddRef();
	CMDAccessor mda(memory_pool, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	CConstExprEvaluatorForDates *pceeval = GPOS_NEW(memory_pool) CConstExprEvaluatorForDates(memory_pool);

	// install opt context in TLS
	CAutoOptCtxt aoc(memory_pool, &mda, pceeval, CTestUtils::Pcm(memory_pool));
	GPOS_ASSERT(NULL != COptCtxt::PoctxtFromTLS()->Pcomp());

	IMDTypeInt8 *pmdtypeint8 = (IMDTypeInt8 *) mda.PtMDType<IMDTypeInt8>(CTestUtils::m_sysidDefault);
	IMDId *pmdid = pmdtypeint8->MDId();

	CExpression *pexprGet = CTestUtils::PexprLogicalGet(memory_pool);
	CColRefSet *pcrs = CDrvdPropRelational::Pdprel(pexprGet->PdpDerive())->PcrsOutput();
	CColRef *pcr =  pcrs->PcrAny();

	// first interval
	CConstraintInterval *pciFirst = PciFirstInterval(memory_pool, pmdid, pcr);
	PrintConstraint(memory_pool, pciFirst);

	// second interval
	CConstraintInterval *pciSecond = PciSecondInterval(memory_pool, pmdid, pcr);
	PrintConstraint(memory_pool, pciSecond);

	// intersection
	CConstraintInterval *pciIntersect = pciFirst->PciIntersect(memory_pool, pciSecond);
	PrintConstraint(memory_pool, pciIntersect);

	// union
	CConstraintInterval *pciUnion = pciFirst->PciUnion(memory_pool, pciSecond);
	PrintConstraint(memory_pool, pciUnion);

	// diff 1
	CConstraintInterval *pciDiff1 = pciFirst->PciDifference(memory_pool, pciSecond);
	PrintConstraint(memory_pool, pciDiff1);

	// diff 2
	CConstraintInterval *pciDiff2 = pciSecond->PciDifference(memory_pool, pciFirst);
	PrintConstraint(memory_pool, pciDiff2);

	// complement
	CConstraintInterval *pciComp = pciFirst->PciComplement(memory_pool);
	PrintConstraint(memory_pool, pciComp);

	// containment
	GPOS_ASSERT(!pciFirst->FContains(pciSecond));
	GPOS_ASSERT(pciFirst->FContains(pciDiff1));
	GPOS_ASSERT(!pciSecond->FContains(pciFirst));
	GPOS_ASSERT(pciSecond->FContains(pciDiff2));

	// equality
	CConstraintInterval *pciThird = PciFirstInterval(memory_pool, pmdid, pcr);
	pciThird->AddRef();
	CConstraintInterval *pciFourth = pciThird;
	GPOS_ASSERT(!pciFirst->Equals(pciSecond));
	GPOS_ASSERT(!pciFirst->Equals(pciDiff1));
	GPOS_ASSERT(!pciSecond->Equals(pciDiff2));
	GPOS_ASSERT(pciFirst->Equals(pciThird));
	GPOS_ASSERT(pciFourth->Equals(pciThird));

	pciFirst->Release();
	pciSecond->Release();
	pciThird->Release();
	pciFourth->Release();
	pciIntersect->Release();
	pciUnion->Release();
	pciDiff1->Release();
	pciDiff2->Release();
	pciComp->Release();

	pexprGet->Release();

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::EresUnittest_CConjunction
//
//	@doc:
//		Conjunction test
//
//---------------------------------------------------------------------------
GPOS_RESULT
CConstraintTest::EresUnittest_CConjunction()
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	// setup a file-based provider
	CMDProviderMemory *pmdp = CTestUtils::m_pmdpf;
	pmdp->AddRef();
	CMDAccessor mda(memory_pool, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	CConstExprEvaluatorForDates *pceeval = GPOS_NEW(memory_pool) CConstExprEvaluatorForDates(memory_pool);

	// install opt context in TLS
	CAutoOptCtxt aoc(memory_pool, &mda, pceeval, CTestUtils::Pcm(memory_pool));
	GPOS_ASSERT(NULL != COptCtxt::PoctxtFromTLS()->Pcomp());

	IMDTypeInt8 *pmdtypeint8 = (IMDTypeInt8 *) mda.PtMDType<IMDTypeInt8>(CTestUtils::m_sysidDefault);
	IMDId *pmdid = pmdtypeint8->MDId();

	CExpression *pexprGet1 = CTestUtils::PexprLogicalGet(memory_pool);
	CColRefSet *pcrs1 = CDrvdPropRelational::Pdprel(pexprGet1->PdpDerive())->PcrsOutput();
	CColRef *pcr1 =  pcrs1->PcrAny();

	CExpression *pexprGet2 = CTestUtils::PexprLogicalGet(memory_pool);
	CColRefSet *pcrs2 = CDrvdPropRelational::Pdprel(pexprGet2->PdpDerive())->PcrsOutput();
	CColRef *pcr2 =  pcrs2->PcrAny();

	CConstraintConjunction *pcconj1 = Pcstconjunction(memory_pool, pmdid, pcr1);
	PrintConstraint(memory_pool, pcconj1);
	GPOS_ASSERT(!pcconj1->FContradiction());

	CConstraintConjunction *pcconj2 = Pcstconjunction(memory_pool, pmdid, pcr2);
	PrintConstraint(memory_pool, pcconj2);

	DrgPcnstr *pdrgpcst = GPOS_NEW(memory_pool) DrgPcnstr(memory_pool);
	pcconj1->AddRef();
	pcconj2->AddRef();
	pdrgpcst->Append(pcconj1);
	pdrgpcst->Append(pcconj2);

	CConstraintConjunction *pcconjTop = GPOS_NEW(memory_pool) CConstraintConjunction(memory_pool, pdrgpcst);
	PrintConstraint(memory_pool, pcconjTop);

	// containment
	GPOS_ASSERT(!pcconj1->FContains(pcconj2));
	GPOS_ASSERT(!pcconj2->FContains(pcconj1));
	GPOS_ASSERT(pcconj1->FContains(pcconjTop));
	GPOS_ASSERT(!pcconjTop->FContains(pcconj1));
	GPOS_ASSERT(pcconj2->FContains(pcconjTop));
	GPOS_ASSERT(!pcconjTop->FContains(pcconj2));

	// equality
	GPOS_ASSERT(!pcconj1->Equals(pcconjTop));
	GPOS_ASSERT(!pcconjTop->Equals(pcconj2));

	pcconjTop->Release();
	pcconj1->Release();
	pcconj2->Release();

	pexprGet1->Release();
	pexprGet2->Release();

	return GPOS_OK;
}


//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::Pcstconjunction
//
//	@doc:
//		Build a conjunction
//
//---------------------------------------------------------------------------
CConstraintConjunction *
CConstraintTest::Pcstconjunction
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid,
	CColRef *pcr
	)
{
	// first interval
	CConstraintInterval *pciFirst = PciFirstInterval(memory_pool, pmdid, pcr);

	// second interval
	CConstraintInterval *pciSecond = PciSecondInterval(memory_pool, pmdid, pcr);

	DrgPcnstr *pdrgpcst = GPOS_NEW(memory_pool) DrgPcnstr(memory_pool);
	pdrgpcst->Append(pciFirst);
	pdrgpcst->Append(pciSecond);

	return GPOS_NEW(memory_pool) CConstraintConjunction(memory_pool, pdrgpcst);
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::Pcstdisjunction
//
//	@doc:
//		Build a disjunction
//
//---------------------------------------------------------------------------
CConstraintDisjunction *
CConstraintTest::Pcstdisjunction
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid,
	CColRef *pcr
	)
{
	// first interval
	CConstraintInterval *pciFirst = PciFirstInterval(memory_pool, pmdid, pcr);

	// second interval
	CConstraintInterval *pciSecond = PciSecondInterval(memory_pool, pmdid, pcr);

	DrgPcnstr *pdrgpcst = GPOS_NEW(memory_pool) DrgPcnstr(memory_pool);
	pdrgpcst->Append(pciFirst);
	pdrgpcst->Append(pciSecond);

	return GPOS_NEW(memory_pool) CConstraintDisjunction(memory_pool, pdrgpcst);
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::EresUnittest_CDisjunction
//
//	@doc:
//		Conjunction test
//
//---------------------------------------------------------------------------
GPOS_RESULT
CConstraintTest::EresUnittest_CDisjunction()
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	// setup a file-based provider
	CMDProviderMemory *pmdp = CTestUtils::m_pmdpf;
	pmdp->AddRef();
	CMDAccessor mda(memory_pool, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	CConstExprEvaluatorForDates *pceeval = GPOS_NEW(memory_pool) CConstExprEvaluatorForDates(memory_pool);

	// install opt context in TLS
	CAutoOptCtxt aoc(memory_pool, &mda, pceeval, CTestUtils::Pcm(memory_pool));
	GPOS_ASSERT(NULL != COptCtxt::PoctxtFromTLS()->Pcomp());

	IMDTypeInt8 *pmdtypeint8 = (IMDTypeInt8 *) mda.PtMDType<IMDTypeInt8>(CTestUtils::m_sysidDefault);
	IMDId *pmdid = pmdtypeint8->MDId();

	CExpression *pexprGet1 = CTestUtils::PexprLogicalGet(memory_pool);
	CColRefSet *pcrs1 = CDrvdPropRelational::Pdprel(pexprGet1->PdpDerive())->PcrsOutput();
	CColRef *pcr1 =  pcrs1->PcrAny();

	CExpression *pexprGet2 = CTestUtils::PexprLogicalGet(memory_pool);
	CColRefSet *pcrs2 = CDrvdPropRelational::Pdprel(pexprGet2->PdpDerive())->PcrsOutput();
	CColRef *pcr2 =  pcrs2->PcrAny();

	CConstraintDisjunction *pcdisj1 = Pcstdisjunction(memory_pool, pmdid, pcr1);
	PrintConstraint(memory_pool, pcdisj1);
	GPOS_ASSERT(!pcdisj1->FContradiction());

	CConstraintDisjunction *pcdisj2 = Pcstdisjunction(memory_pool, pmdid, pcr2);
	PrintConstraint(memory_pool, pcdisj2);

	DrgPcnstr *pdrgpcst = GPOS_NEW(memory_pool) DrgPcnstr(memory_pool);
	pcdisj1->AddRef();
	pcdisj2->AddRef();
	pdrgpcst->Append(pcdisj1);
	pdrgpcst->Append(pcdisj2);

	CConstraintDisjunction *pcdisjTop = GPOS_NEW(memory_pool) CConstraintDisjunction(memory_pool, pdrgpcst);
	PrintConstraint(memory_pool, pcdisjTop);

	// containment
	GPOS_ASSERT(!pcdisj1->FContains(pcdisj2));
	GPOS_ASSERT(!pcdisj2->FContains(pcdisj1));
	GPOS_ASSERT(!pcdisj1->FContains(pcdisjTop));
	GPOS_ASSERT(pcdisjTop->FContains(pcdisj1));
	GPOS_ASSERT(!pcdisj2->FContains(pcdisjTop));
	GPOS_ASSERT(pcdisjTop->FContains(pcdisj2));

	// equality
	GPOS_ASSERT(!pcdisj1->Equals(pcdisjTop));
	GPOS_ASSERT(!pcdisjTop->Equals(pcdisj2));

	pcdisjTop->Release();
	pcdisj1->Release();
	pcdisj2->Release();

	pexprGet1->Release();
	pexprGet2->Release();

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::EresUnittest_CNegation
//
//	@doc:
//		Conjunction test
//
//---------------------------------------------------------------------------
GPOS_RESULT
CConstraintTest::EresUnittest_CNegation()
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	// setup a file-based provider
	CMDProviderMemory *pmdp = CTestUtils::m_pmdpf;
	pmdp->AddRef();
	CMDAccessor mda(memory_pool, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	CConstExprEvaluatorForDates *pceeval = GPOS_NEW(memory_pool) CConstExprEvaluatorForDates(memory_pool);

	// install opt context in TLS
	CAutoOptCtxt aoc(memory_pool, &mda, pceeval, CTestUtils::Pcm(memory_pool));
	GPOS_ASSERT(NULL != COptCtxt::PoctxtFromTLS()->Pcomp());

	IMDTypeInt8 *pmdtypeint8 = (IMDTypeInt8 *) mda.PtMDType<IMDTypeInt8>(CTestUtils::m_sysidDefault);
	IMDId *pmdid = pmdtypeint8->MDId();

	CExpression *pexprGet = CTestUtils::PexprLogicalGet(memory_pool);
	CColRefSet *pcrs = CDrvdPropRelational::Pdprel(pexprGet->PdpDerive())->PcrsOutput();
	CColRef *pcr =  pcrs->PcrAny();

	CConstraintInterval *pci = PciFirstInterval(memory_pool, pmdid, pcr);
	PrintConstraint(memory_pool, pci);

	pci->AddRef();
	CConstraintNegation *pcn1 = GPOS_NEW(memory_pool) CConstraintNegation(memory_pool, pci);
	PrintConstraint(memory_pool, pcn1);

	pcn1->AddRef();
	CConstraintNegation *pcn2 = GPOS_NEW(memory_pool) CConstraintNegation(memory_pool, pcn1);
	PrintConstraint(memory_pool, pcn2);

	// containment
	GPOS_ASSERT(!pcn1->FContains(pci));
	GPOS_ASSERT(!pci->FContains(pcn1));
	GPOS_ASSERT(!pcn2->FContains(pcn1));
	GPOS_ASSERT(!pcn1->FContains(pcn2));
	GPOS_ASSERT(pci->FContains(pcn2));
	GPOS_ASSERT(pcn2->FContains(pci));

	// equality
	GPOS_ASSERT(!pcn1->Equals(pci));
	GPOS_ASSERT(!pcn1->Equals(pcn2));
	GPOS_ASSERT(pci->Equals(pcn2));

	pcn2->Release();
	pcn1->Release();
	pci->Release();

	pexprGet->Release();

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::EresUnittest_CIntervalFromScalarExpr
//
//	@doc:
//		Interval from scalar tests
//
//---------------------------------------------------------------------------
GPOS_RESULT
CConstraintTest::EresUnittest_CIntervalFromScalarExpr()
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	// setup a file-based provider
	CMDProviderMemory *pmdp = CTestUtils::m_pmdpf;
	pmdp->AddRef();
	CMDAccessor mda(memory_pool, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	CConstExprEvaluatorForDates *pceeval = GPOS_NEW(memory_pool) CConstExprEvaluatorForDates(memory_pool);

	// install opt context in TLS
	CAutoOptCtxt aoc(memory_pool, &mda, pceeval, CTestUtils::Pcm(memory_pool));
	GPOS_ASSERT(NULL != COptCtxt::PoctxtFromTLS()->Pcomp());

	CExpression *pexprGet = CTestUtils::PexprLogicalGet(memory_pool);
	CColRefSet *pcrs = CDrvdPropRelational::Pdprel(pexprGet->PdpDerive())->PcrsOutput();
	CColRef *pcr =  pcrs->PcrAny();

	// from ScalarCmp
	GPOS_RESULT eres1 = EresUnittest_CIntervalFromScalarCmp(memory_pool, &mda, pcr);

	// from ScalarBool
	GPOS_RESULT eres2 = EresUnittest_CIntervalFromScalarBoolOp(memory_pool, &mda, pcr);

	pexprGet->Release();
	if (GPOS_OK == eres1 && GPOS_OK == eres2)
	{
		return GPOS_OK;
	}

	return GPOS_FAILED;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::EresUnittest_CConstraintFromScalarExpr
//
//	@doc:
//		Constraint from scalar tests
//
//---------------------------------------------------------------------------
GPOS_RESULT
CConstraintTest::EresUnittest_CConstraintFromScalarExpr()
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	// setup a file-based provider
	CMDProviderMemory *pmdp = CTestUtils::m_pmdpf;
	pmdp->AddRef();
	CMDAccessor mda(memory_pool, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	CConstExprEvaluatorForDates *pceeval = GPOS_NEW(memory_pool) CConstExprEvaluatorForDates(memory_pool);

	// install opt context in TLS
	CAutoOptCtxt aoc(memory_pool, &mda, pceeval, CTestUtils::Pcm(memory_pool));
	GPOS_ASSERT(NULL != COptCtxt::PoctxtFromTLS()->Pcomp());

	CExpression *pexprGet1 = CTestUtils::PexprLogicalGet(memory_pool);
	CColRefSet *pcrs1 = CDrvdPropRelational::Pdprel(pexprGet1->PdpDerive())->PcrsOutput();
	CColRef *pcr1 =  pcrs1->PcrAny();

	CExpression *pexprGet2 = CTestUtils::PexprLogicalGet(memory_pool);
	CColRefSet *pcrs2 = CDrvdPropRelational::Pdprel(pexprGet2->PdpDerive())->PcrsOutput();
	CColRef *pcr2 =  pcrs2->PcrAny();

	DrgPcrs *pdrgpcrs = NULL;

	// expression with 1 column
	CExpression *pexpr = PexprScalarCmp(memory_pool, &mda, pcr1, IMDType::EcmptG, 15);
	CConstraint *pcnstr = CConstraint::PcnstrFromScalarExpr(memory_pool, pexpr, &pdrgpcrs);
	PrintConstraint(memory_pool, pcnstr);
	PrintEquivClasses(memory_pool, pdrgpcrs);
	pdrgpcrs->Release();
	pdrgpcrs = NULL;
	pcnstr->Release();
	pexpr->Release();

	// expression with 2 columns
	DrgPexpr *pdrgpexpr = GPOS_NEW(memory_pool) DrgPexpr(memory_pool);
	pdrgpexpr->Append(PexprScalarCmp(memory_pool, &mda, pcr1, IMDType::EcmptG, 15));
	pdrgpexpr->Append(PexprScalarCmp(memory_pool, &mda, pcr2, IMDType::EcmptL, 20));

	CExpression *pexprAnd = CUtils::PexprScalarBoolOp(memory_pool, CScalarBoolOp::EboolopAnd, pdrgpexpr);
	(void) pexprAnd->PdpDerive();

	CConstraint *pcnstrAnd = CConstraint::PcnstrFromScalarExpr(memory_pool, pexprAnd, &pdrgpcrs);
	PrintConstraint(memory_pool, pcnstrAnd);
	PrintEquivClasses(memory_pool, pdrgpcrs);
	pdrgpcrs->Release();
	pdrgpcrs = NULL;

	pcnstrAnd->Release();
	pexprAnd->Release();

	// equality predicate with 2 columns
	CExpression *pexprEq = CUtils::PexprScalarEqCmp(memory_pool, pcr1, pcr2);
	(void) pexprEq->PdpDerive();
	CConstraint *pcnstrEq = CConstraint::PcnstrFromScalarExpr(memory_pool, pexprEq, &pdrgpcrs);
	PrintConstraint(memory_pool, pcnstrEq);
	PrintEquivClasses(memory_pool, pdrgpcrs);

	pcnstrEq->Release();
	pexprEq->Release();

	pdrgpcrs->Release();

	pexprGet1->Release();
	pexprGet2->Release();

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::EresUnittest_CConstraintIntervalConvertsTo
//
//	@doc:
//		Tests CConstraintInterval::ConvertsToIn and
//		CConstraintInterval::ConvertsToNotIn
//
//---------------------------------------------------------------------------
GPOS_RESULT
CConstraintTest::EresUnittest_CConstraintIntervalConvertsTo()
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	// setup a file-based provider
	CMDProviderMemory *pmdp = CTestUtils::m_pmdpf;
	pmdp->AddRef();
	CMDAccessor mda(memory_pool, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	CConstExprEvaluatorForDates *pceeval = GPOS_NEW(memory_pool) CConstExprEvaluatorForDates(memory_pool);

	// install opt context in TLS
	CAutoOptCtxt aoc(memory_pool, &mda, pceeval, CTestUtils::Pcm(memory_pool));
	GPOS_ASSERT(NULL != COptCtxt::PoctxtFromTLS()->Pcomp());

	// create a range which should convert to an IN array expression
	const SRangeInfo rgRangeInfoIn[] =
			{
				{CRange::EriIncluded, -1000, CRange::EriIncluded, -1000},
				{CRange::EriIncluded, -5, CRange::EriIncluded, -5},
				{CRange::EriIncluded, 0, CRange::EriIncluded, 0}
			};

	// metadata id
	IMDTypeInt8 *pmdtypeint8 = (IMDTypeInt8 *) mda.PtMDType<IMDTypeInt8>(CTestUtils::m_sysidDefault);
	IMDId *pmdid = pmdtypeint8->MDId();

	// get a column ref
	CExpression *pexprGet = CTestUtils::PexprLogicalGet(memory_pool);
	CColRefSet *pcrs = CDrvdPropRelational::Pdprel(pexprGet->PdpDerive())->PcrsOutput();
	CColRef *pcr =  pcrs->PcrAny();

	// create constraint
	DrgPrng *pdrgprng = Pdrgprng(memory_pool, pmdid, rgRangeInfoIn, GPOS_ARRAY_SIZE(rgRangeInfoIn));
	CConstraintInterval *pcnstin = GPOS_NEW(memory_pool) CConstraintInterval(memory_pool, pcr, pdrgprng, true);

	PrintConstraint(memory_pool, pcnstin);

	// should convert to in
	GPOS_ASSERT(pcnstin->FConvertsToIn());
	GPOS_ASSERT(!pcnstin->FConvertsToNotIn());

	CConstraintInterval *pcnstNotIn = pcnstin->PciComplement(memory_pool);

	// should convert to a not in statement after taking the complement
	GPOS_ASSERT(pcnstNotIn->FConvertsToNotIn());
	GPOS_ASSERT(!pcnstNotIn->FConvertsToIn());

	pcnstin->Release();
	pcnstNotIn->Release();
	pexprGet->Release();

	return GPOS_OK;
}


//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::EresUnittest_CConstraintIntervalPexpr
//
//	@doc:
//		Tests CConstraintInterval::PexprConstructArrayScalar
//
//---------------------------------------------------------------------------
GPOS_RESULT
CConstraintTest::EresUnittest_CConstraintIntervalPexpr()
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	// setup a file-based provider
	CMDProviderMemory *pmdp = CTestUtils::m_pmdpf;
	pmdp->AddRef();
	CMDAccessor mda(memory_pool, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	CConstExprEvaluatorForDates *pceeval = GPOS_NEW(memory_pool) CConstExprEvaluatorForDates(memory_pool);

	// install opt context in TLS
	CAutoOptCtxt aoc(memory_pool, &mda, pceeval, CTestUtils::Pcm(memory_pool));
	GPOS_RTL_ASSERT(NULL != COptCtxt::PoctxtFromTLS()->Pcomp());

	CAutoTraceFlag atf(EopttraceArrayConstraints, true);

	// create a range which should convert to an IN array expression
	const SRangeInfo rgRangeInfoIn[] =
			{
				{CRange::EriIncluded, -1000, CRange::EriIncluded, -1000},
				{CRange::EriIncluded, -5, CRange::EriIncluded, -5},
				{CRange::EriIncluded, 0, CRange::EriIncluded, 0}
			};

	// metadata id
	IMDTypeInt8 *pmdtypeint8 = (IMDTypeInt8 *) mda.PtMDType<IMDTypeInt8>(CTestUtils::m_sysidDefault);
	IMDId *pmdid = pmdtypeint8->MDId();

	// get a column ref
	CExpression *pexprGet = CTestUtils::PexprLogicalGet(memory_pool);
	CColRefSet *pcrs = CDrvdPropRelational::Pdprel(pexprGet->PdpDerive())->PcrsOutput();
	CColRef *pcr =  pcrs->PcrAny();

	DrgPrng *pdrgprng = NULL;
	CConstraintInterval *pcnstin = NULL;
	CExpression *pexpr = NULL;
	CConstraintInterval *pcnstNotIn = NULL;

	// IN CONSTRAINT FOR SIMPLE INTERVAL (WITHOUT NULL)

	// create constraint
	pdrgprng = Pdrgprng(memory_pool, pmdid, rgRangeInfoIn, GPOS_ARRAY_SIZE(rgRangeInfoIn));
	pcnstin = GPOS_NEW(memory_pool) CConstraintInterval(memory_pool, pcr, pdrgprng, false);

	pexpr = pcnstin->PexprScalar(memory_pool); // pexpr is owned by the constraint
	PrintConstraint(memory_pool, pcnstin);

	GPOS_RTL_ASSERT(!pcnstin->FConvertsToNotIn());
	GPOS_RTL_ASSERT(pcnstin->FConvertsToIn());
	GPOS_RTL_ASSERT(CUtils::FScalarArrayCmp(pexpr));
	GPOS_RTL_ASSERT(3 == CUtils::UlCountOperator(pexpr, COperator::EopScalarConst));

	pcnstin->Release();


	// IN CONSTRAINT FOR SIMPLE INTERVAL WITH NULL

	// create constraint
	pdrgprng = Pdrgprng(memory_pool, pmdid, rgRangeInfoIn, GPOS_ARRAY_SIZE(rgRangeInfoIn));
	pcnstin = GPOS_NEW(memory_pool) CConstraintInterval(memory_pool, pcr, pdrgprng, true);

	pexpr = pcnstin->PexprScalar(memory_pool); // pexpr is owned by the constraint
	PrintConstraint(memory_pool, pcnstin);

	GPOS_RTL_ASSERT(!pcnstin->FConvertsToNotIn());
	GPOS_RTL_ASSERT(pcnstin->FConvertsToIn());
	GPOS_RTL_ASSERT(CUtils::FScalarArrayCmp(pexpr));
	GPOS_RTL_ASSERT(4 == CUtils::UlCountOperator(pexpr, COperator::EopScalarConst));

	pcnstin->Release();


	// NOT IN CONSTRAINT FOR SIMPLE INTERVAL WITHOUT NULL

	// create constraint
	pdrgprng = Pdrgprng(memory_pool, pmdid, rgRangeInfoIn, GPOS_ARRAY_SIZE(rgRangeInfoIn));
	pcnstin = GPOS_NEW(memory_pool) CConstraintInterval(memory_pool, pcr, pdrgprng, true);

	pcnstNotIn = pcnstin->PciComplement(memory_pool);
	pcnstin->Release();

	pexpr = pcnstNotIn->PexprScalar(memory_pool); // pexpr is owned by the constraint
	PrintConstraint(memory_pool, pcnstNotIn);

	GPOS_RTL_ASSERT(pcnstNotIn->FConvertsToNotIn());
	GPOS_RTL_ASSERT(!pcnstNotIn->FConvertsToIn());
	GPOS_RTL_ASSERT(CUtils::FScalarArrayCmp(pexpr));
	GPOS_RTL_ASSERT(3 == CUtils::UlCountOperator(pexpr, COperator::EopScalarConst));

	pcnstNotIn->Release();


	// NOT IN CONSTRAINT FOR SIMPLE INTERVAL WITH NULL

	// create constraint
	pdrgprng = Pdrgprng(memory_pool, pmdid, rgRangeInfoIn, GPOS_ARRAY_SIZE(rgRangeInfoIn));
	pcnstin = GPOS_NEW(memory_pool) CConstraintInterval(memory_pool, pcr, pdrgprng, false);

	pcnstNotIn = pcnstin->PciComplement(memory_pool);
	pcnstin->Release();

	pexpr = pcnstNotIn->PexprScalar(memory_pool); // pexpr is owned by the constraint
	PrintConstraint(memory_pool, pcnstNotIn);

	GPOS_RTL_ASSERT(pcnstNotIn->FConvertsToNotIn());
	GPOS_RTL_ASSERT(!pcnstNotIn->FConvertsToIn());
	GPOS_RTL_ASSERT(CUtils::FScalarArrayCmp(pexpr));
	GPOS_RTL_ASSERT(4 == CUtils::UlCountOperator(pexpr, COperator::EopScalarConst));

	pcnstNotIn->Release();


	pexprGet->Release();

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::EresUnittest_CConstraintIntervalFromArrayExpr
//
//	@doc:
//		Tests CConstraintInterval::PcnstrIntervalFromScalarArrayCmp
//
//---------------------------------------------------------------------------
GPOS_RESULT
CConstraintTest::EresUnittest_CConstraintIntervalFromArrayExpr()
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	// setup a file-based provider
	CMDProviderMemory *pmdp = CTestUtils::m_pmdpf;
	pmdp->AddRef();
	CMDAccessor mda(memory_pool, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	CConstExprEvaluatorForDates *pceeval = GPOS_NEW(memory_pool) CConstExprEvaluatorForDates(memory_pool);

	// install opt context in TLS
	CAutoOptCtxt aoc(memory_pool, &mda, pceeval, CTestUtils::Pcm(memory_pool));
	GPOS_ASSERT(NULL != COptCtxt::PoctxtFromTLS()->Pcomp());

	CAutoTraceFlag atf(EopttraceArrayConstraints, true);

	// Create an IN array expression
	CExpression *pexpr = CTestUtils::PexprLogicalSelectArrayCmp(memory_pool);
	// get a ref to the comparison column
	CColRef *pcr = CDrvdPropRelational::Pdprel(pexpr->PdpDerive())->PcrsOutput()->PcrAny();

	// remove the array child
	CExpression *pexprArrayComp = (*pexpr->PdrgPexpr())[1];
	GPOS_ASSERT(CUtils::FScalarArrayCmp(pexprArrayComp));

	CConstraintInterval *pcnstIn = CConstraintInterval::PciIntervalFromScalarExpr(memory_pool, pexprArrayComp, pcr);
	GPOS_ASSERT(CConstraint::EctInterval == pcnstIn->Ect());
	GPOS_ASSERT(pcnstIn->Pdrgprng()->Size() == CUtils::UlCountOperator(pexprArrayComp, COperator::EopScalarConst));

	pcnstIn->Release();
	pexpr->Release();

	// test a NOT IN expression

	CExpression *pexprNotIn = CTestUtils::PexprLogicalSelectArrayCmp(memory_pool, CScalarArrayCmp::EarrcmpAll, IMDType::EcmptNEq);
	CExpression *pexprArrayNotInComp = (*pexprNotIn->PdrgPexpr())[1];
	CColRef *pcrNot = CDrvdPropRelational::Pdprel(pexprNotIn->PdpDerive())->PcrsOutput()->PcrAny();
	CConstraintInterval *pcnstNotIn = CConstraintInterval::PciIntervalFromScalarExpr(memory_pool, pexprArrayNotInComp, pcrNot);
	GPOS_ASSERT(CConstraint::EctInterval == pcnstNotIn->Ect());
	// a NOT IN range array should have one more element than the expression array consts
	GPOS_ASSERT(pcnstNotIn->Pdrgprng()->Size() == 1 + CUtils::UlCountOperator(pexprArrayNotInComp, COperator::EopScalarConst));

	pexprNotIn->Release();
	pcnstNotIn->Release();

	// create an IN expression with repeated values
	IntPtrArray *pdrgpi = GPOS_NEW(memory_pool) IntPtrArray(memory_pool);
	INT aiValsRepeat[] = {5,1,2,5,3,4,5};
	ULONG aiValsLength = sizeof(aiValsRepeat)/sizeof(INT);
	for (ULONG ul = 0; ul < aiValsLength; ul++)
	{
		pdrgpi->Append(GPOS_NEW(memory_pool) INT(aiValsRepeat[ul]));
	}
	CExpression *pexprInRepeatsSelect =
			CTestUtils::PexprLogicalSelectArrayCmp(memory_pool, CScalarArrayCmp::EarrcmpAny, IMDType::EcmptEq, pdrgpi);
	CColRef *pcrInRepeats = CDrvdPropRelational::Pdprel(pexprInRepeatsSelect->PdpDerive())->PcrsOutput()->PcrAny();
	CExpression *pexprArrayCmpRepeats = (*pexprInRepeatsSelect->PdrgPexpr())[1];
	// add 2 repeated values and one unique
	CConstraintInterval *pcnstInRepeats = CConstraintInterval::PciIntervalFromScalarExpr(memory_pool, pexprArrayCmpRepeats, pcrInRepeats);
	GPOS_ASSERT(5 == pcnstInRepeats->Pdrgprng()->Size());
	pexprInRepeatsSelect->Release();
	pcnstInRepeats->Release();

	// create a NOT IN expression with repeated values
	CExpression *pexprNotInRepeatsSelect = CTestUtils::PexprLogicalSelectArrayCmp(memory_pool, CScalarArrayCmp::EarrcmpAll, IMDType::EcmptNEq, pdrgpi);
	CColRef *pcrNotInRepeats = CDrvdPropRelational::Pdprel(pexprNotInRepeatsSelect->PdpDerive())->PcrsOutput()->PcrAny();
	CExpression *pexprNotInArrayCmpRepeats = (*pexprNotInRepeatsSelect->PdrgPexpr())[1];
	CConstraintInterval *pcnstNotInRepeats = CConstraintInterval::PciIntervalFromScalarExpr(memory_pool, pexprNotInArrayCmpRepeats, pcrNotInRepeats);
	// a total of 5 unique ScalarConsts in the expression will result in 6 ranges
	GPOS_ASSERT(6 == pcnstNotInRepeats->Pdrgprng()->Size());
	pexprNotInRepeatsSelect->Release();
	pcnstNotInRepeats->Release();
	pdrgpi->Release();

	return GPOS_OK;
}


//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::EresUnittest_CConstraintIntervalFromArrayExprIncludesNull
//
//	@doc:
//		Tests CConstraintInterval::PcnstrIntervalFromScalarArrayCmp in cases
//		where NULL is in the scalar array expression
//
//---------------------------------------------------------------------------
GPOS_RESULT
EresUnittest_CConstraintIntervalFromArrayExprIncludesNull()
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	// setup a file-based provider
	CMDProviderMemory *pmdp = CTestUtils::m_pmdpf;
	pmdp->AddRef();
	CMDAccessor mda(memory_pool, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	CConstExprEvaluatorForDates *pceeval = GPOS_NEW(memory_pool) CConstExprEvaluatorForDates(memory_pool);

	// install opt context in TLS
	CAutoOptCtxt aoc(memory_pool, &mda, pceeval, CTestUtils::Pcm(memory_pool));
	GPOS_ASSERT(NULL != COptCtxt::PoctxtFromTLS()->Pcomp());

	CAutoTraceFlag atf(EopttraceArrayConstraints, true);

	// test for includes NULL
	// create an IN expression with repeated values
	IntPtrArray *pdrgpi = GPOS_NEW(memory_pool) IntPtrArray(memory_pool);
	INT rngiValues[] = {1,2};
	ULONG ulValsLength = GPOS_ARRAY_SIZE(rngiValues);
	for (ULONG ul = 0; ul < ulValsLength; ul++)
	{
		pdrgpi->Append(GPOS_NEW(memory_pool) INT(rngiValues[ul]));
	}
	CExpression *pexprIn =
		CTestUtils::PexprLogicalSelectArrayCmp(memory_pool, CScalarArrayCmp::EarrcmpAny, IMDType::EcmptEq, pdrgpi);

	CExpression *pexprArrayChild = (*(*pexprIn)[1])[1];
	// create a int4 datum
	const IMDTypeInt4 *pmdtypeint4 = mda.PtMDType<IMDTypeInt4>();
	IDatumInt4 *pdatumNull =  pmdtypeint4->PdatumInt4(memory_pool, 0, true);

	CExpression *pexprConstNull =
		GPOS_NEW(memory_pool) CExpression(memory_pool, GPOS_NEW(memory_pool) CScalarConst(memory_pool, (IDatum*) pdatumNull));
	pexprArrayChild->PdrgPexpr()->Append(pexprConstNull);

	CColRef *pcr = CDrvdPropRelational::Pdprel(pexprIn->PdpDerive())->PcrsOutput()->PcrAny();
	CConstraintInterval *pci = CConstraintInterval::PciIntervalFromScalarExpr(memory_pool, (*pexprIn)[1], pcr);
	GPOS_RTL_ASSERT(pci->FIncludesNull());
	GPOS_RTL_ASSERT(2 == pci->Pdrgprng()->Size());
	pexprIn->Release();
	pci->Release();
	pdrgpi->Release();

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::EresUnittest_CIntervalFromScalarCmp
//
//	@doc:
//		Interval from scalar comparison
//
//---------------------------------------------------------------------------
GPOS_RESULT
CConstraintTest::EresUnittest_CIntervalFromScalarCmp
	(
	IMemoryPool *memory_pool,
	CMDAccessor *pmda,
	CColRef *pcr
	)
{
	IMDType::ECmpType rgecmpt[] =
			{
			IMDType::EcmptEq,
			IMDType::EcmptNEq,
			IMDType::EcmptL,
			IMDType::EcmptLEq,
			IMDType::EcmptG,
			IMDType::EcmptGEq,
			};

	for (ULONG ul = 0; ul < GPOS_ARRAY_SIZE(rgecmpt); ul++)
	{
		CExpression *pexprScCmp = PexprScalarCmp(memory_pool, pmda, pcr, rgecmpt[ul], 4);
		CConstraintInterval *pci = CConstraintInterval::PciIntervalFromScalarExpr(memory_pool, pexprScCmp, pcr);
		PrintConstraint(memory_pool, pci);

		pci->Release();
		pexprScCmp->Release();
	}

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::EresUnittest_CIntervalFromScalarBoolOp
//
//	@doc:
//		Interval from scalar bool op
//
//---------------------------------------------------------------------------
GPOS_RESULT
CConstraintTest::EresUnittest_CIntervalFromScalarBoolOp
	(
	IMemoryPool *memory_pool,
	CMDAccessor *pmda,
	CColRef *pcr
	)
{
	// AND
	DrgPexpr *pdrgpexpr = GPOS_NEW(memory_pool) DrgPexpr(memory_pool);
	pdrgpexpr->Append(PexprScalarCmp(memory_pool, pmda, pcr, IMDType::EcmptL, 5));
	pdrgpexpr->Append(PexprScalarCmp(memory_pool, pmda, pcr, IMDType::EcmptGEq, 0));

	CExpression *pexpr = CUtils::PexprScalarBoolOp(memory_pool, CScalarBoolOp::EboolopAnd, pdrgpexpr);
	(void) pexpr->PdpDerive();

	CConstraintInterval *pciAnd = CConstraintInterval::PciIntervalFromScalarExpr(memory_pool, pexpr, pcr);
	PrintConstraint(memory_pool, pciAnd);

	pciAnd->Release();
	(void) pexpr->Release();

	// OR
	pdrgpexpr = GPOS_NEW(memory_pool) DrgPexpr(memory_pool);
	pdrgpexpr->Append(PexprScalarCmp(memory_pool, pmda, pcr, IMDType::EcmptL, 5));
	pdrgpexpr->Append(PexprScalarCmp(memory_pool, pmda, pcr, IMDType::EcmptGEq, 10));

	pexpr = CUtils::PexprScalarBoolOp(memory_pool, CScalarBoolOp::EboolopOr, pdrgpexpr);
	(void) pexpr->PdpDerive();

	CConstraintInterval *pciOr = CConstraintInterval::PciIntervalFromScalarExpr(memory_pool, pexpr, pcr);
	PrintConstraint(memory_pool, pciOr);

	pciOr->Release();
	pexpr->Release();

	// NOT
	pdrgpexpr = GPOS_NEW(memory_pool) DrgPexpr(memory_pool);
	pdrgpexpr->Append(CUtils::PexprIsNull(memory_pool, CUtils::PexprScalarIdent(memory_pool, pcr)));

	pexpr = CUtils::PexprScalarBoolOp(memory_pool, CScalarBoolOp::EboolopNot, pdrgpexpr);
	(void) pexpr->PdpDerive();

	CConstraintInterval *pciNot = CConstraintInterval::PciIntervalFromScalarExpr(memory_pool, pexpr, pcr);
	PrintConstraint(memory_pool, pciNot);

	pciNot->Release();
	pexpr->Release();

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::PexprScalarCmp
//
//	@doc:
//		Generate comparison expression
//
//---------------------------------------------------------------------------
CExpression *
CConstraintTest::PexprScalarCmp
	(
	IMemoryPool *memory_pool,
	CMDAccessor *pmda,
	CColRef *pcr,
	IMDType::ECmpType ecmpt,
	LINT lVal
	)
{
	CExpression *pexprConst = CUtils::PexprScalarConstInt8(memory_pool, lVal);

	const IMDTypeInt8 *pmdtypeint8 = pmda->PtMDType<IMDTypeInt8>();
	IMDId *pmdidOp = pmdtypeint8->PmdidCmp(ecmpt);
	pmdidOp->AddRef();

	const CMDName mdname = pmda->Pmdscop(pmdidOp)->Mdname();

	CWStringConst strOpName(mdname.Pstr()->GetBuffer());

	CExpression *pexpr = CUtils::PexprScalarCmp(memory_pool, pcr, pexprConst, strOpName, pmdidOp);
	(void) pexpr->PdpDerive();
	return pexpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::PciFirstInterval
//
//	@doc:
//		Create an interval
//
//---------------------------------------------------------------------------
CConstraintInterval *
CConstraintTest::PciFirstInterval
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid,
	CColRef *pcr
	)
{
	const SRangeInfo rgRangeInfo[] =
			{
				{CRange::EriExcluded, -1000, CRange::EriIncluded, -20},
				{CRange::EriExcluded, -15, CRange::EriExcluded, -5},
				{CRange::EriIncluded, 0, CRange::EriExcluded, 5},
				{CRange::EriIncluded, 10, CRange::EriIncluded, 10},
				{CRange::EriExcluded, 20, CRange::EriExcluded, 1000},
			};

	DrgPrng *pdrgprng = Pdrgprng(memory_pool, pmdid, rgRangeInfo, GPOS_ARRAY_SIZE(rgRangeInfo));

	return GPOS_NEW(memory_pool) CConstraintInterval(memory_pool, pcr, pdrgprng, true /*fIsNull*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::PciSecondInterval
//
//	@doc:
//		Create an interval
//
//---------------------------------------------------------------------------
CConstraintInterval *
CConstraintTest::PciSecondInterval
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid,
	CColRef *pcr
	)
{
	const SRangeInfo rgRangeInfo[] =
			{
				{CRange::EriExcluded, -30, CRange::EriExcluded, 1},
				{CRange::EriIncluded, 3, CRange::EriIncluded, 3},
				{CRange::EriExcluded, 10, CRange::EriExcluded, 25},
			};

	DrgPrng *pdrgprng = Pdrgprng(memory_pool, pmdid, rgRangeInfo, GPOS_ARRAY_SIZE(rgRangeInfo));

	return GPOS_NEW(memory_pool) CConstraintInterval(memory_pool, pcr, pdrgprng, false /*fIsNull*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::Pdrgprng
//
//	@doc:
//		Construct an array of ranges to be used to create an interval
//
//---------------------------------------------------------------------------
DrgPrng *
CConstraintTest::Pdrgprng
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid,
	const SRangeInfo rgRangeInfo[],
	ULONG ulRanges
	)
{
	DrgPrng *pdrgprng = GPOS_NEW(memory_pool) DrgPrng(memory_pool);

	for (ULONG ul = 0; ul < ulRanges; ul++)
	{
		SRangeInfo rnginfo = rgRangeInfo[ul];
		pmdid->AddRef();
		CRange *prange = GPOS_NEW(memory_pool) CRange
									(
									pmdid,
									COptCtxt::PoctxtFromTLS()->Pcomp(),
									GPOS_NEW(memory_pool) CDatumInt8GPDB(CTestUtils::m_sysidDefault, (LINT) rnginfo.iLeft),
									rnginfo.eriLeft,
									GPOS_NEW(memory_pool) CDatumInt8GPDB(CTestUtils::m_sysidDefault, (LINT) rnginfo.iRight),
									rnginfo.eriRight
									);
		pdrgprng->Append(prange);
	}

	return pdrgprng;
}


//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::PrintConstraint
//
//	@doc:
//		Print a constraint and its expression representation
//
//---------------------------------------------------------------------------
void
CConstraintTest::PrintConstraint
	(
	IMemoryPool *memory_pool,
	CConstraint *pcnstr
	)
{
	CExpression *pexpr = pcnstr->PexprScalar(memory_pool);

	// debug print
	CAutoTrace at(memory_pool);
	at.Os() << std::endl;
	at.Os() << "CONSTRAINT:" << std::endl << *pcnstr << std::endl << "EXPR:" << std::endl << *pexpr << std::endl;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::PrintEquivClasses
//
//	@doc:
//		Print equivalent classes
//
//---------------------------------------------------------------------------
void
CConstraintTest::PrintEquivClasses
	(
	IMemoryPool *memory_pool,
	DrgPcrs *pdrgpcrs,
	BOOL fExpected
	)
{
	// debug print
	CAutoTrace at(memory_pool);
	at.Os() << std::endl;
	if (fExpected)
	{
		at.Os() << "EXPECTED ";
	}
	else
	{
		at.Os() << "ACTUAL ";
	}
	at.Os() << "EQUIVALENCE CLASSES: [ ";

	if (NULL == pdrgpcrs)
	{
		at.Os() << "]" << std::endl;

		return;
	}

	for (ULONG ul = 0; ul < pdrgpcrs->Size(); ul++)
	{
		at.Os() << "{" << *(*pdrgpcrs)[ul] << "} ";
	}

	at.Os() << "]" << std::endl;
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::EresUnittest_NegativeTests
//
//	@doc:
//		Tests for unconstrainable types.
//
//---------------------------------------------------------------------------
GPOS_RESULT
CConstraintTest::EresUnittest_NegativeTests()
{
	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	// setup a file-based provider
	CMDProviderMemory *pmdp = CTestUtils::m_pmdpf;
	pmdp->AddRef();

	// we need to use an auto pointer for the cache here to ensure
	// deleting memory of cached objects when we throw
	CAutoP<CMDAccessor::MDCache> apcache;
	apcache = CCacheFactory::CreateCache<gpopt::IMDCacheObject*, gpopt::CMDKey*>
				(
				true, // fUnique
				0 /* unlimited cache quota */,
				CMDKey::UlHashMDKey,
				CMDKey::FEqualMDKey
				);

	CMDAccessor::MDCache *pcache = apcache.Value();

	CMDAccessor mda(memory_pool, pcache, CTestUtils::m_sysidDefault, pmdp);

	CConstExprEvaluatorForDates *pceeval = GPOS_NEW(memory_pool) CConstExprEvaluatorForDates(memory_pool);

	// install opt context in TLS
	CAutoOptCtxt aoc(memory_pool, &mda, pceeval, CTestUtils::Pcm(memory_pool));
	GPOS_ASSERT(NULL != COptCtxt::PoctxtFromTLS()->Pcomp());

	const IMDType *pmdtype = mda.Pmdtype(&CMDIdGPDB::m_mdidText);
	CWStringConst str(GPOS_WSZ_LIT("text_col"));
	CName name(memory_pool, &str);
	CAutoP<CColRef> pcr(COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(pmdtype, IDefaultTypeModifier, name));

	// create a text interval: ['baz', 'foobar')
	CAutoP<CWStringDynamic> pstrLower1(GPOS_NEW(memory_pool) CWStringDynamic(memory_pool, GPOS_WSZ_LIT("AAAAB2Jheg==")));
	CAutoP<CWStringDynamic> pstrUpper1(GPOS_NEW(memory_pool) CWStringDynamic(memory_pool, GPOS_WSZ_LIT("AAAACmZvb2Jhcg==")));
	const LINT lLower1 = 571163436;
	const LINT lUpper1 = 322061118;

	// 'text' is not a constrainable type, so the interval construction should assert-fail
	CConstraintInterval *pciFirst =
					CTestUtils::PciGenericInterval
						(
						memory_pool,
						&mda,
						CMDIdGPDB::m_mdidText,
						pcr.Value(),
						pstrLower1.Value(),
						lLower1,
						CRange::EriIncluded,
						pstrUpper1.Value(),
						lUpper1,
						CRange::EriExcluded
						);
	PrintConstraint(memory_pool, pciFirst);
	pciFirst->Release();

	return GPOS_OK;
}
#endif // GPOS_DEBUG

//---------------------------------------------------------------------------
//	@function:
//		CConstraintTest::EresUnittest_ConstraintsOnDates
//	@doc:
//		Test constraints on date intervals.
//
//---------------------------------------------------------------------------
GPOS_RESULT
CConstraintTest::EresUnittest_ConstraintsOnDates()
{
	CAutoTraceFlag atf(EopttraceEnableConstantExpressionEvaluation, true /*value*/);

	// create memory pool
	CAutoMemoryPool amp;
	IMemoryPool *memory_pool = amp.Pmp();

	// setup a file-based provider
	CMDProviderMemory *pmdp = CTestUtils::m_pmdpf;
	pmdp->AddRef();
	CMDAccessor mda(memory_pool, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	CConstExprEvaluatorForDates *pceeval = GPOS_NEW(memory_pool) CConstExprEvaluatorForDates(memory_pool);

	// install opt context in TLS
	CAutoOptCtxt aoc(memory_pool, &mda, pceeval, CTestUtils::Pcm(memory_pool));
	GPOS_ASSERT(NULL != COptCtxt::PoctxtFromTLS()->Pcomp());

	const IMDType *pmdtype = mda.Pmdtype(&CMDIdGPDB::m_mdidDate);
	CWStringConst str(GPOS_WSZ_LIT("date_col"));
	CName name(memory_pool, &str);
	CAutoP<CColRef> pcr(COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(pmdtype, IDefaultTypeModifier, name));

	// create a date interval: ['01-01-2012', '01-21-2012')
	CWStringDynamic pstrLowerDate1(memory_pool, wszInternalRepresentationFor2012_01_01);
	CWStringDynamic pstrUpperDate1(memory_pool, wszInternalRepresentationFor2012_01_21);
	CConstraintInterval *pciFirst =
			CTestUtils::PciGenericInterval
				(
				memory_pool,
				&mda,
				CMDIdGPDB::m_mdidDate,
				pcr.Value(),
				&pstrLowerDate1,
				lInternalRepresentationFor2012_01_01,
				CRange::EriIncluded,
				&pstrUpperDate1,
				lInternalRepresentationFor2012_01_21,
				CRange::EriExcluded
				);
	PrintConstraint(memory_pool, pciFirst);

	// create a date interval: ['01-02-2012', '01-22-2012')
	CWStringDynamic pstrLowerDate2(memory_pool, wszInternalRepresentationFor2012_01_02);
	CWStringDynamic pstrUpperDate2(memory_pool, wszInternalRepresentationFor2012_01_22);
	CConstraintInterval *pciSecond =
			CTestUtils::PciGenericInterval
				(
				memory_pool,
				&mda,
				CMDIdGPDB::m_mdidDate,
				pcr.Value(),
				&pstrLowerDate2,
				lInternalRepresentationFor2012_01_02,
				CRange::EriIncluded,
				&pstrUpperDate2,
				lInternalRepresentationFor2012_01_22,
				CRange::EriExcluded
				);
	PrintConstraint(memory_pool, pciSecond);

	// intersection
	CConstraintInterval *pciIntersect = pciFirst->PciIntersect(memory_pool, pciSecond);
	PrintConstraint(memory_pool, pciIntersect);
	CConstraintInterval *pciIntersectExpected =
			CTestUtils::PciGenericInterval
				(
				memory_pool,
				&mda,
				CMDIdGPDB::m_mdidDate,
				pcr.Value(),
				&pstrLowerDate2,
				lInternalRepresentationFor2012_01_02,
				CRange::EriIncluded,
				&pstrUpperDate1,
				lInternalRepresentationFor2012_01_21,
				CRange::EriExcluded
				);
	GPOS_ASSERT(pciIntersectExpected->Equals(pciIntersect));

	// union
	CConstraintInterval *pciUnion = pciFirst->PciUnion(memory_pool, pciSecond);
	PrintConstraint(memory_pool, pciUnion);
	CConstraintInterval *pciUnionExpected =
			CTestUtils::PciGenericInterval
				(
				memory_pool,
				&mda,
				CMDIdGPDB::m_mdidDate,
				pcr.Value(),
				&pstrLowerDate1,
				lInternalRepresentationFor2012_01_01,
				CRange::EriIncluded,
				&pstrUpperDate2,
				lInternalRepresentationFor2012_01_22,
				CRange::EriExcluded
				);
	GPOS_ASSERT(pciUnionExpected->Equals(pciUnion));

	// difference between pciFirst and pciSecond
	CConstraintInterval *pciDiff1 = pciFirst->PciDifference(memory_pool, pciSecond);
	PrintConstraint(memory_pool, pciDiff1);
	CConstraintInterval *pciDiff1Expected =
			CTestUtils::PciGenericInterval
				(
				memory_pool,
				&mda,
				CMDIdGPDB::m_mdidDate,
				pcr.Value(),
				&pstrLowerDate1,
				lInternalRepresentationFor2012_01_01,
				CRange::EriIncluded,
				&pstrLowerDate2,
				lInternalRepresentationFor2012_01_02,
				CRange::EriExcluded
				);
	GPOS_ASSERT(pciDiff1Expected->Equals(pciDiff1));

	// difference between pciSecond and pciFirst
	CConstraintInterval *pciDiff2 = pciSecond->PciDifference(memory_pool, pciFirst);
	PrintConstraint(memory_pool, pciDiff2);
	CConstraintInterval *pciDiff2Expected =
			CTestUtils::PciGenericInterval
				(
				memory_pool,
				&mda,
				CMDIdGPDB::m_mdidDate,
				pcr.Value(),
				&pstrUpperDate1,
				lInternalRepresentationFor2012_01_21,
				CRange::EriIncluded,
				&pstrUpperDate2,
				lInternalRepresentationFor2012_01_22,
				CRange::EriExcluded
				);
	GPOS_ASSERT(pciDiff2Expected->Equals(pciDiff2));

	// containment
	GPOS_ASSERT(!pciFirst->FContains(pciSecond));
	GPOS_ASSERT(pciFirst->FContains(pciDiff1));
	GPOS_ASSERT(!pciSecond->FContains(pciFirst));
	GPOS_ASSERT(pciSecond->FContains(pciDiff2));
	GPOS_ASSERT(pciFirst->FContains(pciFirst));
	GPOS_ASSERT(pciSecond->FContains(pciSecond));

	// equality
	// create a third interval identical to the first
	CConstraintInterval *pciThird =
			CTestUtils::PciGenericInterval
				(
				memory_pool,
				&mda,
				CMDIdGPDB::m_mdidDate,
				pcr.Value(),
				&pstrLowerDate1,
				lInternalRepresentationFor2012_01_01,
				CRange::EriIncluded,
				&pstrUpperDate1,
				lInternalRepresentationFor2012_01_21,
				CRange::EriExcluded
				);
	GPOS_ASSERT(!pciFirst->Equals(pciSecond));
	GPOS_ASSERT(!pciFirst->Equals(pciDiff1));
	GPOS_ASSERT(!pciSecond->Equals(pciDiff2));
	GPOS_ASSERT(pciFirst->Equals(pciThird));

	pciThird->Release();
	pciDiff2Expected->Release();
	pciDiff1Expected->Release();
	pciUnionExpected->Release();
	pciIntersectExpected->Release();
	pciDiff2->Release();
	pciDiff1->Release();
	pciUnion->Release();
	pciIntersect->Release();
	pciSecond->Release();
	pciFirst->Release();

	return GPOS_OK;
}

// EOF
