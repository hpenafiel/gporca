//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLLogicalProject.cpp
//
//	@doc:
//		Implementation of DXL logical project operator
//		
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLLogicalProject.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalProject::CDXLLogicalProject
//
//	@doc:
//		Construct a DXL Logical project node
//
//---------------------------------------------------------------------------
CDXLLogicalProject::CDXLLogicalProject
	(
	IMemoryPool *memory_pool
	)
	:CDXLLogical(memory_pool),
	 m_pmdnameAlias(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalProject::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalProject::Edxlop() const
{
	return EdxlopLogicalProject;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalProject::MdName
//
//	@doc:
//		Returns alias name
//
//---------------------------------------------------------------------------
const CMDName *
CDXLLogicalProject::MdName() const
{
	return m_pmdnameAlias;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalProject::SetAliasName
//
//	@doc:
//		Set alias name
//
//---------------------------------------------------------------------------
void
CDXLLogicalProject::SetAliasName
	(
	CMDName *mdname
	)
{
	GPOS_ASSERT(NULL == m_pmdnameAlias);
	GPOS_ASSERT(NULL != mdname);

	m_pmdnameAlias = mdname;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalProject::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalProject::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenLogicalProject);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalProject::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLLogicalProject::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	// serialize alias
	if (NULL != m_pmdnameAlias)
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenDerivedTableName), m_pmdnameAlias->Pstr());
	}

	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalProject::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLLogicalProject::AssertValid
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
}
#endif // GPOS_DEBUG

// EOF
