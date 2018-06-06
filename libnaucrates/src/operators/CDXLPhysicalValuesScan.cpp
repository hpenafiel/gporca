//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 Pivotal Software, Inc.
//
//	@filename:
//		CDXLPhysicalValuesScan.cpp
//
//	@doc:
//		Implementation of DXL physical values scan operator
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLPhysicalValuesScan.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

// ctor
CDXLPhysicalValuesScan::CDXLPhysicalValuesScan
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLPhysical(memory_pool)
{}

// dtor
CDXLPhysicalValuesScan::~CDXLPhysicalValuesScan
	(
	)
{}

// operator type
Edxlopid
CDXLPhysicalValuesScan::GetDXLOperator() const
{
	return EdxlopPhysicalValuesScan;
}

// operator name
const CWStringConst *
CDXLPhysicalValuesScan::GetOpNameStr() const
{
	return CDXLTokens::GetDXLTokenStr(EdxltokenPhysicalValuesScan);
}

CDXLPhysicalValuesScan *
CDXLPhysicalValuesScan::Cast
	(
	CDXLOperator *dxl_op
	)
{
	GPOS_ASSERT(NULL != dxl_op);
	GPOS_ASSERT(EdxlopPhysicalValuesScan ==dxl_op->GetDXLOperator());

	return dynamic_cast<CDXLPhysicalValuesScan *>(dxl_op);
}
// serialize operator in DXL format
void
CDXLPhysicalValuesScan::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);

	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG

// checks whether operator node is well-structured
void
CDXLPhysicalValuesScan::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	)
const
{
	GPOS_ASSERT(EdxloptypePhysical == pdxln->GetOperator()->GetDXLOperatorType());

	const ULONG arity = pdxln->Arity();
	GPOS_ASSERT(EdxlValIndexSentinel <= arity);

	for (ULONG ul = 0; ul < arity; ul++)
	{
		CDXLNode *child_dxlnode = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeScalar == child_dxlnode->GetOperator()->GetDXLOperatorType());
		if (validate_children)
		{
			child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
		}
	}
}
#endif // GPOS_DEBUG

// EOF
