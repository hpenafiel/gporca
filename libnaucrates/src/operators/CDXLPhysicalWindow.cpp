//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalWindow.cpp
//
//	@doc:
//		Implementation of DXL physical window operators
//---------------------------------------------------------------------------

#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/operators/CDXLPhysicalWindow.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalWindow::CDXLPhysicalWindow
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLPhysicalWindow::CDXLPhysicalWindow
	(
	IMemoryPool *memory_pool,
	ULongPtrArray *pdrgpulPartCols,
	CDXLWindowKeyArray *pdrgpdxlwk
	)
	:
	CDXLPhysical(memory_pool),
	m_pdrgpulPartCols(pdrgpulPartCols),
	m_pdrgpdxlwk(pdrgpdxlwk)
{
	GPOS_ASSERT(NULL != m_pdrgpulPartCols);
	GPOS_ASSERT(NULL != m_pdrgpdxlwk);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalWindow::~CDXLPhysicalWindow
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLPhysicalWindow::~CDXLPhysicalWindow()
{
	m_pdrgpulPartCols->Release();
	m_pdrgpdxlwk->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalWindow::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalWindow::Edxlop() const
{
	return EdxlopPhysicalWindow;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalWindow::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalWindow::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalWindow);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalWindow::UlPartCols
//
//	@doc:
//		Returns the number of partition columns
//
//---------------------------------------------------------------------------
ULONG
CDXLPhysicalWindow::UlPartCols() const
{
	return m_pdrgpulPartCols->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalWindow::UlWindowKeys
//
//	@doc:
//		Returns the number of window keys
//
//---------------------------------------------------------------------------
ULONG
CDXLPhysicalWindow::UlWindowKeys() const
{
	return m_pdrgpdxlwk->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalWindow::PdxlWindowKey
//
//	@doc:
//		Return the window key at a given position
//
//---------------------------------------------------------------------------
CDXLWindowKey *
CDXLPhysicalWindow::PdxlWindowKey
	(
	ULONG ulPos
	)
	const
{
	GPOS_ASSERT(ulPos <= m_pdrgpdxlwk->Size());
	return (*m_pdrgpdxlwk)[ulPos];
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalWindow::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalWindow::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	// serialize partition keys
	CWStringDynamic *pstrPartCols = CDXLUtils::Serialize(m_memory_pool, m_pdrgpulPartCols);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenPartKeys), pstrPartCols);
	GPOS_DELETE(pstrPartCols);

	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);

	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);

	// serialize the list of window keys
	const CWStringConst *pstrWindowKeyList = CDXLTokens::PstrToken(EdxltokenWindowKeyList);
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrWindowKeyList);
	const ULONG ulSize = m_pdrgpdxlwk->Size();
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CDXLWindowKey *pdxlwk = (*m_pdrgpdxlwk)[ul];
		pdxlwk->SerializeToDXL(xml_serializer);
	}
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrWindowKeyList);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalWindow::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLPhysicalWindow::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	// assert proj list and filter are valid
	CDXLPhysical::AssertValid(pdxln, validate_children);
	GPOS_ASSERT(NULL != m_pdrgpulPartCols);
	GPOS_ASSERT(NULL != m_pdrgpdxlwk);
	GPOS_ASSERT(EdxlwindowIndexSentinel == pdxln->Arity());
	CDXLNode *child_dxlnode = (*pdxln)[EdxlwindowIndexChild];
	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
