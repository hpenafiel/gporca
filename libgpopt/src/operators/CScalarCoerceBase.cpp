//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 Pivotal Inc.
//
//	@filename:
//		CScalarCoerceBase.cpp
//
//	@doc:
//		Implementation of scalar coerce operator base class
//
//	@owner:
//
//	@test:
//
//
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpopt/operators/CScalarCoerceBase.h"

using namespace gpopt;
using namespace gpmd;


//---------------------------------------------------------------------------
//	@function:
//		CScalarCoerceBase::CScalarCoerceBase
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarCoerceBase::CScalarCoerceBase
	(
	IMemoryPool *memory_pool,
	IMDId *mdid_type,
	INT type_modifier,
	ECoercionForm ecf,
	INT iLoc
	)
	:
	CScalar(memory_pool),
	m_result_type_mdid(mdid_type),
	m_type_modifier(type_modifier),
	m_ecf(ecf),
	m_iLoc(iLoc)
{
	GPOS_ASSERT(NULL != mdid_type);
	GPOS_ASSERT(mdid_type->IsValid());
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarCoerceBase::~CScalarCoerceBase
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CScalarCoerceBase::~CScalarCoerceBase()
{
	m_result_type_mdid->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarCoerceBase::MDIdType
//
//	@doc:
//		Return type of the scalar expression
//
//---------------------------------------------------------------------------
IMDId*
CScalarCoerceBase::MDIdType() const
{
	return m_result_type_mdid;
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarCoerceBase::TypeModifier
//
//	@doc:
//		Return type modifier
//
//---------------------------------------------------------------------------
INT
CScalarCoerceBase::TypeModifier() const
{
	return m_type_modifier;
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarCoerceBase::Ecf
//
//	@doc:
//		Return coercion form
//
//---------------------------------------------------------------------------
CScalar::ECoercionForm
CScalarCoerceBase::Ecf() const
{
	return m_ecf;
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarCoerceBase::ILoc
//
//	@doc:
//		Return token location
//
//---------------------------------------------------------------------------
INT
CScalarCoerceBase::ILoc() const
{
	return m_iLoc;
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarCoerceBase::PopCopyWithRemappedColumns
//
//	@doc:
//		return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
COperator*
CScalarCoerceBase::PopCopyWithRemappedColumns
	(
	IMemoryPool *, //memory_pool,
	HMUlCr *, //phmulcr,
	BOOL //fMustExist
	)
{
	return PopCopyDefault();
}

// EOF

