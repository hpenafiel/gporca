//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLLogicalDelete.cpp
//
//	@doc:
//		Implementation of DXL logical delete operator
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/operators/CDXLLogicalDelete.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalDelete::CDXLLogicalDelete
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLLogicalDelete::CDXLLogicalDelete
	(
	IMemoryPool *memory_pool,
	CDXLTableDescr *pdxltabdesc,
	ULONG ulCtid,
	ULONG ulSegmentId,
	ULongPtrArray *pdrgpulDelete
	)
	:
	CDXLLogical(memory_pool),
	m_pdxltabdesc(pdxltabdesc),
	m_ulCtid(ulCtid),
	m_ulSegmentId(ulSegmentId),
	m_pdrgpulDelete(pdrgpulDelete)
{
	GPOS_ASSERT(NULL != pdxltabdesc);
	GPOS_ASSERT(NULL != pdrgpulDelete);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalDelete::~CDXLLogicalDelete
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLLogicalDelete::~CDXLLogicalDelete()
{
	m_pdxltabdesc->Release();
	m_pdrgpulDelete->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalDelete::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalDelete::Edxlop() const
{
	return EdxlopLogicalDelete;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalDelete::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalDelete::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenLogicalDelete);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalDelete::SerializeToDXL
//
//	@doc:
//		Serialize function descriptor in DXL format
//
//---------------------------------------------------------------------------
void
CDXLLogicalDelete::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *pstrElemName = PstrOpName();
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);

	CWStringDynamic *pstrColsDel = CDXLUtils::Serialize(m_memory_pool, m_pdrgpulDelete);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenDeleteCols), pstrColsDel);
	GPOS_DELETE(pstrColsDel);

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenCtidColId), m_ulCtid);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenGpSegmentIdColId), m_ulSegmentId);

	m_pdxltabdesc->SerializeToDXL(xml_serializer);
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalDelete::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLLogicalDelete::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL fValidateChildren
	)
	const
{
	GPOS_ASSERT(1 == pdxln->UlArity());

	CDXLNode *pdxlnChild = (*pdxln)[0];
	GPOS_ASSERT(EdxloptypeLogical == pdxlnChild->Pdxlop()->Edxloperatortype());

	if (fValidateChildren)
	{
		pdxlnChild->Pdxlop()->AssertValid(pdxlnChild, fValidateChildren);
	}
}

#endif // GPOS_DEBUG


// EOF
