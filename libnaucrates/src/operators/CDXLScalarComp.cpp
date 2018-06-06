//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarComp.cpp
//
//	@doc:
//		Implementation of DXL comparison operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLScalarComp.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarComp::CDXLScalarComp
//
//	@doc:
//		Constructs a scalar comparison node
//
//---------------------------------------------------------------------------
CDXLScalarComp::CDXLScalarComp
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidOp,
	const CWStringConst *comparison_operator_name
	)
	:
	CDXLScalar(memory_pool),
	m_mdid(pmdidOp),
	m_comparison_operator_name(comparison_operator_name)
{
	GPOS_ASSERT(m_mdid->IsValid());
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarComp::~CDXLScalarComp
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLScalarComp::~CDXLScalarComp()
{
	m_mdid->Release();
	GPOS_DELETE(m_comparison_operator_name);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarComp::PstrCmpOpName
//
//	@doc:
//		Comparison operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarComp::GetComparisonOpName() const
{
	return m_comparison_operator_name;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarComp::MDId
//
//	@doc:
//		Comparison operator id
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarComp::MDId() const
{
	return m_mdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarComp::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarComp::GetDXLOperator() const
{
	return EdxlopScalarCmp;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarComp::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarComp::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarComp);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarComp::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarComp::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *node
	)
	const
{
	GPOS_CHECK_ABORT;

	const CWStringConst *element_name = GetOpNameStr();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenComparisonOp), GetComparisonOpName());

	m_mdid->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenOpNo));
	
	node->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	GPOS_CHECK_ABORT;
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarComp::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarComp::AssertValid
	(
	const CDXLNode *node,
	BOOL validate_children
	) 
	const
{
	const ULONG ulArity = node->Arity();
	GPOS_ASSERT(2 == ulArity);

	for (ULONG ul = 0; ul < ulArity; ++ul)
	{
		CDXLNode *child_dxlnode = (*node)[ul];
		GPOS_ASSERT(EdxloptypeScalar == child_dxlnode->GetOperator()->GetDXLOperatorType() ||
					EdxloptypeLogical == child_dxlnode->GetOperator()->GetDXLOperatorType());
		
		if (validate_children)
		{
			child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
		}
	}
}
#endif // GPOS_DEBUG





// EOF
