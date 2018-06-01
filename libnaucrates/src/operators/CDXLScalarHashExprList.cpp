//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarHashExprList.cpp
//
//	@doc:
//		Implementation of DXL hash expression lists for redistribute operators
//---------------------------------------------------------------------------
#include "naucrates/dxl/operators/CDXLScalarHashExprList.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarHashExprList::CDXLScalarHashExprList
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLScalarHashExprList::CDXLScalarHashExprList
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLScalar(memory_pool)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarHashExprList::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarHashExprList::Edxlop() const
{
	return EdxlopScalarHashExprList;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarHashExprList::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarHashExprList::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarHashExprList);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarHashExprList::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarHashExprList::SerializeToDXL
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
//		CDXLScalarHashExprList::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarHashExprList::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children 
	) 
	const
{
	const ULONG ulArity = pdxln->Arity();
	GPOS_ASSERT(1 <= ulArity);

	for (ULONG ul = 0; ul < ulArity; ul++)
	{
		CDXLNode *child_dxlnode = (*pdxln)[ul];
		GPOS_ASSERT(EdxlopScalarHashExpr == child_dxlnode->GetOperator()->Edxlop());
		
		if (validate_children)
		{
			child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
		}
	}
}

#endif // GPOS_DEBUG



// EOF
