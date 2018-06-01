//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLScalarCoalesce.cpp
//
//	@doc:
//		Implementation of DXL Coalesce
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarCoalesce.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpopt;
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCoalesce::CDXLScalarCoalesce
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarCoalesce::CDXLScalarCoalesce
	(
	IMemoryPool *memory_pool,
	IMDId *mdid_type
	)
	:
	CDXLScalar(memory_pool),
	m_mdid_type(mdid_type)
{
	GPOS_ASSERT(m_mdid_type->IsValid());
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCoalesce::~CDXLScalarCoalesce
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLScalarCoalesce::~CDXLScalarCoalesce()
{
	m_mdid_type->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCoalesce::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarCoalesce::GetDXLOperator() const
{
	return EdxlopScalarCoalesce;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCoalesce::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarCoalesce::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarCoalesce);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCoalesce::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarCoalesce::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	m_mdid_type->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenTypeId));
	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCoalesce::FBoolean
//
//	@doc:
//		Does the operator return a boolean result
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarCoalesce::FBoolean
	(
	CMDAccessor *pmda
	)
	const
{
	return (IMDType::EtiBool == pmda->Pmdtype(m_mdid_type)->Eti());
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCoalesce::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarCoalesce::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	GPOS_ASSERT(0 < pdxln->Arity());

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
