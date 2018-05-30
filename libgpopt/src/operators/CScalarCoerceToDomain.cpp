//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CScalarCoerceToDomain.cpp
//
//	@doc:
//		Implementation of scalar CoerceToDomain operators
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpopt/operators/CScalarCoerceToDomain.h"

using namespace gpopt;
using namespace gpmd;


//---------------------------------------------------------------------------
//	@function:
//		CScalarCoerceToDomain::CScalarCoerceToDomain
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarCoerceToDomain::CScalarCoerceToDomain
	(
	IMemoryPool *memory_pool,
	IMDId *mdid_type,
	INT type_modifier,
	ECoercionForm ecf,
	INT iLoc
	)
	:
	CScalarCoerceBase(memory_pool, mdid_type, type_modifier, ecf, iLoc),
	m_fReturnsNullOnNullInput(false)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarCoerceToDomain::FMatch
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarCoerceToDomain::FMatch
	(
	COperator *pop
	)
	const
{
	if (pop->Eopid() == Eopid())
	{
		CScalarCoerceToDomain *popCoerce = CScalarCoerceToDomain::PopConvert(pop);

		return popCoerce->MDIdType()->Equals(MDIdType()) &&
				popCoerce->TypeModifier() == TypeModifier() &&
				popCoerce->Ecf() == Ecf() &&
				popCoerce->ILoc() == ILoc();
	}

	return false;
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarCoerceToDomain::Eber
//
//	@doc:
//		Perform boolean expression evaluation
//
//---------------------------------------------------------------------------
CScalar::EBoolEvalResult
CScalarCoerceToDomain::Eber
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

