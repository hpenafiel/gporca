//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLScalarDMLAction.cpp
//
//	@doc:
//		Implementation of DXL DML action expression
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarDMLAction.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpopt;
using namespace gpmd;
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarDMLAction::CDXLScalarDMLAction
//
//	@doc:
//		Constructs an action expression
//
//---------------------------------------------------------------------------
CDXLScalarDMLAction::CDXLScalarDMLAction
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLScalar(memory_pool)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarDMLAction::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarDMLAction::Edxlop() const
{
	return EdxlopScalarDMLAction;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarDMLAction::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarDMLAction::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarDMLAction);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarDMLAction::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarDMLAction::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode * // pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarDMLAction::FBoolean
//
//	@doc:
//		Does the operator return a boolean result
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarDMLAction::FBoolean
	(
	CMDAccessor * // pmda
	)
	const
{
	return false;
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarDMLAction::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarDMLAction::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL // validate_children
	) 
	const
{
	GPOS_ASSERT(0 == pdxln->Arity());
}
#endif // GPOS_DEBUG

// EOF
