//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CScalarNullIf.cpp
//
//	@doc:
//		Implementation of scalar NullIf operator
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CDrvdPropScalar.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/COptCtxt.h"

#include "gpopt/mdcache/CMDAccessorUtils.h"

#include "gpopt/operators/CScalarNullIf.h"
#include "gpopt/operators/CExpressionHandle.h"

#include "naucrates/md/IMDTypeBool.h"

using namespace gpopt;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CScalarNullIf::CScalarNullIf
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarNullIf::CScalarNullIf
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidOp,
	IMDId *mdid_type
	)
	:
	CScalar(memory_pool),
	m_pmdidOp(pmdidOp),
	m_mdid_type(mdid_type),
	m_fReturnsNullOnNullInput(false),
	m_fBoolReturnType(false)
{
	GPOS_ASSERT(pmdidOp->IsValid());
	GPOS_ASSERT(mdid_type->IsValid());

	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	m_fReturnsNullOnNullInput = CMDAccessorUtils::FScalarOpReturnsNullOnNullInput(md_accessor, m_pmdidOp);
	m_fBoolReturnType = CMDAccessorUtils::FBoolType(md_accessor, m_mdid_type);
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarNullIf::~CScalarNullIf
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CScalarNullIf::~CScalarNullIf()
{
	m_pmdidOp->Release();
	m_mdid_type->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarNullIf::HashValue
//
//	@doc:
//		Operator specific hash function; combined hash of operator id and
//		return type id
//
//---------------------------------------------------------------------------
ULONG
CScalarNullIf::HashValue() const
{
	return gpos::CombineHashes(COperator::HashValue(),
			gpos::CombineHashes(m_pmdidOp->HashValue(),m_mdid_type->HashValue()));
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarNullIf::FMatch
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarNullIf::FMatch
	(
	COperator *pop
	)
	const
{
	if(pop->Eopid() != Eopid())
	{
		return false;
	}

	CScalarNullIf *popScNullIf = CScalarNullIf::PopConvert(pop);

	// match if operators and return types are identical
	return m_pmdidOp->Equals(popScNullIf->PmdidOp()) &&
			m_mdid_type->Equals(popScNullIf->MDIdType());
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarNullIf::Eber
//
//	@doc:
//		Perform boolean expression evaluation
//
//---------------------------------------------------------------------------
CScalar::EBoolEvalResult
CScalarNullIf::Eber
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

