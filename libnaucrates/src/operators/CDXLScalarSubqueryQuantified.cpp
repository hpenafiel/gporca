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
	IMDId *pmdidScalarOp,
	CMDName *pmdnameScalarOp,
	ULONG ulColId
	)
	:
	CDXLScalar(memory_pool),
	m_pmdidScalarOp(pmdidScalarOp),
	m_pmdnameScalarOp(pmdnameScalarOp),
	m_ulColId(ulColId)
{
	GPOS_ASSERT(pmdidScalarOp->IsValid());
	GPOS_ASSERT(NULL != pmdnameScalarOp);
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
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	// serialize operator id and name
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenOpName), m_pmdnameScalarOp->Pstr());
	m_pmdidScalarOp->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenOpNo));

	// serialize computed column id
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenColId), m_ulColId);

	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
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
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	GPOS_ASSERT(2 == pdxln->Arity());

	CDXLNode *pdxlnScalarChild = (*pdxln)[EdxlsqquantifiedIndexScalar];
	CDXLNode *pdxlnRelationalChild = (*pdxln)[EdxlsqquantifiedIndexRelational];

	GPOS_ASSERT(EdxloptypeScalar == pdxlnScalarChild->GetOperator()->GetDXLOperatorType());
	GPOS_ASSERT(EdxloptypeLogical == pdxlnRelationalChild->GetOperator()->GetDXLOperatorType());

	pdxln->AssertValid(validate_children);
}
#endif // GPOS_DEBUG

// EOF
