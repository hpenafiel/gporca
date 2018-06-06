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
//		CDXLPhysicalWindow::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalWindow::GetDXLOperator() const
{
	return EdxlopPhysicalWindow;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalWindow::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalWindow::GetOpNameStr() const
{
	return CDXLTokens::GetDXLTokenStr(EdxltokenPhysicalWindow);
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
	const CDXLNode *dxlnode
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);

	// serialize partition keys
	CWStringDynamic *pstrPartCols = CDXLUtils::Serialize(m_memory_pool, m_pdrgpulPartCols);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenPartKeys), pstrPartCols);
	GPOS_DELETE(pstrPartCols);

	// serialize properties
	dxlnode->SerializePropertiesToDXL(xml_serializer);

	// serialize children
	dxlnode->SerializeChildrenToDXL(xml_serializer);

	// serialize the list of window keys
	const CWStringConst *pstrWindowKeyList = CDXLTokens::GetDXLTokenStr(EdxltokenWindowKeyList);
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), pstrWindowKeyList);
	const ULONG ulSize = m_pdrgpdxlwk->Size();
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CDXLWindowKey *pdxlwk = (*m_pdrgpdxlwk)[ul];
		pdxlwk->SerializeToDXL(xml_serializer);
	}
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), pstrWindowKeyList);

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
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
	const CDXLNode *dxlnode,
	BOOL validate_children
	)
	const
{
	// assert proj list and filter are valid
	CDXLPhysical::AssertValid(dxlnode, validate_children);
	GPOS_ASSERT(NULL != m_pdrgpulPartCols);
	GPOS_ASSERT(NULL != m_pdrgpdxlwk);
	GPOS_ASSERT(EdxlwindowIndexSentinel == dxlnode->Arity());
	CDXLNode *child_dxlnode = (*dxlnode)[EdxlwindowIndexChild];
	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
