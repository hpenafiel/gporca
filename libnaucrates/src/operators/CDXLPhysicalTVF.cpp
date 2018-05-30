//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalTVF.cpp
//
//	@doc:
//		Implementation of DXL physical table-valued function
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/operators/CDXLPhysicalTVF.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalTVF::CDXLPhysicalTVF
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLPhysicalTVF::CDXLPhysicalTVF
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidFunc,
	IMDId *pmdidRetType,
	CWStringConst *pstr
	)
	:
	CDXLPhysical(memory_pool),
	m_pmdidFunc(pmdidFunc),
	m_pmdidRetType(pmdidRetType),
	m_pstr(pstr)
{
	GPOS_ASSERT(NULL != m_pmdidFunc);
	GPOS_ASSERT(m_pmdidFunc->IsValid());
	GPOS_ASSERT(NULL != m_pmdidRetType);
	GPOS_ASSERT(m_pmdidRetType->IsValid());
	GPOS_ASSERT(NULL != m_pstr);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalTVF::~CDXLPhysicalTVF
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLPhysicalTVF::~CDXLPhysicalTVF()
{
	m_pmdidFunc->Release();
	m_pmdidRetType->Release();
	GPOS_DELETE(m_pstr);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalTVF::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalTVF::Edxlop() const
{
	return EdxlopPhysicalTVF;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalTVF::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalTVF::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalTVF);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalTVF::SerializeToDXL
//
//	@doc:
//		Serialize function descriptor in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalTVF::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *pstrElemName = PstrOpName();
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);
	m_pmdidFunc->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenFuncId));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenName), m_pstr);
	m_pmdidRetType->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenTypeId));

	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);

	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalTVF::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLPhysicalTVF::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL fValidateChildren
	)
	const
{
	// assert validity of function id and return type
	GPOS_ASSERT(NULL != m_pmdidFunc);
	GPOS_ASSERT(NULL != m_pmdidRetType);

	const ULONG ulArity = pdxln->UlArity();
	for (ULONG ul = 0; ul < ulArity; ++ul)
	{
		CDXLNode *pdxlnArg = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeScalar == pdxlnArg->Pdxlop()->Edxloperatortype());

		if (fValidateChildren)
		{
			pdxlnArg->Pdxlop()->AssertValid(pdxlnArg, fValidateChildren);
		}
	}
}

#endif // GPOS_DEBUG


// EOF
