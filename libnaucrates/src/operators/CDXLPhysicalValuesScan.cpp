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
CDXLPhysicalValuesScan::Edxlop() const
{
	return EdxlopPhysicalValuesScan;
}

// operator name
const CWStringConst *
CDXLPhysicalValuesScan::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalValuesScan);
}

CDXLPhysicalValuesScan *
CDXLPhysicalValuesScan::PdxlopConvert
	(
	CDXLOperator *dxl_op
	)
{
	GPOS_ASSERT(NULL != dxl_op);
	GPOS_ASSERT(EdxlopPhysicalValuesScan ==dxl_op->Edxlop());

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
	const CWStringConst *element_name = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);

	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
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
	GPOS_ASSERT(EdxloptypePhysical == pdxln->GetOperator()->Edxloperatortype());

	const ULONG ulArity = pdxln->Arity();
	GPOS_ASSERT(EdxlValIndexSentinel <= ulArity);

	for (ULONG ul = 0; ul < ulArity; ul++)
	{
		CDXLNode *child_dxlnode = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeScalar == child_dxlnode->GetOperator()->Edxloperatortype());
		if (validate_children)
		{
			child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
		}
	}
}
#endif // GPOS_DEBUG

// EOF
