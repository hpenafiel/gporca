//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CScalarArrayRef.cpp
//
//	@doc:
//		Implementation of scalar arrayref
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/operators/CScalarArrayRef.h"

using namespace gpopt;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayRef::CScalarArrayRef
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarArrayRef::CScalarArrayRef
	(
	IMemoryPool *pmp,
	IMDId *pmdidElem,
	INT iTypeModifier,
	IMDId *pmdidArray,
	IMDId *pmdidReturn
	)
	:
	CScalar(pmp),
	m_pmdidElem(pmdidElem),
	m_iTypeModifier(iTypeModifier),
	m_pmdidArray(pmdidArray),
	m_pmdidType(pmdidReturn)
{
	GPOS_ASSERT(pmdidElem->IsValid());
	GPOS_ASSERT(pmdidArray->IsValid());
	GPOS_ASSERT(pmdidReturn->IsValid());
	GPOS_ASSERT(pmdidReturn->Equals(pmdidElem) || pmdidReturn->Equals(pmdidArray));
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayRef::~CScalarArrayRef
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CScalarArrayRef::~CScalarArrayRef()
{
	m_pmdidElem->Release();
	m_pmdidArray->Release();
	m_pmdidType->Release();
}


INT
CScalarArrayRef::TypeModifier() const
{
	return m_iTypeModifier;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayRef::HashValue
//
//	@doc:
//		Operator specific hash function
//
//---------------------------------------------------------------------------
ULONG
CScalarArrayRef::HashValue() const
{
	return gpos::CombineHashes
					(
					CombineHashes(m_pmdidElem->HashValue(), m_pmdidArray->HashValue()),
					m_pmdidType->HashValue()
					);
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayRef::FMatch
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarArrayRef::FMatch
	(
	COperator *pop
	)
	const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}

	CScalarArrayRef *popArrayRef = CScalarArrayRef::PopConvert(pop);

	return m_pmdidType->Equals(popArrayRef->MDIdType()) &&
			m_pmdidElem->Equals(popArrayRef->PmdidElem()) &&
			m_pmdidArray->Equals(popArrayRef->PmdidArray());
}

// EOF

