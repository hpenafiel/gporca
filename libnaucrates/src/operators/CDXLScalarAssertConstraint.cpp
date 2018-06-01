//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2015 Pivotal Inc.
//
//	@filename:
//		CDXLScalarAssertConstraint.cpp
//
//	@doc:
//		Implementation of DXL scalar assert predicate
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarAssertConstraint.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarAssertConstraint::CDXLScalarAssertConstraint
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarAssertConstraint::CDXLScalarAssertConstraint
	(
	IMemoryPool *memory_pool,
	CWStringBase *pstrErrorMsg
	)
	:
	CDXLScalar(memory_pool),
	m_pstrErrorMsg(pstrErrorMsg)
{
	GPOS_ASSERT(NULL != pstrErrorMsg);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarAssertConstraint::~CDXLScalarAssertConstraint
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLScalarAssertConstraint::~CDXLScalarAssertConstraint()
{
	GPOS_DELETE(m_pstrErrorMsg);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarAssertConstraint::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarAssertConstraint::GetDXLOperator() const
{
	return EdxlopScalarAssertConstraint;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarAssertConstraint::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarAssertConstraint::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarAssertConstraint);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarAssertConstraint::PstrErrorMsg
//
//	@doc:
//		Error message
//
//---------------------------------------------------------------------------
CWStringBase *
CDXLScalarAssertConstraint::PstrErrorMsg() const
{
	return m_pstrErrorMsg;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarAssertConstraint::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarAssertConstraint::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenErrorMessage), m_pstrErrorMsg);
		
	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}


#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarAssertConstraint::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarAssertConstraint::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	GPOS_ASSERT(1 == pdxln->Arity());
	
	CDXLNode *child_dxlnode = (*pdxln)[0];
	GPOS_ASSERT(EdxloptypeScalar == child_dxlnode->GetOperator()->GetDXLOperatorType());

	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}

#endif // GPOS_DEBUG

// EOF
