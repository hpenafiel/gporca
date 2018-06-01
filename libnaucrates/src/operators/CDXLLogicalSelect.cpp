//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLLogicalSelect.cpp
//
//	@doc:
//		Implementation of DXL logical select operator
//		
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLLogicalSelect.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalSelect::CDXLLogicalSelect
//
//	@doc:
//		Construct a DXL Logical select node
//
//---------------------------------------------------------------------------
CDXLLogicalSelect::CDXLLogicalSelect
	(
	IMemoryPool *memory_pool
	)
	:CDXLLogical(memory_pool)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalSelect::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalSelect::Edxlop() const
{
	return EdxlopLogicalSelect;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalSelect::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalSelect::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenLogicalSelect);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalSelect::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLLogicalSelect::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalSelect::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLLogicalSelect::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) const
{
	GPOS_ASSERT(2 == pdxln->Arity());

	CDXLNode *pdxlnCond = (*pdxln)[0];
	CDXLNode *child_dxlnode = (*pdxln)[1];

	GPOS_ASSERT(EdxloptypeScalar ==  pdxlnCond->GetOperator()->Edxloperatortype());
	GPOS_ASSERT(EdxloptypeLogical == child_dxlnode->GetOperator()->Edxloperatortype());
	
	if (validate_children)
	{
		pdxlnCond->GetOperator()->AssertValid(pdxlnCond, validate_children);
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
