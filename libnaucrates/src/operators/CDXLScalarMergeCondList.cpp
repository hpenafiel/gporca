//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarMergeCondList.cpp
//
//	@doc:
//		Implementation of DXL merge condition lists for merge join operators
//---------------------------------------------------------------------------
#include "naucrates/dxl/operators/CDXLScalarMergeCondList.h"

#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarMergeCondList::CDXLScalarMergeCondList
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLScalarMergeCondList::CDXLScalarMergeCondList
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLScalar(memory_pool)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarMergeCondList::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarMergeCondList::GetDXLOperator() const
{
	return EdxlopScalarMergeCondList;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarMergeCondList::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarMergeCondList::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarMergeCondList);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarMergeCondList::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarMergeCondList::SerializeToDXL
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
//		CDXLScalarMergeCondList::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLScalarMergeCondList::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	GPOS_ASSERT(NULL != pdxln);
	
	const ULONG ulArity = pdxln->Arity();
	for (ULONG ul = 0; ul < ulArity; ul++)
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
