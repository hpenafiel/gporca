//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLLogicalWindow.cpp
//
//	@doc:
//		Implementation of DXL logical window operator
//		
//	@owner: 
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/operators/CDXLLogicalWindow.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"


using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalWindow::CDXLLogicalWindow
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLLogicalWindow::CDXLLogicalWindow
	(
	IMemoryPool *memory_pool,
	DXLWindowSpecArray *pdrgpdxlws
	)
	:
	CDXLLogical(memory_pool),
	m_pdrgpdxlws(pdrgpdxlws)
{
	GPOS_ASSERT(NULL != m_pdrgpdxlws);
	GPOS_ASSERT(0 < m_pdrgpdxlws->Size());
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalWindow::~CDXLLogicalWindow
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLLogicalWindow::~CDXLLogicalWindow()
{
	m_pdrgpdxlws->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalWindow::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalWindow::Edxlop() const
{
	return EdxlopLogicalWindow;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalWindow::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalWindow::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenLogicalWindow);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalWindow::Pdxlws
//
//	@doc:
//		Return the window specification at a given position
//
//---------------------------------------------------------------------------
CDXLWindowSpec *
CDXLLogicalWindow::Pdxlws
	(
	ULONG ulPos
	)
	const
{
	GPOS_ASSERT(ulPos <= m_pdrgpdxlws->Size());
	return (*m_pdrgpdxlws)[ulPos];
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalWindow::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLLogicalWindow::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	// serialize the list of window specifications
	const CWStringConst *pstrWindowSpecList = CDXLTokens::PstrToken(EdxltokenWindowSpecList);
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrWindowSpecList);
	const ULONG ulSize = m_pdrgpdxlws->Size();
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CDXLWindowSpec *pdxlwinspec = (*m_pdrgpdxlws)[ul];
		pdxlwinspec->SerializeToDXL(xml_serializer);
	}
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrWindowSpecList);

	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalWindow::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLLogicalWindow::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) const
{
	GPOS_ASSERT(2 == pdxln->Arity());

	CDXLNode *pdxlnProjList = (*pdxln)[0];
	CDXLNode *child_dxlnode = (*pdxln)[1];

	GPOS_ASSERT(EdxlopScalarProjectList == pdxlnProjList->GetOperator()->Edxlop());
	GPOS_ASSERT(EdxloptypeLogical == child_dxlnode->GetOperator()->Edxloperatortype());

	if (validate_children)
	{
		pdxlnProjList->GetOperator()->AssertValid(pdxlnProjList, validate_children);
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}

	const ULONG ulArity = pdxlnProjList->Arity();
	for (ULONG ul = 0; ul < ulArity; ++ul)
	{
		CDXLNode *pdxlnPrEl = (*pdxlnProjList)[ul];
		GPOS_ASSERT(EdxlopScalarIdent != pdxlnPrEl->GetOperator()->Edxlop());
	}

	GPOS_ASSERT(NULL != m_pdrgpdxlws);
	GPOS_ASSERT(0 < m_pdrgpdxlws->Size());
}

#endif // GPOS_DEBUG

// EOF
