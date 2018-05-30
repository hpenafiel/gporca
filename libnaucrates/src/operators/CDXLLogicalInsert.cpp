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
	m_pdxltabdesc(pdxltabdesc),
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
	m_pdxltabdesc->Release();
	m_pdrgpul->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalInsert::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalInsert::Edxlop() const
{
	return EdxlopLogicalInsert;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalInsert::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalInsert::PstrOpName() const
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
	const CWStringConst *pstrElemName = PstrOpName();
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);

	CWStringDynamic *pstrCols = CDXLUtils::Serialize(m_memory_pool, m_pdrgpul);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenInsertCols), pstrCols);
	GPOS_DELETE(pstrCols);

	// serialize table descriptor
	m_pdxltabdesc->SerializeToDXL(xml_serializer);
	
	// serialize arguments
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);
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
	BOOL fValidateChildren
	) 
	const
{
	GPOS_ASSERT(1 == pdxln->Arity());

	CDXLNode *pdxlnChild = (*pdxln)[0];
	GPOS_ASSERT(EdxloptypeLogical == pdxlnChild->Pdxlop()->Edxloperatortype());

	if (fValidateChildren)
	{
		pdxlnChild->Pdxlop()->AssertValid(pdxlnChild, fValidateChildren);
	}
}

#endif // GPOS_DEBUG


// EOF
