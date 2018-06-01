//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLLogicalTVF.cpp
//
//	@doc:
//		Implementation of DXL table-valued function
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/operators/CDXLLogicalTVF.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalTVF::CDXLLogicalTVF
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLLogicalTVF::CDXLLogicalTVF
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidFunc,
	IMDId *pmdidRetType,
	CMDName *mdname,
	ColumnDescrDXLArray *pdrgdxlcd
	)
	:CDXLLogical(memory_pool),
	m_pmdidFunc(pmdidFunc),
	m_pmdidRetType(pmdidRetType),
	m_mdname(mdname),
	m_pdrgdxlcd(pdrgdxlcd)
{
	GPOS_ASSERT(m_pmdidFunc->IsValid());
	GPOS_ASSERT(m_pmdidRetType->IsValid());
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalTVF::~CDXLLogicalTVF
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLLogicalTVF::~CDXLLogicalTVF()
{
	m_pdrgdxlcd->Release();
	m_pmdidFunc->Release();
	m_pmdidRetType->Release();
	GPOS_DELETE(m_mdname);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalTVF::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalTVF::Edxlop() const
{
	return EdxlopLogicalTVF;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalTVF::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalTVF::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenLogicalTVF);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalTVF::Arity
//
//	@doc:
//		Return number of return columns
//
//---------------------------------------------------------------------------
ULONG
CDXLLogicalTVF::Arity() const
{
	return m_pdrgdxlcd->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalTVF::GetColumnDescrAt
//
//	@doc:
//		Get the column descriptor at the given position
//
//---------------------------------------------------------------------------
const CDXLColDescr *
CDXLLogicalTVF::GetColumnDescrAt
	(
	ULONG ul
	)
	const
{
	return (*m_pdrgdxlcd)[ul];
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalTVF::FDefinesColumn
//
//	@doc:
//		Check if given column is defined by operator
//
//---------------------------------------------------------------------------
BOOL
CDXLLogicalTVF::FDefinesColumn
	(
	ULONG ulColId
	)
	const
{
	const ULONG ulSize = Arity();
	for (ULONG ulDescr = 0; ulDescr < ulSize; ulDescr++)
	{
		ULONG ulId = GetColumnDescrAt(ulDescr)->Id();
		if (ulId == ulColId)
		{
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalTVF::SerializeToDXL
//
//	@doc:
//		Serialize function descriptor in DXL format
//
//---------------------------------------------------------------------------
void
CDXLLogicalTVF::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	m_pmdidFunc->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenFuncId));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenName), m_mdname->Pstr());
	m_pmdidRetType->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenTypeId));
	
	// serialize columns
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenColumns));
	GPOS_ASSERT(NULL != m_pdrgdxlcd);

	for (ULONG ul = 0; ul < Arity(); ul++)
	{
		CDXLColDescr *pdxlcd = (*m_pdrgdxlcd)[ul];
		pdxlcd->SerializeToDXL(xml_serializer);
	}

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenColumns));

	// serialize arguments
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalTVF::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLLogicalTVF::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) const
{
	// assert validity of function id and return type
	GPOS_ASSERT(NULL != m_pmdidFunc);
	GPOS_ASSERT(NULL != m_pmdidRetType);

	const ULONG ulArity = pdxln->Arity();
	for (ULONG ul = 0; ul < ulArity; ++ul)
	{
		CDXLNode *pdxlnArg = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeScalar == pdxlnArg->GetOperator()->Edxloperatortype());

		if (validate_children)
		{
			pdxlnArg->GetOperator()->AssertValid(pdxlnArg, validate_children);
		}
	}
}

#endif // GPOS_DEBUG


// EOF
