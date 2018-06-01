//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalCTEProducer.cpp
//
//	@doc:
//		Implementation of DXL physical CTE producer operator
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/operators/CDXLPhysicalCTEProducer.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/dxltokens.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalCTEProducer::CDXLPhysicalCTEProducer
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLPhysicalCTEProducer::CDXLPhysicalCTEProducer
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
//		CDXLPhysicalCTEProducer::~CDXLPhysicalCTEProducer
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLPhysicalCTEProducer::~CDXLPhysicalCTEProducer()
{
	m_pdrgpulColIds->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalCTEProducer::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalCTEProducer::Edxlop() const
{
	return EdxlopPhysicalCTEProducer;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalCTEProducer::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalCTEProducer::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalCTEProducer);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalCTEProducer::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalCTEProducer::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();

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
//		CDXLPhysicalCTEProducer::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLPhysicalCTEProducer::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) const
{
	GPOS_ASSERT(2 == pdxln->Arity());

	CDXLNode *pdxlnPrL = (*pdxln)[0];
	CDXLNode *child_dxlnode = (*pdxln)[1];

	GPOS_ASSERT(EdxlopScalarProjectList == pdxlnPrL->GetOperator()->Edxlop());
	GPOS_ASSERT(EdxloptypePhysical == child_dxlnode->GetOperator()->Edxloperatortype());

	if (validate_children)
	{
		pdxlnPrL->GetOperator()->AssertValid(pdxlnPrL, validate_children);
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
