//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLScalarIndexCondList.cpp
//
//	@doc:
//		Implementation of DXL index condition lists for DXL index scan operator
//---------------------------------------------------------------------------
#include "naucrates/dxl/operators/CDXLScalarIndexCondList.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarIndexCondList::CDXLScalarIndexCondList
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CDXLScalarIndexCondList::CDXLScalarIndexCondList
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLScalar(memory_pool)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarIndexCondList::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarIndexCondList::Edxlop() const
{
	return EdxlopScalarIndexCondList;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarIndexCondList::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarIndexCondList::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarIndexCondList);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarIndexCondList::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarIndexCondList::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarIndexCondList::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarIndexCondList::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	GPOS_ASSERT(NULL != pdxln);

	if (validate_children)
	{
		const ULONG ulArity = pdxln->Arity();
		for (ULONG ul = 0; ul < ulArity; ul++)
		{
			CDXLNode *child_dxlnode = (*pdxln)[ul];
			GPOS_ASSERT(EdxloptypeScalar == child_dxlnode->GetOperator()->Edxloperatortype());
			child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
		}
	}
}
#endif // GPOS_DEBUG

// EOF
