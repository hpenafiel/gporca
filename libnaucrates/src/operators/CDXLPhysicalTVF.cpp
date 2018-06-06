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
	m_func_mdid(pmdidFunc),
	m_return_type_mdid(pmdidRetType),
	m_pstr(pstr)
{
	GPOS_ASSERT(NULL != m_func_mdid);
	GPOS_ASSERT(m_func_mdid->IsValid());
	GPOS_ASSERT(NULL != m_return_type_mdid);
	GPOS_ASSERT(m_return_type_mdid->IsValid());
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
	m_func_mdid->Release();
	m_return_type_mdid->Release();
	GPOS_DELETE(m_pstr);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalTVF::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalTVF::GetDXLOperator() const
{
	return EdxlopPhysicalTVF;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalTVF::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalTVF::GetOpNameStr() const
{
	return CDXLTokens::GetDXLTokenStr(EdxltokenPhysicalTVF);
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
	const CWStringConst *element_name = GetOpNameStr();
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
	m_func_mdid->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenFuncId));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenName), m_pstr);
	m_return_type_mdid->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenTypeId));

	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);

	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
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
	BOOL validate_children
	)
	const
{
	// assert validity of function id and return type
	GPOS_ASSERT(NULL != m_func_mdid);
	GPOS_ASSERT(NULL != m_return_type_mdid);

	const ULONG ulArity = pdxln->Arity();
	for (ULONG ul = 0; ul < ulArity; ++ul)
	{
		CDXLNode *pdxlnArg = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeScalar == pdxlnArg->GetOperator()->GetDXLOperatorType());

		if (validate_children)
		{
			pdxlnArg->GetOperator()->AssertValid(pdxlnArg, validate_children);
		}
	}
}

#endif // GPOS_DEBUG


// EOF
