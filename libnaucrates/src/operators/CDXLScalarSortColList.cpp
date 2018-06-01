//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarSortColList.cpp
//
//	@doc:
//		Implementation of DXL sorting column lists for sort and motion operator nodes.
//---------------------------------------------------------------------------
#include "naucrates/dxl/operators/CDXLScalarSortColList.h"

#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSortColList::CDXLScalarSortColList
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLScalarSortColList::CDXLScalarSortColList
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLScalar(memory_pool)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSortColList::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarSortColList::GetDXLOperator() const
{
	return EdxlopScalarSortColList;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSortColList::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarSortColList::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarSortColList);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSortColList::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarSortColList::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSortColList::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLScalarSortColList::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	const ULONG ulArity = pdxln->Arity();
	for (ULONG ul = 0; ul < ulArity; ul++)
	{
		CDXLNode *child_dxlnode = (*pdxln)[ul];
		GPOS_ASSERT(EdxlopScalarSortCol == child_dxlnode->GetOperator()->GetDXLOperator());
		
		if (validate_children)
		{
			child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
		}
	}
}
#endif // GPOS_DEBUG

// EOF
