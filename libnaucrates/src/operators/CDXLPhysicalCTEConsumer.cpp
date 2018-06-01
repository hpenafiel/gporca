//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalCTEConsumer.cpp
//
//	@doc:
//		Implementation of DXL physical CTE Consumer operator
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/operators/CDXLPhysicalCTEConsumer.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/dxltokens.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalCTEConsumer::CDXLPhysicalCTEConsumer
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLPhysicalCTEConsumer::CDXLPhysicalCTEConsumer
	(
	IMemoryPool *memory_pool,
	ULONG ulId,
	ULongPtrArray *pdrgpulColIds
	)
	:
	CDXLPhysical(memory_pool),
	m_ulId(ulId),
	m_pdrgpulColIds(pdrgpulColIds)
{
	GPOS_ASSERT(NULL != pdrgpulColIds);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalCTEConsumer::~CDXLPhysicalCTEConsumer
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLPhysicalCTEConsumer::~CDXLPhysicalCTEConsumer()
{
	m_pdrgpulColIds->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalCTEConsumer::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalCTEConsumer::GetDXLOperator() const
{
	return EdxlopPhysicalCTEConsumer;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalCTEConsumer::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalCTEConsumer::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalCTEConsumer);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalCTEConsumer::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalCTEConsumer::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenCTEId), UlId());

	CWStringDynamic *pstrColIds = CDXLUtils::Serialize(m_memory_pool, m_pdrgpulColIds);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenColumns), pstrColIds);
	GPOS_DELETE(pstrColIds);

	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);

	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalCTEConsumer::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLPhysicalCTEConsumer::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) const
{
	GPOS_ASSERT(1 == pdxln->Arity());

	CDXLNode *pdxlnPrL = (*pdxln)[0];
	GPOS_ASSERT(EdxlopScalarProjectList == pdxlnPrL->GetOperator()->GetDXLOperator());

	if (validate_children)
	{
		pdxlnPrL->GetOperator()->AssertValid(pdxlnPrL, validate_children);
	}

}
#endif // GPOS_DEBUG

// EOF
