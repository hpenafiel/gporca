//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright 2014 Pivotal Inc.
//
//	@filename:
//		CStatsPredUtils.cpp
//
//	@doc:
//		Statistics predicate helper routines
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/base/CCastUtils.h"
#include "gpopt/exception.h"
#include "gpopt/operators/ops.h"
#include "gpopt/operators/CExpressionUtils.h"
#include "gpopt/operators/CPredicateUtils.h"

#include "naucrates/statistics/CStatsPredUtils.h"
#include "naucrates/statistics/CStatisticsUtils.h"
#include "naucrates/statistics/CStatsPredLike.h"
#include "naucrates/statistics/CHistogram.h"

#include "gpopt/mdcache/CMDAccessor.h"

#include "naucrates/md/IMDScalarOp.h"
#include "naucrates/md/IMDType.h"
#include "naucrates/statistics/CStatistics.h"
#include "naucrates/statistics/CStatsPredDisj.h"
#include "naucrates/statistics/CStatsPredConj.h"

using namespace gpopt;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::Estatscmpt
//
//	@doc:
//		For the purpose of statistics computation, what is the comparison
//		type of an operator. Note that this has different, looser semantics
//		than CUtils::ParseCmpType
//
//---------------------------------------------------------------------------
CStatsPred::EStatsCmpType
CStatsPredUtils::Estatscmpt
	(
	const CWStringConst *str_opname
	)
{
	GPOS_ASSERT(NULL != str_opname);

	CStatsPred::EStatsCmpType escmpt = CStatsPred::EstatscmptOther;

	CWStringConst pstrL(GPOS_WSZ_LIT("<"));
	CWStringConst pstrLEq(GPOS_WSZ_LIT("<="));
	CWStringConst pstrEq(GPOS_WSZ_LIT("="));
	CWStringConst pstrGEq(GPOS_WSZ_LIT(">="));
	CWStringConst pstrG(GPOS_WSZ_LIT(">"));
	CWStringConst pstrNEq(GPOS_WSZ_LIT("<>"));

	if (str_opname->Equals(&pstrL))
	{
		escmpt = CStatsPred::EstatscmptL;
	}
	if (str_opname->Equals(&pstrLEq))
	{
		escmpt = CStatsPred::EstatscmptLEq;
	}
	if (str_opname->Equals(&pstrEq))
	{
		escmpt = CStatsPred::EstatscmptEq;
	}
	if (str_opname->Equals(&pstrGEq))
	{
		escmpt = CStatsPred::EstatscmptGEq;
	}
	if (str_opname->Equals(&pstrG))
	{
		escmpt = CStatsPred::EstatscmptG;
	}
	if (str_opname->Equals(&pstrNEq))
	{
		escmpt = CStatsPred::EstatscmptNEq;
	}

	return escmpt;
}

//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::Estatscmpt
//
//	@doc:
//		For the purpose of statistics computation, what is the comparison
//		type of an operator. Note that this has different, looser semantics
//		than CUtils::ParseCmpType
//
//---------------------------------------------------------------------------
CStatsPred::EStatsCmpType
CStatsPredUtils::Estatscmpt
	(
	IMDId *mdid
	)
{
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	const IMDScalarOp *md_scalar_op = md_accessor->Pmdscop(mdid);

	// Simply go by operator name.
	// If the name of the operator is "<", then it is a LessThan etc.
	const CWStringConst *str_opname = md_scalar_op->Mdname().GetMDName();

	return Estatscmpt(str_opname);
}

//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::PstatspredUnsupported
//
//	@doc:
//		Create an unsupported statistics predicate
//---------------------------------------------------------------------------
CStatsPred *
CStatsPredUtils::PstatspredUnsupported
	(
	IMemoryPool *memory_pool,
	CExpression *, // pexprPred,
	CColRefSet * //pcrsOuterRefs
	)
{
	return GPOS_NEW(memory_pool) CStatsPredUnsupported(ULONG_MAX, CStatsPred::EstatscmptOther);
}

//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::PstatspredNullTest
//
//	@doc:
//		Extract statistics filtering information from a null test
//---------------------------------------------------------------------------
CStatsPred *
CStatsPredUtils::PstatspredNullTest
	(
	IMemoryPool *memory_pool,
	CExpression *pexprPred,
	CColRefSet * //pcrsOuterRefs
	)
{
	GPOS_ASSERT(NULL != pexprPred);
	GPOS_ASSERT(FScalarIdentIsNull(pexprPred) || FScalarIdentIsNotNull(pexprPred));

	CExpression *pexprNullTest = pexprPred;
	CStatsPred::EStatsCmpType escmpt = CStatsPred::EstatscmptEq; // 'is null'

	if (FScalarIdentIsNotNull(pexprPred))
	{
		pexprNullTest = (*pexprPred)[0];
		escmpt = CStatsPred::EstatscmptNEq; // 'is not null'
	}

	CScalarIdent *popScalarIdent = CScalarIdent::PopConvert((*pexprNullTest)[0]->Pop());
	const CColRef *pcr = popScalarIdent->Pcr();

	IDatum *pdatum = CStatisticsUtils::PdatumNull(pcr);
	if (!pdatum->FStatsComparable(pdatum))
	{
		// stats calculations on such datums unsupported
		pdatum->Release();

		return GPOS_NEW(memory_pool) CStatsPredUnsupported(pcr->Id(), escmpt);
	}

	CPoint *ppoint = GPOS_NEW(memory_pool) CPoint(pdatum);
	CStatsPredPoint *pstatspred = GPOS_NEW(memory_pool) CStatsPredPoint(pcr->Id(), escmpt, ppoint);

	return pstatspred;
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::PstatspredPoint
//
//	@doc:
//		Extract statistics filtering information from a point comparison
//---------------------------------------------------------------------------
CStatsPred *
CStatsPredUtils::PstatspredPoint
	(
	IMemoryPool *memory_pool,
	CExpression *pexprPred,
	CColRefSet *//pcrsOuterRefs,
	)
{
	GPOS_ASSERT(NULL != pexprPred);

	CStatsPred *pstatspred = Pstatspred(memory_pool, pexprPred);
	GPOS_ASSERT (NULL != pstatspred);

	return pstatspred;
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::Estatscmptype
//
//	@doc:
// 		Return the statistics predicate comparison type
//---------------------------------------------------------------------------
CStatsPred::EStatsCmpType
CStatsPredUtils::Estatscmptype
	(
	IMDId *mdid
	)
{
	GPOS_ASSERT(NULL != mdid);
	CStatsPred::EStatsCmpType escmpt = Estatscmpt(mdid);

	if (CStatsPred::EstatscmptOther != escmpt)
	{
		return escmpt;
	}

	if (CPredicateUtils::FLikePredicate(mdid))
	{
		return CStatsPred::EstatscmptLike;
	}

	return CStatsPred::EstatscmptOther;
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::Pstatspred
//
//	@doc:
// 		Generate a statistics point predicate for expressions that are supported.
//		Else an unsupported predicate. Note that the comparison semantics extracted are
//		for statistics computation only.
//---------------------------------------------------------------------------
CStatsPred *
CStatsPredUtils::Pstatspred
	(
	IMemoryPool *memory_pool,
	CExpression *pexpr
	)
{
	GPOS_ASSERT(NULL != pexpr);

	CExpression *pexprLeft = NULL;
	CExpression *pexprRight = NULL;

	CStatsPred::EStatsCmpType escmpt = CStatsPred::EstatscmptOther;
	if (CPredicateUtils::FIdentIDFConstIgnoreCast(pexpr))
	{
		pexprLeft = (*pexpr)[0];
		pexprRight = (*pexpr)[1];

		escmpt = CStatsPred::EstatscmptIDF;
	}
	else if (CPredicateUtils::FIdentINDFConstIgnoreCast(pexpr))
	{
		CExpression *pexprChild = (*pexpr)[0];
		pexprLeft = (*pexprChild)[0];
		pexprRight = (*pexprChild)[1];

		escmpt = CStatsPred::EstatscmptINDF;
	}
	else
	{
		pexprLeft = (*pexpr)[0];
		pexprRight = (*pexpr)[1];

		GPOS_ASSERT(CPredicateUtils::FIdentCompareConstIgnoreCast(pexpr, COperator::EopScalarCmp));

		COperator *pop = pexpr->Pop();
		CScalarCmp *popScCmp = CScalarCmp::PopConvert(pop);

		// Comparison semantics for statistics purposes is looser
		// than regular comparison
		escmpt = Estatscmptype(popScCmp->MdIdOp());
	}

	GPOS_ASSERT(COperator::EopScalarIdent == pexprLeft->Pop()->Eopid() || CScalarIdent::FCastedScId(pexprLeft));
	GPOS_ASSERT(COperator::EopScalarConst == pexprRight->Pop()->Eopid() || CScalarConst::FCastedConst(pexprRight));

	const CColRef *pcr = CCastUtils::PcrExtractFromScIdOrCastScId(pexprLeft);
	CScalarConst *popScalarConst = CScalarConst::PopExtractFromConstOrCastConst(pexprRight);
	GPOS_ASSERT(NULL != popScalarConst);

	IDatum *pdatum = popScalarConst->Pdatum();
	if (!CHistogram::FSupportsFilter(escmpt) || !IMDType::FStatsComparable(pcr->Pmdtype(), pdatum))
	{
		// case 1: unsupported predicate for stats calculations
		// example: SELECT 1 FROM pg_catalog.pg_class c WHERE c.relname ~ '^(t36)$';
		// case 2: unsupported stats comparison between the column and datum

		return GPOS_NEW(memory_pool) CStatsPredUnsupported(pcr->Id(), escmpt);
	}

	return GPOS_NEW(memory_pool) CStatsPredPoint(memory_pool, pcr, escmpt, pdatum);
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::FCmpColsIgnoreCast
//
//	@doc:
// 		Is the expression a comparison of scalar ident or cast of a scalar ident?
//		Extract relevant info.
//
//---------------------------------------------------------------------------
BOOL
CStatsPredUtils::FCmpColsIgnoreCast
	(
	CExpression *pexpr,
	const CColRef **ppcrLeft,
	CStatsPred::EStatsCmpType *pescmpt,
	const CColRef **ppcrRight
	)
{
	GPOS_ASSERT(NULL != ppcrLeft);
	GPOS_ASSERT(NULL != ppcrRight);
	COperator *pop = pexpr->Pop();

	BOOL fINDF = CPredicateUtils::FINDF(pexpr);
	BOOL fIDF = CPredicateUtils::FIDF(pexpr);
	BOOL fScalarCmp = (COperator::EopScalarCmp == pop->Eopid());
	if (!fScalarCmp && !fINDF && !fIDF)
	{
		return false;
	}

	CExpression *pexprLeft = NULL;
	CExpression *pexprRight = NULL;

	if (fINDF)
	{
		(*pescmpt) = CStatsPred::EstatscmptINDF;
		CExpression *pexprIDF = (*pexpr)[0];
		pexprLeft = (*pexprIDF)[0];
		pexprRight = (*pexprIDF)[1];
	}
	else if (fIDF)
	{
		(*pescmpt) = CStatsPred::EstatscmptIDF;
		pexprLeft = (*pexpr)[0];
		pexprRight = (*pexpr)[1];
	}
	else
	{
		GPOS_ASSERT(fScalarCmp);

		CScalarCmp *popScCmp = CScalarCmp::PopConvert(pop);

		// Comparison semantics for stats purposes is looser
		// than regular comparison.
		(*pescmpt) = CStatsPredUtils::Estatscmpt(popScCmp->MdIdOp());

		pexprLeft = (*pexpr)[0];
		pexprRight = (*pexpr)[1];
	}

	(*ppcrLeft) = CCastUtils::PcrExtractFromScIdOrCastScId(pexprLeft);
	(*ppcrRight) = CCastUtils::PcrExtractFromScIdOrCastScId(pexprRight);

	if (NULL == *ppcrLeft || NULL == *ppcrRight)
	{
		// if the scalar cmp is of equality type, we may not have been able to extract
		// the column referenes of scalar ident if they had any other expression than cast
		// on top of them.
		// in such cases, check if there is still a possibility to extract scalar ident,
		// if there is more than one column reference on either side, this is unsupported
		// If supported, mark the comparison as NDV-based
		if (*pescmpt == CStatsPred::EstatscmptEq)
		{
			(*ppcrLeft) = CUtils::PcrExtractFromScExpression(pexprLeft);
			(*ppcrRight) = CUtils::PcrExtractFromScExpression(pexprRight);
			
			if (NULL == *ppcrLeft || NULL == *ppcrRight)
			{
				return false;
			}
			(*pescmpt) = CStatsPred::EstatscmptEqNDV;
			return true;
		}
		// failed to extract a scalar ident
		return false;
	}

	return true;
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::PstatspredExtract
//
//	@doc:
//		Extract scalar expression for statistics filtering
//
//---------------------------------------------------------------------------
CStatsPred *
CStatsPredUtils::PstatspredExtract
	(
	IMemoryPool *memory_pool,
	CExpression *pexprScalar,
	CColRefSet *pcrsOuterRefs
	)
{
	GPOS_ASSERT(NULL != pexprScalar);
	if (CPredicateUtils::FOr(pexprScalar))
	{
		CStatsPred *pstatspredDisj = PstatspredDisj(memory_pool, pexprScalar, pcrsOuterRefs);
		if (NULL != pstatspredDisj)
		{
			return pstatspredDisj;
		}
	}
	else
	{
		CStatsPred *pstatspredConj = PstatspredConj(memory_pool, pexprScalar, pcrsOuterRefs);
		if (NULL != pstatspredConj)
		{
			return pstatspredConj;
		}
	}

	return GPOS_NEW(memory_pool) CStatsPredConj(GPOS_NEW(memory_pool) DrgPstatspred(memory_pool));
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::PstatspredConj
//
//	@doc:
//		Create conjunctive statistics filter composed of the extracted
//		components of the conjunction
//---------------------------------------------------------------------------
CStatsPred *
CStatsPredUtils::PstatspredConj
	(
	IMemoryPool *memory_pool,
	CExpression *pexprScalar,
	CColRefSet *pcrsOuterRefs
	)
{
	GPOS_ASSERT(NULL != pexprScalar);
	DrgPexpr *pdrgpexprConjuncts = CPredicateUtils::PdrgpexprConjuncts(memory_pool, pexprScalar);
	const ULONG ulLen = pdrgpexprConjuncts->Size();

	DrgPstatspred *pdrgpstatspred = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		CExpression *pexprPred = (*pdrgpexprConjuncts)[ul];
		CColRefSet *pcrsUsed = CDrvdPropScalar::Pdpscalar(pexprPred->PdpDerive())->PcrsUsed();
		if (NULL != pcrsOuterRefs && pcrsOuterRefs->ContainsAll(pcrsUsed))
		{
			// skip predicate with outer references
			continue;
		}

		if (CPredicateUtils::FOr(pexprPred))
		{
			CStatsPred *pstatspredDisj = PstatspredDisj(memory_pool, pexprPred, pcrsOuterRefs);
			if (NULL != pstatspredDisj)
			{
				pdrgpstatspred->Append(pstatspredDisj);
			}
		}
		else
		{
			AddSupportedStatsFilters(memory_pool, pdrgpstatspred, pexprPred, pcrsOuterRefs);
		}
	}

	pdrgpexprConjuncts->Release();

	if (0 < pdrgpstatspred->Size())
	{
		return GPOS_NEW(memory_pool) CStatsPredConj(pdrgpstatspred);
	}

	pdrgpstatspred->Release();

	return NULL;
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::PstatspredDisj
//
//	@doc:
//		Create disjunctive statistics filter composed of the extracted
//		components of the disjunction
//---------------------------------------------------------------------------
CStatsPred *
CStatsPredUtils::PstatspredDisj
	(
	IMemoryPool *memory_pool,
	CExpression *pexprPred,
	CColRefSet *pcrsOuterRefs
	)
{
	GPOS_ASSERT(NULL != pexprPred);
	GPOS_ASSERT(CPredicateUtils::FOr(pexprPred));

	DrgPstatspred *pdrgpstatspredDisjChild = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);

	// remove duplicate components of the OR tree
	CExpression *pexprNew = CExpressionUtils::PexprDedupChildren(memory_pool, pexprPred);

	// extract the components of the OR tree
	DrgPexpr *pdrgpexpr = CPredicateUtils::PdrgpexprDisjuncts(memory_pool, pexprNew);
	const ULONG ulLen = pdrgpexpr->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		CExpression *pexpr = (*pdrgpexpr)[ul];
		CColRefSet *pcrsUsed = CDrvdPropScalar::Pdpscalar(pexpr->PdpDerive())->PcrsUsed();
		if (NULL != pcrsOuterRefs && pcrsOuterRefs->ContainsAll(pcrsUsed))
		{
			// skip predicate with outer references
			continue;
		}

		AddSupportedStatsFilters(memory_pool, pdrgpstatspredDisjChild, pexpr, pcrsOuterRefs);
	}

	// clean up
	pexprNew->Release();
	pdrgpexpr->Release();

	if (0 < pdrgpstatspredDisjChild->Size())
	{
		return GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspredDisjChild);
	}

	pdrgpstatspredDisjChild->Release();

	return NULL;
}

//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::AddSupportedStatsFilters
//
//	@doc:
//		Add supported filter for statistics computation
//---------------------------------------------------------------------------
void
CStatsPredUtils::AddSupportedStatsFilters
	(
	IMemoryPool *memory_pool,
	DrgPstatspred *pdrgpstatspred,
	CExpression *pexprPred,
	CColRefSet *pcrsOuterRefs
	)
{
	GPOS_ASSERT(NULL != pexprPred);
	GPOS_ASSERT(NULL != pdrgpstatspred);

	CColRefSet *pcrsUsed = CDrvdPropScalar::Pdpscalar(pexprPred->PdpDerive())->PcrsUsed();
	if (NULL != pcrsOuterRefs && pcrsOuterRefs->ContainsAll(pcrsUsed))
	{
		// skip predicates with outer references
		return;
	}

	if (COperator::EopScalarConst == pexprPred->Pop()->Eopid())
	{
		pdrgpstatspred->Append
							(
							GPOS_NEW(memory_pool) CStatsPredUnsupported
										(
										ULONG_MAX,
										CStatsPred::EstatscmptOther,
										CHistogram::DNeutralScaleFactor
										)
							);

		return;
	}

	if (COperator::EopScalarArrayCmp == pexprPred->Pop()->Eopid())
	{
		ProcessArrayCmp(memory_pool, pexprPred, pdrgpstatspred);
	}
	else
	{
		CStatsPredUtils::EPredicateType ept = Ept(memory_pool, pexprPred);
		GPOS_ASSERT(CStatsPredUtils::EptSentinel != ept);

		// array extract function mapping
		SScStatsfilterMapping rgStatsfilterTranslators[] =
		{
			{CStatsPredUtils::EptDisj, &CStatsPredUtils::PstatspredDisj},
			{CStatsPredUtils::EptScIdent, &CStatsPredUtils::PstatspredBoolean},
			{CStatsPredUtils::EptLike, &CStatsPredUtils::PstatspredLike},
			{CStatsPredUtils::EptPoint, &CStatsPredUtils::PstatspredPoint},
			{CStatsPredUtils::EptConj, &CStatsPredUtils::PstatspredConj},
			{CStatsPredUtils::EptNullTest, &CStatsPredUtils::PstatspredNullTest},
		};

		PfPstatspred *pf = &CStatsPredUtils::PstatspredUnsupported;
		const ULONG translators_mapping_len = GPOS_ARRAY_SIZE(rgStatsfilterTranslators);
		for (ULONG ul = 0; ul < translators_mapping_len; ul++)
		{
			SScStatsfilterMapping elem = rgStatsfilterTranslators[ul];
			if (ept == elem.ept)
			{
				pf = elem.pf;
				break;
			}
		}

		CStatsPred *pstatspred = pf(memory_pool, pexprPred, pcrsOuterRefs);
		if (NULL != pstatspred)
		{
			pdrgpstatspred->Append(pstatspred);
		}
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::Ept
//
//	@doc:
//		Return statistics filter type of the given expression
//---------------------------------------------------------------------------
CStatsPredUtils::EPredicateType
CStatsPredUtils::Ept
	(
	IMemoryPool *memory_pool,
	CExpression *pexprPred
	)
{
	GPOS_ASSERT(NULL != pexprPred);
	if (CPredicateUtils::FOr(pexprPred))
	{
		return CStatsPredUtils::EptDisj;
	}

	if (FBooleanScIdent(pexprPred))
	{
		return CStatsPredUtils::EptScIdent;
	}

	if (CPredicateUtils::FLikePredicate(pexprPred))
	{
		return CStatsPredUtils::EptLike;
	}

	if (FPointPredicate(pexprPred))
	{
		return CStatsPredUtils::EptPoint;
	}

	if (FPointIDF(pexprPred))
	{
		return CStatsPredUtils::EptPoint;
	}

	if (FPointINDF(pexprPred))
	{
		return CStatsPredUtils::EptPoint;
	}

	if (FConjunction(memory_pool, pexprPred))
	{
		return CStatsPredUtils::EptConj;
	}

	if (FScalarIdentIsNull(pexprPred) || FScalarIdentIsNotNull(pexprPred))
	{
		return CStatsPredUtils::EptNullTest;
	}

	return CStatsPredUtils::EptUnsupported;
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::FConjunction
//
//	@doc:
//		Is the condition a conjunctive predicate
//---------------------------------------------------------------------------
BOOL
CStatsPredUtils::FConjunction
	(
	IMemoryPool *memory_pool,
	CExpression *pexprPred
	)
{
	GPOS_ASSERT(NULL != pexprPred);
	DrgPexpr *pdrgpexprConjuncts = CPredicateUtils::PdrgpexprConjuncts(memory_pool, pexprPred);
	const ULONG ulLen = pdrgpexprConjuncts->Size();
	pdrgpexprConjuncts->Release();

	return (1 < ulLen);
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::FBooleanScIdent
//
//	@doc:
//		Is the condition a boolean predicate
//---------------------------------------------------------------------------
BOOL
CStatsPredUtils::FBooleanScIdent
	(
	CExpression *pexprPred
	)
{
	GPOS_ASSERT(NULL != pexprPred);
	return CPredicateUtils::FBooleanScalarIdent(pexprPred) || CPredicateUtils::FNegatedBooleanScalarIdent(pexprPred);
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::FPointPredicate
//
//	@doc:
//		Is the condition a point predicate
//---------------------------------------------------------------------------
BOOL
CStatsPredUtils::FPointPredicate
	(
	CExpression *pexprPred
	)
{
	GPOS_ASSERT(NULL != pexprPred);
	return (CPredicateUtils::FIdentCompareConstIgnoreCast(pexprPred, COperator::EopScalarCmp));
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::FPointIDF
//
//	@doc:
//		Is the condition an IDF point predicate
//---------------------------------------------------------------------------
BOOL
CStatsPredUtils::FPointIDF
	(
	CExpression *pexprPred
	)
{
	GPOS_ASSERT(NULL != pexprPred);
	return CPredicateUtils::FIdentIDFConstIgnoreCast(pexprPred);
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::FPointINDF
//
//	@doc:
//		Is the condition an INDF point predicate
//---------------------------------------------------------------------------
BOOL
CStatsPredUtils::FPointINDF
	(
	CExpression *pexprPred
	)
{
	GPOS_ASSERT(NULL != pexprPred);

	if (!CPredicateUtils::FNot(pexprPred))
	{
		return false;
	}

	return FPointIDF((*pexprPred)[0]);
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::FScalarIdentIsNull
//
//	@doc:
//		Is the condition a 'is null' test on top a scalar ident
//---------------------------------------------------------------------------
BOOL
CStatsPredUtils::FScalarIdentIsNull
	(
	CExpression *pexprPred
	)
{
	GPOS_ASSERT(NULL != pexprPred);

	if (0 == pexprPred->Arity())
	{
		return false;
	}
	// currently we support null test on scalar ident only
	return CUtils::FScalarNullTest(pexprPred) && CUtils::FScalarIdent((*pexprPred)[0]);
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::FScalarIdentNullTest
//
//	@doc:
//		Is the condition a not-null test
//---------------------------------------------------------------------------
BOOL
CStatsPredUtils::FScalarIdentIsNotNull
	(
	CExpression *pexprPred
	)
{
	GPOS_ASSERT(NULL != pexprPred);

	if (0 == pexprPred->Arity())
	{
		return false;
	}
	CExpression *pexprNullTest = (*pexprPred)[0];

	// currently we support not-null test on scalar ident only
	return CUtils::FScalarBoolOp(pexprPred, CScalarBoolOp::EboolopNot) && FScalarIdentIsNull(pexprNullTest);
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::PstatspredLikeHandleCasting
//
//	@doc:
//		Create a LIKE statistics filter
//---------------------------------------------------------------------------
CStatsPred *
CStatsPredUtils::PstatspredLike
	(
	IMemoryPool *memory_pool,
	CExpression *pexprPred,
	CColRefSet *//pcrsOuterRefs,
	)
{
	GPOS_ASSERT(NULL != pexprPred);
	GPOS_ASSERT(CPredicateUtils::FLikePredicate(pexprPred));

	CExpression *pexprLeft = (*pexprPred)[0];
	CExpression *pexprRight = (*pexprPred)[1];

	// we support LIKE predicate of the following patterns
	// CAST(ScIdent) LIKE Const
	// CAST(ScIdent) LIKE CAST(Const)
	// ScIdent LIKE Const
	// ScIdent LIKE CAST(Const)
	// CAST(Const) LIKE ScIdent
	// CAST(Const) LIKE CAST(ScIdent)
	// const LIKE ScIdent
	// const LIKE CAST(ScIdent)

	CExpression *pexprScIdent = NULL;
	CExpression *pexprScConst = NULL;

	CPredicateUtils::ExtractLikePredComponents(pexprPred, &pexprScIdent, &pexprScConst);

	if (NULL == pexprScIdent || NULL == pexprScConst)
	{
		return GPOS_NEW(memory_pool) CStatsPredUnsupported(ULONG_MAX, CStatsPred::EstatscmptLike);
	}

	CScalarIdent *popScalarIdent = CScalarIdent::PopConvert(pexprScIdent->Pop());
	ULONG col_id = popScalarIdent->Pcr()->Id();

	CScalarConst *popScalarConst = CScalarConst::PopConvert(pexprScConst->Pop());
	IDatum  *pdatumLiteral = popScalarConst->Pdatum();

	const CColRef *pcr = popScalarIdent->Pcr();
	if (!IMDType::FStatsComparable(pcr->Pmdtype(), pdatumLiteral))
	{
		// unsupported stats comparison between the column and datum
		return GPOS_NEW(memory_pool) CStatsPredUnsupported(pcr->Id(), CStatsPred::EstatscmptLike);
	}

	CDouble dDefaultScaleFactor(1.0);
	if (pdatumLiteral->FSupportLikePredicate())
	{
		dDefaultScaleFactor = pdatumLiteral->DLikePredicateScaleFactor();
	}

	pexprLeft->AddRef();
	pexprRight->AddRef();

	return GPOS_NEW(memory_pool) CStatsPredLike(col_id, pexprLeft, pexprRight, dDefaultScaleFactor);
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::ProcessArrayCmp
//
//	@doc:
//		Extract statistics filtering information from scalar array comparison
//---------------------------------------------------------------------------
void
CStatsPredUtils::ProcessArrayCmp
	(
	IMemoryPool *memory_pool,
	CExpression *pexprPred,
	DrgPstatspred *pdrgpstatspred
	)
{
	GPOS_ASSERT(NULL != pdrgpstatspred);
	GPOS_ASSERT(NULL != pexprPred);
	GPOS_ASSERT(2 == pexprPred->Arity());
	CExpression *pexprIdent = NULL;
	CScalarArrayCmp *popScArrayCmp = CScalarArrayCmp::PopConvert(pexprPred->Pop());
	if (CUtils::FScalarIdent((*pexprPred)[0]))
	{
		pexprIdent = (*pexprPred)[0];
	}
	else if (CCastUtils::FBinaryCoercibleCast((*pexprPred)[0]))
	{
		pexprIdent = (*(*pexprPred)[0])[0];
	}
	CExpression *pexprArray = CUtils::PexprScalarArrayChild(pexprPred);

	BOOL fCompareToConstAndScalarIdents = CPredicateUtils::FCompareCastIdentToConstArray(pexprPred) ||
										  CPredicateUtils::FCompareScalarIdentToConstAndScalarIdentArray(pexprPred);

	if (!fCompareToConstAndScalarIdents)
	{
		// unsupported predicate for stats calculations
		pdrgpstatspred->Append(GPOS_NEW(memory_pool) CStatsPredUnsupported(ULONG_MAX, CStatsPred::EstatscmptOther));

		return;
	}

	BOOL fAny = (CScalarArrayCmp::EarrcmpAny == popScArrayCmp->Earrcmpt());

	DrgPstatspred *pdrgpstatspredChild = pdrgpstatspred;
	if (fAny)
	{
		pdrgpstatspredChild = GPOS_NEW(memory_pool) DrgPstatspred(memory_pool);
	}

	const ULONG ulConstants = CUtils::UlScalarArrayArity(pexprArray);
	// comparison semantics for statistics purposes is looser than regular comparison.
	CStatsPred::EStatsCmpType escmpt = Estatscmptype(popScArrayCmp->MdIdOp());

	CScalarIdent *popScalarIdent = CScalarIdent::PopConvert(pexprIdent->Pop());
	const CColRef *pcr = popScalarIdent->Pcr();

	if (!CHistogram::FSupportsFilter(escmpt))
	{
		// unsupported predicate for stats calculations
		pdrgpstatspred->Append(GPOS_NEW(memory_pool) CStatsPredUnsupported(pcr->Id(), escmpt));

		return;
	}

	for (ULONG ul = 0; ul < ulConstants; ul++)
	{
		CExpression *pexprConst = CUtils::PScalarArrayExprChildAt(memory_pool, pexprArray, ul);
		if (COperator::EopScalarConst == pexprConst->Pop()->Eopid())
		{
			CScalarConst *popScalarConst = CScalarConst::PopConvert(pexprConst->Pop());
			IDatum *pdatumLiteral = popScalarConst->Pdatum();
			CStatsPred *pstatspredChild = NULL;
			if (!pdatumLiteral->FStatsComparable(pdatumLiteral))
			{
				// stats calculations on such datums unsupported
				pstatspredChild = GPOS_NEW(memory_pool) CStatsPredUnsupported(pcr->Id(), escmpt);
			}
			else
			{
				pstatspredChild = GPOS_NEW(memory_pool) CStatsPredPoint(memory_pool, pcr, escmpt, pdatumLiteral);
			}

			pdrgpstatspredChild->Append(pstatspredChild);
		}
		pexprConst->Release();
	}

	if (fAny)
	{
		CStatsPredDisj *pstatspredOr = GPOS_NEW(memory_pool) CStatsPredDisj(pdrgpstatspredChild);
		pdrgpstatspred->Append(pstatspredOr);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::PstatspredBoolean
//
//	@doc:
//		Extract statistics filtering information from boolean predicate
//		in the form of scalar id or negated scalar id
//---------------------------------------------------------------------------
CStatsPred *
CStatsPredUtils::PstatspredBoolean
	(
	IMemoryPool *memory_pool,
	CExpression *pexprPred,
	CColRefSet * //pcrsOuterRefs
	)
{
	GPOS_ASSERT(NULL != pexprPred);
	GPOS_ASSERT(CPredicateUtils::FBooleanScalarIdent(pexprPred) || CPredicateUtils::FNegatedBooleanScalarIdent(pexprPred));

	COperator *pop = pexprPred->Pop();

	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();

	IDatum *pdatum = NULL;
	ULONG col_id = ULONG_MAX;

	if (CPredicateUtils::FBooleanScalarIdent(pexprPred))
	{
		CScalarIdent *popScIdent = CScalarIdent::PopConvert(pop);
		pdatum = md_accessor->PtMDType<IMDTypeBool>()->PdatumBool(memory_pool, true /* fValue */, false /* is_null */);
		col_id = popScIdent->Pcr()->Id();
	}
	else
	{
		CExpression *pexprChild = (*pexprPred)[0];
		pdatum = md_accessor->PtMDType<IMDTypeBool>()->PdatumBool(memory_pool, false /* fValue */, false /* is_null */);
		col_id = CScalarIdent::PopConvert(pexprChild->Pop())->Pcr()->Id();
	}

	if (!pdatum->FStatsComparable(pdatum))
	{
		// stats calculations on such datums unsupported
		pdatum->Release();

		return GPOS_NEW(memory_pool) CStatsPredUnsupported(col_id, CStatsPred::EstatscmptEq);
	}


	GPOS_ASSERT(NULL != pdatum && ULONG_MAX != col_id);

	return GPOS_NEW(memory_pool) CStatsPredPoint(col_id, CStatsPred::EstatscmptEq, GPOS_NEW(memory_pool) CPoint(pdatum));
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::PstatsjoinExtract
//
//	@doc:
//		Helper function to extract the statistics join filter from a given join predicate
//
//---------------------------------------------------------------------------
CStatsPredJoin *
CStatsPredUtils::PstatsjoinExtract
	(
	IMemoryPool *memory_pool,
	CExpression *pexprJoinPred,
	DrgPcrs *pdrgpcrsOutput, // array of output columns of join's relational inputs
	CColRefSet *pcrsOuterRefs,
	DrgPexpr *pdrgpexprUnsupported
	)
{
	GPOS_ASSERT(NULL != pexprJoinPred);
	GPOS_ASSERT(NULL != pdrgpcrsOutput);
	GPOS_ASSERT(NULL != pdrgpexprUnsupported);

	CColRefSet *pcrsUsed = CDrvdPropScalar::Pdpscalar(pexprJoinPred->PdpDerive())->PcrsUsed();

	if (pcrsOuterRefs->ContainsAll(pcrsUsed))
	{
		return NULL;
	}

	const CColRef *pcrLeft = NULL;
	const CColRef *pcrRight = NULL;
	CStatsPred::EStatsCmpType escmpt = CStatsPred::EstatscmptOther;

	BOOL fSupportedScIdentComparison = FCmpColsIgnoreCast(pexprJoinPred, &pcrLeft, &escmpt, &pcrRight);
	if (fSupportedScIdentComparison && CStatsPred::EstatscmptOther != escmpt)
	{
		if (!IMDType::FStatsComparable(pcrLeft->Pmdtype(), pcrRight->Pmdtype()))
		{
			// unsupported statistics comparison between the histogram boundaries of the columns
			pexprJoinPred->AddRef();
			pdrgpexprUnsupported->Append(pexprJoinPred);
			return NULL;
		}

		ULONG ulIndexLeft = CUtils::UlPcrIndexContainingSet(pdrgpcrsOutput, pcrLeft);
		ULONG ulIndexRight = CUtils::UlPcrIndexContainingSet(pdrgpcrsOutput, pcrRight);

		if (ULONG_MAX != ulIndexLeft && ULONG_MAX != ulIndexRight && ulIndexLeft != ulIndexRight)
		{
			if (ulIndexLeft < ulIndexRight)
			{
				return GPOS_NEW(memory_pool) CStatsPredJoin(pcrLeft->Id(), escmpt, pcrRight->Id());
			}

			return GPOS_NEW(memory_pool) CStatsPredJoin(pcrRight->Id(), escmpt, pcrLeft->Id());
		}
	}

	if (CColRefSet::FCovered(pdrgpcrsOutput, pcrsUsed))
	{
		// unsupported join predicate
		pexprJoinPred->AddRef();
		pdrgpexprUnsupported->Append(pexprJoinPred);
	}

	return NULL;
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::PdrgpstatspredjoinExtract
//
//	@doc:
//		Helper function to extract array of statistics join filter
//		from an array of join predicates
//
//---------------------------------------------------------------------------
DrgPstatspredjoin *
CStatsPredUtils::PdrgpstatspredjoinExtract
	(
	IMemoryPool *memory_pool,
	CExpression *pexprScalar,
	DrgPcrs *pdrgpcrsOutput, // array of output columns of join's relational inputs
	CColRefSet *pcrsOuterRefs,
	CStatsPred **ppstatspredUnsupported
	)
{
	GPOS_ASSERT(NULL != pexprScalar);
	GPOS_ASSERT(NULL != pdrgpcrsOutput);

	DrgPstatspredjoin *pdrgpstatspredjoin = GPOS_NEW(memory_pool) DrgPstatspredjoin(memory_pool);

	DrgPexpr *pdrgpexprUnsupported = GPOS_NEW(memory_pool) DrgPexpr(memory_pool);

	// extract all the conjuncts
	DrgPexpr *pdrgpexprConjuncts = CPredicateUtils::PdrgpexprConjuncts(memory_pool, pexprScalar);
	const ULONG size = pdrgpexprConjuncts->Size();
	for (ULONG ul = 0; ul < size; ul++)
	{
		CExpression *pexprPred = (*pdrgpexprConjuncts) [ul];
		CStatsPredJoin *pstatsjoin = PstatsjoinExtract
										(
										memory_pool,
										pexprPred,
										pdrgpcrsOutput,
										pcrsOuterRefs,
										pdrgpexprUnsupported
										);
		if (NULL != pstatsjoin)
		{
			pdrgpstatspredjoin->Append(pstatsjoin);
		}
	}

	const ULONG ulUnsupported = pdrgpexprUnsupported->Size();
	if (1 == ulUnsupported)
	{
		*ppstatspredUnsupported = CStatsPredUtils::PstatspredExtract(memory_pool, (*pdrgpexprUnsupported)[0], pcrsOuterRefs);
	}
	else if (1 < ulUnsupported)
	{
		pdrgpexprUnsupported->AddRef();
		CExpression *pexprConj = CPredicateUtils::PexprConjDisj(memory_pool, pdrgpexprUnsupported, true /* fConjunction */);
		*ppstatspredUnsupported = CStatsPredUtils::PstatspredExtract(memory_pool, pexprConj, pcrsOuterRefs);
		pexprConj->Release();
	}

	// clean up
	pdrgpexprUnsupported->Release();
	pdrgpexprConjuncts->Release();

	return pdrgpstatspredjoin;
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::Pdrgpstatspredjoin
//
//	@doc:
//		Helper function to extract array of statistics join filter from
//		an expression
//---------------------------------------------------------------------------
DrgPstatspredjoin *
CStatsPredUtils::Pdrgpstatspredjoin
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	CExpression *pexprScalarInput,
	DrgPcrs *pdrgpcrsOutput, // array of output columns of join's relational inputs
	CColRefSet *pcrsOuterRefs
	)
{
	GPOS_ASSERT(NULL != pdrgpcrsOutput);

	// remove implied predicates from join condition to avoid cardinality under-estimation
	CExpression *pexprScalar = CPredicateUtils::PexprRemoveImpliedConjuncts(memory_pool, pexprScalarInput, exprhdl);

	// extract all the conjuncts
	CStatsPred *pstatspredUnsupported = NULL;
	DrgPstatspredjoin *pdrgpstatspredjoin = PdrgpstatspredjoinExtract
										(
										memory_pool,
										pexprScalar,
										pdrgpcrsOutput,
										pcrsOuterRefs,
										&pstatspredUnsupported
										);

	// TODO:  May 15 2014, handle unsupported predicates for LASJ, LOJ and LS joins
	// clean up
	CRefCount::SafeRelease(pstatspredUnsupported);
	pexprScalar->Release();

	return pdrgpstatspredjoin;
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::Pdrgpstatspredjoin
//
//	@doc:
//		Extract statistics join information
//
//---------------------------------------------------------------------------
DrgPstatspredjoin *
CStatsPredUtils::Pdrgpstatspredjoin
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl
	)
{
	// in case of subquery in join predicate, we return empty stats
	if (exprhdl.Pdpscalar(exprhdl.Arity() - 1)->FHasSubquery())
	{
		return GPOS_NEW(memory_pool) DrgPstatspredjoin(memory_pool);
	}

	DrgPcrs *pdrgpcrsOutput = GPOS_NEW(memory_pool) DrgPcrs(memory_pool);
	const ULONG size = exprhdl.Arity();
	for (ULONG ul = 0; ul < size - 1; ul++)
	{
		CColRefSet *pcrs = exprhdl.Pdprel(ul)->PcrsOutput();
		pcrs->AddRef();
		pdrgpcrsOutput->Append(pcrs);
	}

	// TODO:  02/29/2012 replace with constraint property info once available
	CExpression *pexprScalar = exprhdl.PexprScalarChild(exprhdl.Arity() - 1);
	CColRefSet *pcrsOuterRefs = exprhdl.Pdprel()->PcrsOuter();

	DrgPstatspredjoin *pdrgpstats = Pdrgpstatspredjoin(memory_pool, exprhdl, pexprScalar, pdrgpcrsOutput, pcrsOuterRefs);

	// clean up
	pdrgpcrsOutput->Release();

	return pdrgpstats;
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::FConjOrDisjPred
//
//	@doc:
//		Is the predicate a conjunctive or disjunctive predicate
//
//---------------------------------------------------------------------------
BOOL
CStatsPredUtils::FConjOrDisjPred
	(
	CStatsPred *pstatspred
	)
{
	return ((CStatsPred::EsptConj == pstatspred->Espt()) || (CStatsPred::EsptDisj == pstatspred->Espt()));
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredUtils::FUnsupportedPredOnDefinedCol
//
//	@doc:
//		Is unsupported predicate on defined column
//
//---------------------------------------------------------------------------
BOOL
CStatsPredUtils::FUnsupportedPredOnDefinedCol
	(
	CStatsPred *pstatspred
	)
{
	return ((CStatsPred::EsptUnsupported == pstatspred->Espt()) && (ULONG_MAX == pstatspred->GetColId()));
}


// EOF
