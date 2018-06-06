//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CScalarOp.cpp
//
//	@doc:
//		Implementation of general scalar operator
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "naucrates/md/IMDScalarOp.h"

#include "gpopt/base/CDrvdPropScalar.h"
#include "gpopt/base/CColRefSet.h"

#include "gpopt/mdcache/CMDAccessorUtils.h"

#include "gpopt/operators/CScalarOp.h"
#include "gpopt/operators/CExpressionHandle.h"


using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CScalarOp::CScalarOp
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarOp::CScalarOp
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidOp,
	IMDId *pmdidReturnType,
	const CWStringConst *pstrOp
	)
	:
	CScalar(memory_pool),
	m_pmdidOp(pmdidOp),
	m_pmdidReturnType(pmdidReturnType),
	m_pstrOp(pstrOp),
	m_fReturnsNullOnNullInput(false),
	m_fBoolReturnType(false),
	m_fCommutative(false)
{
	GPOS_ASSERT(pmdidOp->IsValid());

	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();

	m_fReturnsNullOnNullInput = CMDAccessorUtils::FScalarOpReturnsNullOnNullInput(md_accessor, m_pmdidOp);
	m_fCommutative = CMDAccessorUtils::FCommutativeScalarOp(md_accessor, m_pmdidOp);
	m_fBoolReturnType = CMDAccessorUtils::FBoolType(md_accessor, m_pmdidReturnType);
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarOp::GetMDName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CScalarOp::Pstr() const
{
	return m_pstrOp;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarOp::PmdidOp
//
//	@doc:
//		Scalar operator metadata id
//
//---------------------------------------------------------------------------
IMDId *
CScalarOp::PmdidOp() const
{
	return m_pmdidOp;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarOp::HashValue
//
//	@doc:
//		Operator specific hash function; combined hash of operator id and
//		metadata id
//
//---------------------------------------------------------------------------
ULONG
CScalarOp::HashValue() const
{
	return gpos::CombineHashes(COperator::HashValue(), m_pmdidOp->HashValue());
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarOp::FMatch
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarOp::FMatch
	(
	COperator *pop
	)
	const
{
	if (pop->Eopid() == Eopid())
	{
		CScalarOp *pscop = CScalarOp::PopConvert(pop);

		// match if operator oid are identical
		return m_pmdidOp->Equals(pscop->PmdidOp());
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarOp::PmdidReturnType
//
//	@doc:
//		Accessor to the return type
//
//---------------------------------------------------------------------------
IMDId *
CScalarOp::PmdidReturnType() const
{
	return m_pmdidReturnType;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarOp::MDIdType
//
//	@doc:
//		Expression type
//
//---------------------------------------------------------------------------
IMDId *
CScalarOp::MDIdType() const
{
	if (NULL != m_pmdidReturnType)
	{
		return m_pmdidReturnType;
	}
	
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	return md_accessor->Pmdscop(m_pmdidOp)->PmdidTypeResult();
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarOp::FInputOrderSensitive
//
//	@doc:
//		Sensitivity to order of inputs
//
//---------------------------------------------------------------------------
BOOL
CScalarOp::FInputOrderSensitive() const
{
	return !m_fCommutative;
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarOp::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CScalarOp::OsPrint
	(
	IOstream &os
	)
	const
{
	os << SzId() << " (";
	os << Pstr()->GetBuffer();
	os << ")";

	return os;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarOp::Eber
//
//	@doc:
//		Perform boolean expression evaluation
//
//---------------------------------------------------------------------------
CScalar::EBoolEvalResult
CScalarOp::Eber
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

// EOF

