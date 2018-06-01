//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLScalarNullIf.cpp
//
//	@doc:
//		Implementation of DXL NullIf operator
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarNullIf.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullIf::CDXLScalarNullIf
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarNullIf::CDXLScalarNullIf
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidOp,
	IMDId *mdid_type
	)
	:
	CDXLScalar(memory_pool),
	m_pmdidOp(pmdidOp),
	m_mdid_type(mdid_type)
{
	GPOS_ASSERT(pmdidOp->IsValid());
	GPOS_ASSERT(mdid_type->IsValid());
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullIf::~CDXLScalarNullIf
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLScalarNullIf::~CDXLScalarNullIf()
{
	m_pmdidOp->Release();
	m_mdid_type->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullIf::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarNullIf::Edxlop() const
{
	return EdxlopScalarNullIf;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullIf::PmdidOp
//
//	@doc:
//		Operator id
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarNullIf::PmdidOp() const
{
	return m_pmdidOp;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullIf::MDIdType
//
//	@doc:
//		Return type
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarNullIf::MDIdType() const
{
	return m_mdid_type;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullIf::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarNullIf::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarNullIf);;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullIf::FBoolean
//
//	@doc:
//		Does the operator return boolean result
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarNullIf::FBoolean
	(
	CMDAccessor *pmda
	)
	const
{
	return (IMDType::EtiBool == pmda->Pmdtype(m_mdid_type)->Eti());
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullIf::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarNullIf::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	m_pmdidOp->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenOpNo));
	m_mdid_type->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenTypeId));

	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullIf::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarNullIf::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	const ULONG ulArity = pdxln->Arity();
	GPOS_ASSERT(2 == ulArity);

	for (ULONG ul = 0; ul < ulArity; ++ul)
	{
		CDXLNode *child_dxlnode = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeScalar == child_dxlnode->GetOperator()->Edxloperatortype());

		if (validate_children)
		{
			child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
		}
	}
}
#endif // GPOS_DEBUG

// EOF
