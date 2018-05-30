//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarCast.cpp
//
//	@doc:
//		Implementation of DXL RelabelType
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarCast.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/dxltokens.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpopt;
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCast::CDXLScalarCast
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarCast::CDXLScalarCast
	(
	IMemoryPool *memory_pool,
	IMDId *mdid_type,
	IMDId *pmdidFunc
	)
	:
	CDXLScalar(memory_pool),
	m_mdid_type(mdid_type),
	m_pmdidFunc(pmdidFunc)
{
	GPOS_ASSERT(NULL != m_pmdidFunc);
	GPOS_ASSERT(m_mdid_type->IsValid());
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCast::~CDXLScalarCast
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLScalarCast::~CDXLScalarCast()
{
	m_mdid_type->Release();
	m_pmdidFunc->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCast::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarCast::Edxlop() const
{
	return EdxlopScalarCast;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCast::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarCast::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarCast);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCast::MDIdType
//
//	@doc:
//		Return the oid of the type
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarCast::MDIdType() const
{
	return m_mdid_type;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCast::PmdidFunc
//
//	@doc:
//		Casting function id
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarCast::PmdidFunc() const
{
	return m_pmdidFunc;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCast::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarCast::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *pstrElemName = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);

	m_mdid_type->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenTypeId));
	m_pmdidFunc->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenFuncId));

	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCast::FBoolean
//
//	@doc:
//		Does the operator return a boolean result
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarCast::FBoolean
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
//		CDXLScalarCast::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarCast::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL fValidateChildren
	) const
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
