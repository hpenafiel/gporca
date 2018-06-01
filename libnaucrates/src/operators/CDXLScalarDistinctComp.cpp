//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarDistinctComp.cpp
//
//	@doc:
//		Implementation of DXL "is distinct from" comparison operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLScalarDistinctComp.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarDistinctComp::CDXLScalarDistinctComp
//
//	@doc:
//		Constructs a scalar distinct comparison node
//
//---------------------------------------------------------------------------
CDXLScalarDistinctComp::CDXLScalarDistinctComp
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidOp
	)
	:
	CDXLScalarComp(memory_pool, pmdidOp, GPOS_NEW(memory_pool) CWStringConst(memory_pool, CDXLTokens::PstrToken(EdxltokenEq)->GetBuffer()))
{
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarDistinctComp::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarDistinctComp::GetDXLOperator() const
{
	return EdxlopScalarDistinct;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarDistinctComp::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarDistinctComp::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarDistinctComp);;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarDistinctComp::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarDistinctComp::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	m_mdid->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenOpNo));
	
	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);	
}



#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarDistinctComp::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLScalarDistinctComp::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{	
	GPOS_ASSERT(EdxlscdistcmpSentinel == pdxln->Arity());
	
	CDXLNode *pdxlnLeft = (*pdxln)[EdxlscdistcmpIndexLeft];
	CDXLNode *pdxlnRight = (*pdxln)[EdxlscdistcmpIndexRight];
	
	// assert children are of right type (scalar)
	GPOS_ASSERT(EdxloptypeScalar == pdxlnLeft->GetOperator()->GetDXLOperatorType());
	GPOS_ASSERT(EdxloptypeScalar == pdxlnRight->GetOperator()->GetDXLOperatorType());
	
	GPOS_ASSERT(PstrCmpOpName()->Equals(CDXLTokens::PstrToken(EdxltokenEq)));
	
	if (validate_children)
	{
		pdxlnLeft->GetOperator()->AssertValid(pdxlnLeft, validate_children);
		pdxlnRight->GetOperator()->AssertValid(pdxlnRight, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
