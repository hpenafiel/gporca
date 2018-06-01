//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CDXLScalarArrayRefIndexList.cpp
//
//	@doc:
//		Implementation of DXL arrayref index list
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarArrayRefIndexList.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayRefIndexList::CDXLScalarArrayRefIndexList
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarArrayRefIndexList::CDXLScalarArrayRefIndexList
	(
	IMemoryPool *memory_pool,
	EIndexListBound eilb
	)
	:
	CDXLScalar(memory_pool),
	m_eilb(eilb)
{
	GPOS_ASSERT(EilbSentinel > eilb);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayRefIndexList::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarArrayRefIndexList::Edxlop() const
{
	return EdxlopScalarArrayRefIndexList;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayRefIndexList::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarArrayRefIndexList::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarArrayRefIndexList);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayRefIndexList::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarArrayRefIndexList::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenScalarArrayRefIndexListBound), PstrIndexListBound(m_eilb));

	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayRefIndexList::PstrIndexListBound
//
//	@doc:
//		String representation of index list bound
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarArrayRefIndexList::PstrIndexListBound
	(
	EIndexListBound eilb
	)
{
	switch (eilb)
	{
		case EilbLower:
			return CDXLTokens::PstrToken(EdxltokenScalarArrayRefIndexListLower);

		case EilbUpper:
			return CDXLTokens::PstrToken(EdxltokenScalarArrayRefIndexListUpper);

		default:
			GPOS_ASSERT("Invalid array bound");
			return NULL;
	}
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayRefIndexList::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarArrayRefIndexList::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	const ULONG ulArity = pdxln->Arity();
	for (ULONG ul = 0; ul < ulArity; ++ul)
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
