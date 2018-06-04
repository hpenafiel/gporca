//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLLogicalInsert.cpp
//
//	@doc:
//		Implementation of DXL logical insert operator
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/operators/CDXLLogicalInsert.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalInsert::CDXLLogicalInsert
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLLogicalInsert::CDXLLogicalInsert
	(
	IMemoryPool *memory_pool,
	CDXLTableDescr *pdxltabdesc,
	ULongPtrArray *pdrgpul
	)
	:
	CDXLLogical(memory_pool),
	m_table_descr_dxl(pdxltabdesc),
	m_pdrgpul(pdrgpul)
{
	GPOS_ASSERT(NULL != pdxltabdesc);
	GPOS_ASSERT(NULL != pdrgpul);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalInsert::~CDXLLogicalInsert
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLLogicalInsert::~CDXLLogicalInsert()
{
	m_table_descr_dxl->Release();
	m_pdrgpul->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalInsert::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalInsert::GetDXLOperator() const
{
	return EdxlopLogicalInsert;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalInsert::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalInsert::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenLogicalInsert);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalInsert::SerializeToDXL
//
//	@doc:
//		Serialize function descriptor in DXL format
//
//---------------------------------------------------------------------------
void
CDXLLogicalInsert::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	CWStringDynamic *pstrCols = CDXLUtils::Serialize(m_memory_pool, m_pdrgpul);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenInsertCols), pstrCols);
	GPOS_DELETE(pstrCols);

	// serialize table descriptor
	m_table_descr_dxl->SerializeToDXL(xml_serializer);
	
	// serialize arguments
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalInsert::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLLogicalInsert::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	GPOS_ASSERT(1 == pdxln->Arity());

	CDXLNode *child_dxlnode = (*pdxln)[0];
	GPOS_ASSERT(EdxloptypeLogical == child_dxlnode->GetOperator()->GetDXLOperatorType());

	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}

#endif // GPOS_DEBUG


// EOF
