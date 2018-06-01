//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarHashExpr.cpp
//
//	@doc:
//		Implementation of DXL hash expressions for redistribute operators
//---------------------------------------------------------------------------
#include "naucrates/dxl/operators/CDXLScalarHashExpr.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarHashExpr::CDXLScalarHashExpr
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLScalarHashExpr::CDXLScalarHashExpr
	(
	IMemoryPool *memory_pool,
	IMDId *mdid_type
	)
	:
	CDXLScalar(memory_pool),
	m_mdid_type(mdid_type)
{
	GPOS_ASSERT(m_mdid_type->IsValid());
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarHashExpr::~CDXLScalarHashExpr
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLScalarHashExpr::~CDXLScalarHashExpr()
{
	m_mdid_type->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarHashExpr::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarHashExpr::Edxlop() const
{
	return EdxlopScalarHashExpr;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarHashExpr::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarHashExpr::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarHashExpr);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarHashExpr::MDIdType
//
//	@doc:
//		Hash expression type from the catalog
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarHashExpr::MDIdType() const
{
	return m_mdid_type;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarHashExpr::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarHashExpr::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	m_mdid_type->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenTypeId));

	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarHashExpr::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarHashExpr::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children 
	) 
	const
{
	GPOS_ASSERT(1 == pdxln->Arity());
	CDXLNode *child_dxlnode = (*pdxln)[0];
	
	GPOS_ASSERT(EdxloptypeScalar == child_dxlnode->GetOperator()->Edxloperatortype());

	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}

#endif // GPOS_DEBUG


// EOF
