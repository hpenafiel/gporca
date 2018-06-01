//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CDXLScalarCoerceToDomain.cpp
//
//	@doc:
//		Implementation of DXL scalar coerce
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarCoerceToDomain.h"
#include "naucrates/dxl/xml/dxltokens.h"

using namespace gpopt;
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCoerceToDomain::CDXLScalarCoerceToDomain
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarCoerceToDomain::CDXLScalarCoerceToDomain
	(
	IMemoryPool *memory_pool,
	IMDId *mdid_type,
	INT type_modifier,
	EdxlCoercionForm edxlcf,
	INT iLoc
	)
	:
	CDXLScalarCoerceBase(memory_pool, mdid_type, type_modifier, edxlcf, iLoc)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCoerceToDomain::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarCoerceToDomain::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarCoerceToDomain);
}

// EOF
