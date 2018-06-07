//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC, Corp.
//
//	@filename:
//		CDXLScalarSubqueryQuantified.cpp
//
//	@doc:
//		Implementation of quantified subquery operator
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/operators/CDXLScalarSubqueryQuantified.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubqueryQuantified::CDXLScalarSubqueryQuantified
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLScalarSubqueryQuantified::CDXLScalarSubqueryQuantified
	(
	IMemoryPool *memory_pool,
	IMDId *scalar_op_mdid,
	CMDName *scalar_op_mdname,
	ULONG col_id
	)
	:
	CDXLScalar(memory_pool),
	m_pmdidScalarOp(scalar_op_mdid),
	m_pmdnameScalarOp(scalar_op_mdname),
	m_colid(col_id)
{
	GPOS_ASSERT(scalar_op_mdid->IsValid());
	GPOS_ASSERT(NULL != scalar_op_mdname);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubqueryQuantified::~CDXLScalarSubqueryQuantified
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLScalarSubqueryQuantified::~CDXLScalarSubqueryQuantified()
{
	m_pmdidScalarOp->Release();
	GPOS_DELETE(m_pmdnameScalarOp);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubqueryQuantified::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarSubqueryQuantified::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *dxlnode
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);

	// serialize operator id and name
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenOpName), m_pmdnameScalarOp->GetMDName());
	m_pmdidScalarOp->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenOpNo));

	// serialize computed column id
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenColId), m_colid);

	dxlnode->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubqueryQuantified::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarSubqueryQuantified::AssertValid
	(
	const CDXLNode *dxlnode,
	BOOL validate_children
	)
	const
{
	GPOS_ASSERT(2 == dxlnode->Arity());

	CDXLNode *pdxlnScalarChild = (*dxlnode)[EdxlsqquantifiedIndexScalar];
	CDXLNode *pdxlnRelationalChild = (*dxlnode)[EdxlsqquantifiedIndexRelational];

	GPOS_ASSERT(EdxloptypeScalar == pdxlnScalarChild->GetOperator()->GetDXLOperatorType());
	GPOS_ASSERT(EdxloptypeLogical == pdxlnRelationalChild->GetOperator()->GetDXLOperatorType());

	dxlnode->AssertValid(validate_children);
}
#endif // GPOS_DEBUG

// EOF
