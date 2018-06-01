//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC, Corp.
//
//	@filename:
//		CDXLScalarSubqueryExists.cpp
//
//	@doc:
//		Implementation of EXISTS subqueries
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/operators/CDXLScalarSubqueryExists.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubqueryExists::CDXLScalarSubqueryExists
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLScalarSubqueryExists::CDXLScalarSubqueryExists
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLScalar(memory_pool)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubqueryExists::~CDXLScalarSubqueryExists
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLScalarSubqueryExists::~CDXLScalarSubqueryExists()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubqueryExists::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarSubqueryExists::Edxlop() const
{
	return EdxlopScalarSubqueryExists;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubqueryExists::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarSubqueryExists::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarSubqueryExists);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubqueryExists::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarSubqueryExists::SerializeToDXL
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
//		CDXLScalarSubqueryExists::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLScalarSubqueryExists::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	GPOS_ASSERT(1 == pdxln->Arity());
	
	CDXLNode *child_dxlnode = (*pdxln)[0];
	GPOS_ASSERT(EdxloptypeLogical == child_dxlnode->GetOperator()->Edxloperatortype());

	pdxln->AssertValid(validate_children);
}
#endif // GPOS_DEBUG

// EOF
