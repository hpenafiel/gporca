//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLLogicalUpdate.cpp
//
//	@doc:
//		Implementation of DXL logical update operator
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/operators/CDXLLogicalUpdate.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalUpdate::CDXLLogicalUpdate
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLLogicalUpdate::CDXLLogicalUpdate
	(
	IMemoryPool *pmp,
	CDXLTableDescr *pdxltabdesc,
	ULONG ulCtid,
	ULONG ulSegmentId,
	ULongPtrArray *pdrgpulDelete,
	ULongPtrArray *pdrgpulInsert,
	BOOL fPreserveOids,
	ULONG ulTupleOid
	)
	:
	CDXLLogical(pmp),
	m_pdxltabdesc(pdxltabdesc),
	m_ulCtid(ulCtid),
	m_ulSegmentId(ulSegmentId),
	m_pdrgpulDelete(pdrgpulDelete),
	m_pdrgpulInsert(pdrgpulInsert),
	m_fPreserveOids(fPreserveOids),
	m_ulTupleOid(ulTupleOid)
{
	GPOS_ASSERT(NULL != pdxltabdesc);
	GPOS_ASSERT(NULL != pdrgpulDelete);
	GPOS_ASSERT(NULL != pdrgpulInsert);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalUpdate::~CDXLLogicalUpdate
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLLogicalUpdate::~CDXLLogicalUpdate()
{
	m_pdxltabdesc->Release();
	m_pdrgpulDelete->Release();
	m_pdrgpulInsert->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalUpdate::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalUpdate::Edxlop() const
{
	return EdxlopLogicalUpdate;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalUpdate::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalUpdate::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenLogicalUpdate);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalUpdate::SerializeToDXL
//
//	@doc:
//		Serialize function descriptor in DXL format
//
//---------------------------------------------------------------------------
void
CDXLLogicalUpdate::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *pstrElemName = PstrOpName();
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);

	CWStringDynamic *pstrColsDel = CDXLUtils::Serialize(m_pmp, m_pdrgpulDelete);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenDeleteCols), pstrColsDel);
	GPOS_DELETE(pstrColsDel);

	CWStringDynamic *pstrColsIns = CDXLUtils::Serialize(m_pmp, m_pdrgpulInsert);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenInsertCols), pstrColsIns);
	GPOS_DELETE(pstrColsIns);

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenCtidColId), m_ulCtid);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenGpSegmentIdColId), m_ulSegmentId);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenUpdatePreservesOids), m_fPreserveOids);
	
	if (m_fPreserveOids)
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenTupleOidColId), m_ulTupleOid);
	}
	
	m_pdxltabdesc->SerializeToDXL(xml_serializer);
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalUpdate::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLLogicalUpdate::AssertValid
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
