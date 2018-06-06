//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CScalarCast.cpp
//
//	@doc:
//		Implementation of scalar relabel type  operator
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CDrvdPropScalar.h"
#include "gpopt/base/CColRefSet.h"

#include "gpopt/mdcache/CMDAccessorUtils.h"

#include "gpopt/operators/CScalarCast.h"
#include "gpopt/operators/CExpressionHandle.h"

#include "naucrates/md/IMDTypeBool.h"

using namespace gpopt;
using namespace gpmd;


//---------------------------------------------------------------------------
//	@function:
//		CScalarCast::CScalarCast
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarCast::CScalarCast
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidReturnType,
	IMDId *pmdidFunc,
	BOOL fBinaryCoercible
	)
	:
	CScalar(memory_pool),
	m_pmdidReturnType(pmdidReturnType),
	m_func_mdid(pmdidFunc),
	m_fBinaryCoercible(fBinaryCoercible),
	m_fReturnsNullOnNullInput(false),
	m_fBoolReturnType(false)
{
	if (NULL != m_func_mdid && m_func_mdid->IsValid())
	{
		CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
		const IMDFunction *pmdfunc = md_accessor->Pmdfunc(m_func_mdid);

		m_fReturnsNullOnNullInput = pmdfunc->FStrict();
		m_fBoolReturnType = CMDAccessorUtils::FBoolType(md_accessor, m_pmdidReturnType);
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarCast::FMatch
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarCast::FMatch
	(
	COperator *pop
	)
	const
{
	if (pop->Eopid() == Eopid())
	{
		CScalarCast *pscop = CScalarCast::PopConvert(pop);

		// match if the return type oids are identical
		return pscop->MDIdType()->Equals(m_pmdidReturnType) &&
				((!IMDId::IsValid(pscop->FuncMdId()) && !IMDId::IsValid(m_func_mdid)) || pscop->FuncMdId()->Equals(m_func_mdid));
	}

	return false;
}

// EOF

