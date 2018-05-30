//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 Pivotal Software, Inc.
//
//	@filename:
//		CCastUtils.cpp
//
//	@doc:
//		Implementation of cast utility functions
//---------------------------------------------------------------------------

#include "gpos/memory/CAutoMemoryPool.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/base/CCastUtils.h"
#include "gpopt/mdcache/CMDAccessorUtils.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarArrayCoerceExpr.h"
#include "gpopt/operators/CScalarCast.h"
#include "naucrates/md/IMDCast.h"
#include "naucrates/md/CMDArrayCoerceCastGPDB.h"
#include "gpopt/operators/CPredicateUtils.h"

using namespace gpopt;
using namespace gpmd;

// is the given expression a binary coercible cast of a scalar identifier
BOOL
CCastUtils::FBinaryCoercibleCastedScId
    (
	CExpression *pexpr,
	CColRef *pcr
	)
{
	GPOS_ASSERT(NULL != pexpr);

	if (!FBinaryCoercibleCast(pexpr))
	{
		return false;
	}

    CExpression *pexprChild = (*pexpr)[0];

	// cast(col1)
	return COperator::EopScalarIdent == pexprChild->Pop()->Eopid() &&
	pcr == CScalarIdent::PopConvert(pexprChild->Pop())->Pcr();
}

// is the given expression a binary coercible cast of a scalar identifier
BOOL
CCastUtils::FBinaryCoercibleCastedScId
    (
	CExpression *pexpr
	)
{
	GPOS_ASSERT(NULL != pexpr);

	if (!FBinaryCoercibleCast(pexpr))
	{
		return false;
	}

    CExpression *pexprChild = (*pexpr)[0];

	// cast(col1)
	return COperator::EopScalarIdent == pexprChild->Pop()->Eopid();
}

// extract the column reference if the given expression a scalar identifier
// or a cast of a scalar identifier or a function that casts a scalar identifier.
// Else return NULL.
const CColRef *
CCastUtils::PcrExtractFromScIdOrCastScId
    (
	CExpression *pexpr
	)
{
	GPOS_ASSERT(NULL != pexpr);

    BOOL fScIdent = COperator::EopScalarIdent == pexpr->Pop()->Eopid();
	BOOL fCastedScIdent = CScalarIdent::FCastedScId(pexpr);

	// col or cast(col)
	if (!fScIdent && !fCastedScIdent)
	{
		return NULL;
	}

    CScalarIdent *popScIdent = NULL;
	if (fScIdent)
	{
		popScIdent = CScalarIdent::PopConvert(pexpr->Pop());
	}
	else
	{
		GPOS_ASSERT(fCastedScIdent);
		popScIdent = CScalarIdent::PopConvert((*pexpr)[0]->Pop());
	}

	return popScIdent->Pcr();
}

// cast the input column reference to the destination mdid
CExpression *
CCastUtils::PexprCast
    (
	IMemoryPool *memory_pool,
	CMDAccessor *pmda,
	const CColRef *pcr,
	IMDId *pmdidDest
	)
{
	GPOS_ASSERT(NULL != pmdidDest);

    IMDId *pmdidSrc = pcr->Pmdtype()->MDId();
	GPOS_ASSERT(CMDAccessorUtils::FCastExists(pmda, pmdidSrc, pmdidDest));

	const IMDCast *pmdcast = pmda->Pmdcast(pmdidSrc, pmdidDest);

    pmdidDest->AddRef();
	pmdcast->PmdidCastFunc()->AddRef();
	CExpression *pexpr;

	if(pmdcast->EmdPathType() == IMDCast::EmdtArrayCoerce)
	{
		CMDArrayCoerceCastGPDB *parrayCoerceCast = (CMDArrayCoerceCastGPDB *) pmdcast;
		pexpr = GPOS_NEW(memory_pool) CExpression
		(
		 memory_pool,
		 GPOS_NEW(memory_pool) CScalarArrayCoerceExpr
		 (
		  memory_pool,
		  parrayCoerceCast->PmdidCastFunc(),
		  pmdidDest,
		  parrayCoerceCast->TypeModifier(),
		  parrayCoerceCast->FIsExplicit(),
		  (COperator::ECoercionForm) parrayCoerceCast->Ecf(),
		  parrayCoerceCast->ILoc()
		  ),
		 CUtils::PexprScalarIdent(memory_pool, pcr)
		 );
	}
	else
	{
		CScalarCast *popCast = GPOS_NEW(memory_pool) CScalarCast(memory_pool, pmdidDest, pmdcast->PmdidCastFunc(), pmdcast->FBinaryCoercible());
		pexpr = GPOS_NEW(memory_pool) CExpression(memory_pool, popCast, CUtils::PexprScalarIdent(memory_pool, pcr));
	}
	return pexpr;
}

// check whether the given expression is a binary coercible cast of something
BOOL
CCastUtils::FBinaryCoercibleCast
    (
	CExpression *pexpr
	)
{
	GPOS_ASSERT(NULL != pexpr);
	COperator *pop = pexpr->Pop();

	return FScalarCast(pexpr) &&
	CScalarCast::PopConvert(pop)->FBinaryCoercible();
}

BOOL
CCastUtils::FScalarCast
	(
	CExpression *pexpr
	)
{
	GPOS_ASSERT(NULL != pexpr);
	COperator *pop = pexpr->Pop();

	return COperator::EopScalarCast == pop->Eopid();
}

// return the given expression without any binary coercible casts
// that exist on the top
CExpression *
CCastUtils::PexprWithoutBinaryCoercibleCasts
    (
	CExpression *pexpr
	)
{
	GPOS_ASSERT(NULL != pexpr);
	GPOS_ASSERT(pexpr->Pop()->FScalar());

	CExpression *pexprOutput = pexpr;

    while (FBinaryCoercibleCast(pexprOutput))
	{
		GPOS_ASSERT(1 == pexprOutput->UlArity());
		pexprOutput = (*pexprOutput)[0];
	}

	return pexprOutput;
}

// add explicit casting to equality operations between compatible types
DrgPexpr *
CCastUtils::PdrgpexprCastEquality
    (
	IMemoryPool *memory_pool,
	CExpression *pexpr
	)
{
    GPOS_ASSERT(pexpr->Pop()->FScalar());

    DrgPexpr *pdrgpexpr = CPredicateUtils::PdrgpexprConjuncts(memory_pool, pexpr);
    DrgPexpr *pdrgpexprNew = GPOS_NEW(memory_pool) DrgPexpr(memory_pool);
    const ULONG ulPreds = pdrgpexpr->Size();
    for (ULONG ul = 0; ul < ulPreds; ul++)
    {
        CExpression *pexprPred = (*pdrgpexpr)[ul];
        pexprPred->AddRef();
        CExpression *pexprNewPred = pexprPred;

        if (CPredicateUtils::FEquality(pexprPred) || CPredicateUtils::FINDF(pexprPred))
        {
            CExpression *pexprCasted = PexprAddCast(memory_pool, pexprPred);
            if (NULL != pexprCasted)
            {
                // release predicate since we will construct a new one
                pexprNewPred->Release();
                pexprNewPred = pexprCasted;
            }
        }
        pdrgpexprNew->Append(pexprNewPred);
    }

    pdrgpexpr->Release();

    return pdrgpexprNew;
}

// add explicit casting to left child of given equality or INDF predicate
// and return resulting casted expression;
// the function returns NULL if operation failed
CExpression *
CCastUtils::PexprAddCast
    (
	IMemoryPool *memory_pool,
	CExpression *pexprPred
	)
{
    GPOS_ASSERT(NULL != pexprPred);
    GPOS_ASSERT(CUtils::FScalarCmp(pexprPred) || CPredicateUtils::FINDF(pexprPred));

    CExpression *pexprChild = pexprPred;

    if (!CUtils::FScalarCmp(pexprPred))
    {
        pexprChild = (*pexprPred)[0];
    }

    CExpression *pexprLeft = (*pexprChild)[0];
    CExpression *pexprRight = (*pexprChild)[1];

    IMDId *pmdidTypeLeft = CScalar::PopConvert(pexprLeft->Pop())->MDIdType();
    IMDId *pmdidTypeRight = CScalar::PopConvert(pexprRight->Pop())->MDIdType();

    CMDAccessor *pmda = COptCtxt::PoctxtFromTLS()->Pmda();

    CExpression *pexprNewPred = NULL;

    BOOL fTypesEqual = pmdidTypeLeft->Equals(pmdidTypeRight);
    BOOL fCastLtoR = CMDAccessorUtils::FCastExists(pmda, pmdidTypeLeft, pmdidTypeRight);
    BOOL fCastRtoL = CMDAccessorUtils::FCastExists(pmda, pmdidTypeRight, pmdidTypeLeft);

    if (fTypesEqual || !(fCastLtoR || fCastRtoL))
    {
        return pexprNewPred;
    }

    pexprLeft->AddRef();
    pexprRight->AddRef();

    CExpression *pexprNewLeft = pexprLeft;
    CExpression *pexprNewRight = pexprRight;

    if (fCastLtoR)
    {
        pexprNewLeft = PexprCast(memory_pool, pmda, pexprLeft, pmdidTypeRight);
    }
    else
    {
        GPOS_ASSERT(fCastRtoL);
        pexprNewRight = PexprCast(memory_pool, pmda, pexprRight, pmdidTypeLeft);;
    }

    GPOS_ASSERT(NULL != pexprNewLeft && NULL != pexprNewRight);

    if (CUtils::FScalarCmp(pexprPred))
    {
        pexprNewPred = CUtils::PexprScalarCmp(memory_pool, pexprNewLeft, pexprNewRight, IMDType::EcmptEq);
    }
    else
    {
        pexprNewPred = CUtils::PexprINDF(memory_pool, pexprNewLeft, pexprNewRight);
    }

    return pexprNewPred;
}

// add explicit casting on the input expression to the destination type
CExpression *
CCastUtils::PexprCast
    (
	IMemoryPool *memory_pool,
	CMDAccessor *pmda,
	CExpression *pexpr,
	IMDId *pmdidDest
	)
{
    IMDId *pmdidSrc = CScalar::PopConvert(pexpr->Pop())->MDIdType();
    const IMDCast *pmdcast = pmda->Pmdcast(pmdidSrc, pmdidDest);

    pmdidDest->AddRef();
    pmdcast->PmdidCastFunc()->AddRef();
    CExpression *pexprCast;

    if (pmdcast->EmdPathType() == IMDCast::EmdtArrayCoerce)
    {
        CMDArrayCoerceCastGPDB *parrayCoerceCast = (CMDArrayCoerceCastGPDB *) pmdcast;
        pexprCast = GPOS_NEW(memory_pool) CExpression
        (
         memory_pool,
         GPOS_NEW(memory_pool) CScalarArrayCoerceExpr(memory_pool, parrayCoerceCast->PmdidCastFunc(), pmdidDest, parrayCoerceCast->TypeModifier(), parrayCoerceCast->FIsExplicit(), (COperator::ECoercionForm) parrayCoerceCast->Ecf(), parrayCoerceCast->ILoc()),
         pexpr
         );
    }
    else
    {
        CScalarCast *popCast = GPOS_NEW(memory_pool) CScalarCast(memory_pool, pmdidDest, pmdcast->PmdidCastFunc(), pmdcast->FBinaryCoercible());
        pexprCast = GPOS_NEW(memory_pool) CExpression(memory_pool, popCast, pexpr);
    }

    return pexprCast;
}

// EOF
