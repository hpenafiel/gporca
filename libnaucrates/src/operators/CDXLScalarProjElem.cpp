//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarProjElem.cpp
//
//	@doc:
//		Implementation of DXL projection list element operators
//---------------------------------------------------------------------------
#include "naucrates/dxl/operators/CDXLScalarProjElem.h"

#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarProjElem::CDXLScalarProjElem
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLScalarProjElem::CDXLScalarProjElem
	(
	IMemoryPool *memory_pool,
	ULONG ulId,
	const CMDName *pmdname
	)
	:
	CDXLScalar(memory_pool),
	m_ulId(ulId),
	m_mdname(pmdname)
{
	GPOS_ASSERT(NULL != pmdname);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarProjElem::~CDXLScalarProjElem
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLScalarProjElem::~CDXLScalarProjElem()
{
	GPOS_DELETE(m_mdname);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarProjElem::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarProjElem::Edxlop() const
{
	return EdxlopScalarProjectElem;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarProjElem::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarProjElem::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarProjElem);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarProjElem::UlId
//
//	@doc:
//		Col id for this project element
//
//---------------------------------------------------------------------------
ULONG
CDXLScalarProjElem::UlId() const
{
	return m_ulId;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarProjElem::PmdnameAlias
//
//	@doc:
//		Alias
//
//---------------------------------------------------------------------------
const CMDName *
CDXLScalarProjElem::PmdnameAlias() const
{
	return m_mdname;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarProjElem::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarProjElem::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
		
	// serialize proj elem id
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenColId), m_ulId);
		
	// serialize proj element alias
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenAlias), m_mdname->Pstr());
	
	pdxln->SerializeChildrenToDXL(xml_serializer);
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarProjElem::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarProjElem::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL fValidateChildren 
	) 
	const
{
	GPOS_ASSERT(1 == pdxln->Arity());
	CDXLNode *pdxlnChild = (*pdxln)[0];
	
	GPOS_ASSERT(EdxloptypeScalar == pdxlnChild->Pdxlop()->Edxloperatortype());

	if (fValidateChildren)
	{
		pdxlnChild->Pdxlop()->AssertValid(pdxlnChild, fValidateChildren);
	}
}

#endif // GPOS_DEBUG


// EOF
