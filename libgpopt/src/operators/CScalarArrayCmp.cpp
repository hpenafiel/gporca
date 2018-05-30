//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CScalarArrayCmp.cpp
//
//	@doc:
//		Implementation of scalar array comparison operator
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CDrvdPropScalar.h"
#include "gpopt/base/CColRefSet.h"

#include "gpopt/mdcache/CMDAccessorUtils.h"

#include "gpopt/operators/CScalarArrayCmp.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CPredicateUtils.h"

#include "naucrates/md/IMDTypeBool.h"

using namespace gpopt;
using namespace gpmd;

const CHAR CScalarArrayCmp::m_rgszCmpType[EarrcmpSentinel][10] =
{
	"Any",
	"All"
};


//---------------------------------------------------------------------------
//	@function:
//		CScalarOp::CScalarArrayCmp
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarArrayCmp::CScalarArrayCmp
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidOp,
	const CWStringConst *pstrOp,
	EArrCmpType earrcmpt
	)
	:
	CScalar(memory_pool),
	m_pmdidOp(pmdidOp),
	m_pscOp(pstrOp),
	m_earrccmpt(earrcmpt),
	m_fReturnsNullOnNullInput(false)
{
	GPOS_ASSERT(pmdidOp->IsValid());
	GPOS_ASSERT(EarrcmpSentinel > earrcmpt);

	CMDAccessor *pmda = COptCtxt::PoctxtFromTLS()->Pmda();
	m_fReturnsNullOnNullInput = CMDAccessorUtils::FScalarOpReturnsNullOnNullInput(pmda, m_pmdidOp);
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayCmp::Pstr
//
//	@doc:
//		Comparison operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CScalarArrayCmp::Pstr() const
{
	return m_pscOp;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayCmp::PmdidOp
//
//	@doc:
//		Comparison operator mdid
//
//---------------------------------------------------------------------------
IMDId *
CScalarArrayCmp::PmdidOp() const
{
	return m_pmdidOp;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayCmp::HashValue
//
//	@doc:
//		Operator specific hash function; combined hash of operator id and
//		metadata id
//
//---------------------------------------------------------------------------
ULONG
CScalarArrayCmp::HashValue() const
{
	return gpos::CombineHashes
					(
					gpos::CombineHashes(COperator::HashValue(), m_pmdidOp->HashValue()),
					m_earrccmpt
					);
}

	
//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayCmp::FMatch
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarArrayCmp::FMatch
	(
	COperator *pop
	)
	const
{
	if (pop->Eopid() == Eopid())
	{
		CScalarArrayCmp *popCmp = CScalarArrayCmp::PopConvert(pop);
		
		// match if operator oid are identical
		return m_earrccmpt == popCmp->Earrcmpt() && m_pmdidOp->Equals(popCmp->PmdidOp());
	}
	
	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayCmp::MDIdType
//
//	@doc:
//		Expression type
//
//---------------------------------------------------------------------------
IMDId *
CScalarArrayCmp::MDIdType() const
{
	CMDAccessor *pmda = COptCtxt::PoctxtFromTLS()->Pmda();
	return pmda->PtMDType<IMDTypeBool>()->MDId();
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayCmp::Eber
//
//	@doc:
//		Perform boolean expression evaluation
//
//---------------------------------------------------------------------------
CScalar::EBoolEvalResult
CScalarArrayCmp::Eber
	(
	ULongPtrArray *pdrgpulChildren
	)
	const
{
	if (m_fReturnsNullOnNullInput)
	{
		return EberNullOnAnyNullChild(pdrgpulChildren);
	}

	return EberUnknown;
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayCmp::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CScalarArrayCmp::OsPrint
	(
	IOstream &os
	)
	const
{
	os << SzId() << " " <<  m_rgszCmpType[m_earrccmpt] << " (";
	os << Pstr()->GetBuffer();
	os << ")";
	
	return os;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayCmp::PexprExpand
//
//	@doc:
//		Expand array comparison expression into a conjunctive/disjunctive
//		expression
//
//---------------------------------------------------------------------------
CExpression *
CScalarArrayCmp::PexprExpand
	(
	IMemoryPool *memory_pool,
	CExpression *pexprArrayCmp
	)
{
	GPOS_ASSERT(NULL != pexprArrayCmp);
	GPOS_ASSERT(EopScalarArrayCmp == pexprArrayCmp->Pop()->Eopid());

	CExpression *pexprIdent = (*pexprArrayCmp)[0];
	CExpression *pexprArray = CUtils::PexprScalarArrayChild(pexprArrayCmp);
	CScalarArrayCmp *popArrayCmp = CScalarArrayCmp::PopConvert(pexprArrayCmp->Pop());
	ULONG ulArrayElems = 0;

	if (CUtils::FScalarArray(pexprArray))
	{
		ulArrayElems = CUtils::UlScalarArrayArity(pexprArray);
	}

	// if this condition is true, we know the right child of ArrayCmp is a constant.
	if (0 == ulArrayElems)
	{
		// if right child is not an actual array (e.g., Const of type array), return input
		// expression without expansion
		pexprArrayCmp->AddRef();
		return pexprArrayCmp;
	}

	DrgPexpr *pdrgpexpr = GPOS_NEW(memory_pool) DrgPexpr(memory_pool);
	for (ULONG ul = 0; ul < ulArrayElems; ul++)
	{
		CExpression *pexprArrayElem = CUtils::PScalarArrayExprChildAt(memory_pool, pexprArray, ul);
		pexprIdent->AddRef();
		const CWStringConst *pstrOpName = popArrayCmp->Pstr();
		IMDId *pmdidOp = popArrayCmp->PmdidOp();
		GPOS_ASSERT(IMDId::IsValid(pmdidOp));

		pmdidOp->AddRef();

		CExpression *pexprCmp = CUtils::PexprScalarCmp(memory_pool, pexprIdent, pexprArrayElem, *pstrOpName, pmdidOp);
		pdrgpexpr->Append(pexprCmp);
	}
	GPOS_ASSERT(0 < pdrgpexpr->Size());

	// deduplicate resulting array
	DrgPexpr *pdrgpexprDeduped = CUtils::PdrgpexprDedup(memory_pool, pdrgpexpr);
	pdrgpexpr->Release();

	EArrCmpType earrcmpt = popArrayCmp->Earrcmpt();
	if (EarrcmpAny == earrcmpt)
	{
		return CPredicateUtils::PexprDisjunction(memory_pool, pdrgpexprDeduped);
	}
	GPOS_ASSERT(EarrcmpAll == earrcmpt);

	return CPredicateUtils::PexprConjunction(memory_pool, pdrgpexprDeduped);
}



// EOF

