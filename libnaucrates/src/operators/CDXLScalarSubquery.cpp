//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC, Corp.
//
//	@filename:
//		CDXLScalarSubquery.cpp
//
//	@doc:
//		Implementation of subqueries computing scalar values
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/operators/CDXLScalarSubquery.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubquery::CDXLScalarSubquery
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLScalarSubquery::CDXLScalarSubquery
	(
	IMemoryPool *memory_pool,
	ULONG ulColId
	)
	:
	CDXLScalar(memory_pool),
	m_ulColId(ulColId)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubquery::~CDXLScalarSubquery
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLScalarSubquery::~CDXLScalarSubquery()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubquery::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarSubquery::GetDXLOperator() const
{
	return EdxlopScalarSubquery;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubquery::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarSubquery::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarSubquery);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubquery::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarSubquery::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	// serialize computed column id
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenColId), m_ulColId);

	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubquery::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLScalarSubquery::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	GPOS_ASSERT(1 == pdxln->Arity());
	
	CDXLNode *child_dxlnode = (*pdxln)[0];
	GPOS_ASSERT(EdxloptypeLogical == child_dxlnode->GetOperator()->GetDXLOperatorType());

	pdxln->AssertValid(validate_children);
}
#endif // GPOS_DEBUG

// EOF
