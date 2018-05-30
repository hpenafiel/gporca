//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 Pivotal, Inc.
//
//	@filename:
//		CDXLLogicalExternalGet.cpp
//
//	@doc:
//		Implementation of DXL logical external get operator
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLLogicalExternalGet.h"
#include "naucrates/dxl/xml/dxltokens.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalExternalGet::CDXLLogicalExternalGet
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLLogicalExternalGet::CDXLLogicalExternalGet
	(
	IMemoryPool *memory_pool,
	CDXLTableDescr *pdxltabdesc
	)
	:
	CDXLLogicalGet(memory_pool, pdxltabdesc)
{}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalExternalGet::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalExternalGet::Edxlop() const
{
	return EdxlopLogicalExternalGet;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalExternalGet::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalExternalGet::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenLogicalExternalGet);
}

// EOF
