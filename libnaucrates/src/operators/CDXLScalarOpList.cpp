//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CDXLScalarOpList.cpp
//
//	@doc:
//		Implementation of DXL list of scalar expressions
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarOpList.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarOpList::CDXLScalarOpList
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarOpList::CDXLScalarOpList
	(
	IMemoryPool *memory_pool,
	EdxlOpListType edxloplisttype
	)
	:
	CDXLScalar(memory_pool),
	m_edxloplisttype(edxloplisttype)
{
	GPOS_ASSERT(EdxloplistSentinel > edxloplisttype);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarOpList::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarOpList::GetDXLOperator() const
{
	return EdxlopScalarOpList;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarOpList::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarOpList::GetOpNameStr() const
{
	Edxltoken edxlt = EdxltokenSentinel;
	switch (m_edxloplisttype)
	{
		case EdxloplistEqFilterList:
			edxlt = EdxltokenPartLevelEqFilterList;
			break;

		case EdxloplistFilterList:
			edxlt = EdxltokenPartLevelFilterList;
			break;

		case EdxloplistGeneral:
			edxlt = EdxltokenScalarOpList;
			break;

		default:
			GPOS_ASSERT(!"Invalid op list type");
			break;
	}

	return CDXLTokens::GetDXLTokenStr(edxlt);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarOpList::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarOpList::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *dxlnode
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
	dxlnode->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarOpList::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarOpList::AssertValid
	(
	const CDXLNode *dxlnode,
	BOOL validate_children
	)
	const
{
	const ULONG arity = dxlnode->Arity();
	for (ULONG ul = 0; ul < arity; ++ul)
	{
		CDXLNode *child_dxlnode = (*dxlnode)[ul];
		GPOS_ASSERT(EdxloptypeScalar == child_dxlnode->GetOperator()->GetDXLOperatorType());

		if (validate_children)
		{
			child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
		}
	}
}
#endif // GPOS_DEBUG

// EOF
