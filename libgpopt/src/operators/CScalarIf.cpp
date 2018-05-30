//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CScalarIf.cpp
//
//	@doc:
//		Implementation of scalar if operator
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CDrvdPropScalar.h"
#include "gpopt/base/CColRefSet.h"

#include "gpopt/operators/CScalarIf.h"
#include "gpopt/operators/CExpressionHandle.h"

#include "gpopt/mdcache/CMDAccessorUtils.h"

#include "naucrates/md/IMDTypeBool.h"

using namespace gpopt;
using namespace gpmd;


//---------------------------------------------------------------------------
//	@function:
//		CScalarIf::CScalarIf
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarIf::CScalarIf
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid
	)
	:
	CScalar(memory_pool),
	m_mdid_type(pmdid),
	m_fBoolReturnType(false)
{
	GPOS_ASSERT(pmdid->IsValid());

	CMDAccessor *pmda = COptCtxt::PoctxtFromTLS()->Pmda();
	m_fBoolReturnType = CMDAccessorUtils::FBoolType(pmda, m_mdid_type);
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarIf::HashValue
//
//	@doc:
//		Operator specific hash function; combined hash of operator id and
//		return type id
//
//---------------------------------------------------------------------------
ULONG
CScalarIf::HashValue() const
{
	return gpos::CombineHashes(COperator::HashValue(), m_mdid_type->HashValue());
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarIf::FMatch
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarIf::FMatch
	(
	COperator *pop
	)
	const
{
	if(pop->Eopid() == Eopid())
	{
		CScalarIf *popScIf = CScalarIf::PopConvert(pop);

		// match if return types are identical
		return popScIf->MDIdType()->Equals(m_mdid_type);
	}

	return false;
}

// EOF

