//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLLogicalCTEProducer.cpp
//
//	@doc:
//		Implementation of DXL logical CTE producer operator
//		
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/operators/CDXLLogicalCTEProducer.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/dxltokens.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTEProducer::CDXLLogicalCTEProducer
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLLogicalCTEProducer::CDXLLogicalCTEProducer
	(
	IMemoryPool *memory_pool,
	ULONG ulId,
	ULongPtrArray *pdrgpulColIds
	)
	:
	CDXLLogical(memory_pool),
	m_ulId(ulId),
	m_pdrgpulColIds(pdrgpulColIds)
{
	GPOS_ASSERT(NULL != pdrgpulColIds);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTEProducer::~CDXLLogicalCTEProducer
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLLogicalCTEProducer::~CDXLLogicalCTEProducer()
{
	m_pdrgpulColIds->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTEProducer::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalCTEProducer::Edxlop() const
{
	return EdxlopLogicalCTEProducer;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTEProducer::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalCTEProducer::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenLogicalCTEProducer);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTEProducer::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLLogicalCTEProducer::SerializeToDXL
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
	
	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTEProducer::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLLogicalCTEProducer::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) const
{
	GPOS_ASSERT(1 == pdxln->Arity());

	CDXLNode *child_dxlnode = (*pdxln)[0];
	GPOS_ASSERT(EdxloptypeLogical == child_dxlnode->GetOperator()->Edxloperatortype());

	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
