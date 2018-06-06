//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLScalarSwitchCase.cpp
//
//	@doc:
//		Implementation of DXL Switch case
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarSwitchCase.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpopt;
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSwitchCase::CDXLScalarSwitchCase
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarSwitchCase::CDXLScalarSwitchCase
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLScalar(memory_pool)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSwitchCase::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarSwitchCase::GetDXLOperator() const
{
	return EdxlopScalarSwitchCase;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSwitchCase::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarSwitchCase::GetOpNameStr() const
{
	return CDXLTokens::GetDXLTokenStr(EdxltokenScalarSwitchCase);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSwitchCase::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarSwitchCase::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSwitchCase::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarSwitchCase::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	const ULONG ulArity = pdxln->Arity();
	GPOS_ASSERT(2 == ulArity);

	for (ULONG ul = 0; ul < ulArity; ++ul)
	{
		CDXLNode *dxlnode_arg = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeScalar == dxlnode_arg->GetOperator()->GetDXLOperatorType());

		if (validate_children)
		{
			dxlnode_arg->GetOperator()->AssertValid(dxlnode_arg, validate_children);
		}
	}
}
#endif // GPOS_DEBUG

// EOF
