//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CScalarCoerceViaIO.cpp
//
//	@doc:
//		Implementation of scalar CoerceViaIO operators
//
//	@owner:
//
//	@test:
//
//
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpopt/operators/CScalarCoerceViaIO.h"

using namespace gpopt;
using namespace gpmd;


//---------------------------------------------------------------------------
//	@function:
//		CScalarCoerceViaIO::CScalarCoerceViaIO
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarCoerceViaIO::CScalarCoerceViaIO
	(
	IMemoryPool *memory_pool,
	IMDId *mdid_type,
	INT type_modifier,
	ECoercionForm ecf,
	INT iLoc
	)
	:
	CScalarCoerceBase(memory_pool, mdid_type, type_modifier, ecf, iLoc)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarCoerceViaIO::FMatch
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarCoerceViaIO::FMatch
	(
	COperator *pop
	)
	const
{
	if (pop->Eopid() == Eopid())
	{
		CScalarCoerceViaIO *popCoerce = CScalarCoerceViaIO::PopConvert(pop);

		return popCoerce->MDIdType()->Equals(MDIdType()) &&
				popCoerce->TypeModifier() == TypeModifier() &&
				popCoerce->Ecf() == Ecf() &&
				popCoerce->ILoc() == ILoc();
	}

	return false;
}


// EOF

