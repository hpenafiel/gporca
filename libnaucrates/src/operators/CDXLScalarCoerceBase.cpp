//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 Pivotal Inc.
//
//	@filename:
//		CDXLScalarCoerceBase.cpp
//
//	@doc:
//		Implementation of DXL scalar coerce base class
//
//	@owner:
//
//	@test:
//
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarCoerceBase.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/dxltokens.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpopt;
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCoerceBase::CDXLScalarCoerceBase
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarCoerceBase::CDXLScalarCoerceBase
	(
	IMemoryPool *pmp,
	IMDId *pmdidType,
	INT iTypeModifier,
	EdxlCoercionForm edxlcf,
	INT iLoc
	)
	:
	CDXLScalar(pmp),
	m_pmdidResultType(pmdidType),
	m_iTypeModifier(iTypeModifier),
	m_edxlcf(edxlcf),
	m_iLoc(iLoc)
{
	GPOS_ASSERT(NULL != pmdidType);
	GPOS_ASSERT(pmdidType->IsValid());
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCoerceBase::~CDXLScalarCoerceBase
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLScalarCoerceBase::~CDXLScalarCoerceBase()
{
	m_pmdidResultType->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCoerceBase::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarCoerceBase::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *pstrElemName = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);

	m_pmdidResultType->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenTypeId));

	if (IDefaultTypeModifier != TypeModifier())
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenTypeMod), TypeModifier());
	}
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenCoercionForm), (ULONG) m_edxlcf);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenLocation), m_iLoc);

	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCoerceBase::FBoolean
//
//	@doc:
//		Does the operator return a boolean result
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarCoerceBase::FBoolean
	(
	CMDAccessor *pmda
	)
	const
{
	return (IMDType::EtiBool == pmda->Pmdtype(m_pmdidResultType)->Eti());
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCoerceBase::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarCoerceBase::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL fValidateChildren
	) const
{
	GPOS_ASSERT(1 == pdxln->UlArity());

	CDXLNode *pdxlnChild = (*pdxln)[0];
	GPOS_ASSERT(EdxloptypeScalar == pdxlnChild->Pdxlop()->Edxloperatortype());

	if (fValidateChildren)
	{
		pdxlnChild->Pdxlop()->AssertValid(pdxlnChild, fValidateChildren);
	}
}
#endif // GPOS_DEBUG

// EOF
