//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CScalarCmp.cpp
//
//	@doc:
//		Implementation of scalar comparison operator
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CDrvdPropScalar.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/COptCtxt.h"

#include "gpopt/mdcache/CMDAccessorUtils.h"

#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/operators/CExpressionHandle.h"

#include "naucrates/md/IMDTypeBool.h"
#include "naucrates/md/IMDScalarOp.h"

using namespace gpopt;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CScalarCmp::CScalarCmp
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarCmp::CScalarCmp
	(
	IMemoryPool *memory_pool,
	IMDId *mdid_op,
	const CWStringConst *pstrOp,
	IMDType::ECmpType ecmpt
	)
	:
	CScalar(memory_pool),
	m_mdid_op(mdid_op),
	m_pstrOp(pstrOp),
	m_ecmpt(ecmpt),
	m_fReturnsNullOnNullInput(false)
{
	GPOS_ASSERT(mdid_op->IsValid());

	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	m_fReturnsNullOnNullInput = CMDAccessorUtils::FScalarOpReturnsNullOnNullInput(md_accessor, m_mdid_op);
	m_fCommutative = CMDAccessorUtils::FCommutativeScalarOp(md_accessor, m_mdid_op);
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarCmp::GetMDName
//
//	@doc:
//		Comparison operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CScalarCmp::Pstr() const
{
	return m_pstrOp;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarCmp::MdIdOp
//
//	@doc:
//		Comparison operator metadata id
//
//---------------------------------------------------------------------------
IMDId *
CScalarCmp::MdIdOp() const
{
	return m_mdid_op;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarCmp::HashValue
//
//	@doc:
//		Operator specific hash function; combined hash of operator id and
//		metadata id
//
//---------------------------------------------------------------------------
ULONG
CScalarCmp::HashValue() const
{
	return gpos::CombineHashes(COperator::HashValue(), m_mdid_op->HashValue());
}

	
//---------------------------------------------------------------------------
//	@function:
//		CScalarCmp::FMatch
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarCmp::FMatch
	(
	COperator *pop
	)
	const
{
	if (pop->Eopid() == Eopid())
	{
		CScalarCmp *popScCmp = CScalarCmp::PopConvert(pop);
		
		// match if operator oid are identical
		return m_mdid_op->Equals(popScCmp->MdIdOp());
	}
	
	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarCmp::FInputOrderSensitive
//
//	@doc:
//		Sensitivity to order of inputs
//
//---------------------------------------------------------------------------
BOOL
CScalarCmp::FInputOrderSensitive() const
{
	return !m_fCommutative;
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarCmp::MDIdType
//
//	@doc:
//		Expression type
//
//---------------------------------------------------------------------------
IMDId *
CScalarCmp::MDIdType() const
{
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	return md_accessor->PtMDType<IMDTypeBool>()->MDId();
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarCmp::Eber
//
//	@doc:
//		Perform boolean expression evaluation
//
//---------------------------------------------------------------------------
CScalar::EBoolEvalResult
CScalarCmp::Eber
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

// get metadata id of the commuted operator
IMDId *
CScalarCmp::PmdidCommuteOp
	(
	CMDAccessor *md_accessor,
	COperator *pop
	)
{
	CScalarCmp *popScalarCmp = dynamic_cast<CScalarCmp *>(pop);
	const IMDScalarOp *pmdScalarCmpOp = md_accessor->Pmdscop(popScalarCmp->MdIdOp());

	IMDId *pmdidScalarCmpCommute = pmdScalarCmpOp->PmdidOpCommute();
	return pmdidScalarCmpCommute;
}

// get the string representation of a metadata object
CWStringConst *
CScalarCmp::Pstr
	(
	IMemoryPool *memory_pool,
	CMDAccessor *md_accessor,
	IMDId *pmdid
	)
{
	pmdid->AddRef();
	return GPOS_NEW(memory_pool) CWStringConst(memory_pool, (md_accessor->Pmdscop(pmdid)->Mdname().GetMDName())->GetBuffer());
}

// get commuted scalar comparision operator
CScalarCmp *
CScalarCmp::PopCommutedOp
	(
	IMemoryPool *memory_pool,
	COperator *pop
	)
{
	
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	IMDId *pmdid = PmdidCommuteOp(md_accessor, pop);
	if (NULL != pmdid && pmdid->IsValid())
	{
		return GPOS_NEW(memory_pool) CScalarCmp(memory_pool, pmdid, Pstr(memory_pool, md_accessor, pmdid), CUtils::ParseCmpType(pmdid));
	}
	return NULL;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarCmp::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CScalarCmp::OsPrint
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


// EOF

