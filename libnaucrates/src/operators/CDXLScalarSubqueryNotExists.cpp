//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC, Corp.
//
//	@filename:
//		CDXLScalarSubqueryNotExists.cpp
//
//	@doc:
//		Implementation of NOT EXISTS subqueries
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/operators/CDXLScalarSubqueryNotExists.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubqueryNotExists::CDXLScalarSubqueryNotExists
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLScalarSubqueryNotExists::CDXLScalarSubqueryNotExists
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLScalar(memory_pool)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubqueryNotExists::~CDXLScalarSubqueryNotExists
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLScalarSubqueryNotExists::~CDXLScalarSubqueryNotExists()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubqueryNotExists::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarSubqueryNotExists::GetDXLOperator() const
{
	return EdxlopScalarSubqueryNotExists;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubqueryNotExists::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarSubqueryNotExists::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarSubqueryNotExists);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubqueryNotExists::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarSubqueryNotExists::SerializeToDXL
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
//		CDXLScalarSubqueryNotExists::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLScalarSubqueryNotExists::AssertValid
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
