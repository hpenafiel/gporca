//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalDML.cpp
//
//	@doc:
//		Implementation of DXL physical DML operator
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLDirectDispatchInfo.h"
#include "naucrates/dxl/operators/CDXLPhysicalDML.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDML::CDXLPhysicalDML
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLPhysicalDML::CDXLPhysicalDML
	(
	IMemoryPool *memory_pool,
	const EdxlDmlType edxldmltype,
	CDXLTableDescr *table_descr,
	ULongPtrArray *pdrgpul,
	ULONG ulAction,
	ULONG ulOid,
	ULONG ctid_colid,
	ULONG segid_colid,
	BOOL preserve_oids,
	ULONG tuple_oid,
	CDXLDirectDispatchInfo *dxl_direct_dispatch_info,
	BOOL fInputSorted
	)
	:
	CDXLPhysical(memory_pool),
	m_edxldmltype(edxldmltype),
	m_table_descr_dxl(table_descr),
	m_pdrgpul(pdrgpul),
	m_ulAction(ulAction),
	m_ulOid(ulOid),
	m_ctid_colid(ctid_colid),
	m_segid_colid(segid_colid),
	m_preserve_oids(preserve_oids),
	m_tuple_oid(tuple_oid),
	m_direct_dispatch_info(dxl_direct_dispatch_info),
	m_fInputSorted(fInputSorted)
{
	GPOS_ASSERT(EdxldmlSentinel > edxldmltype);
	GPOS_ASSERT(NULL != table_descr);
	GPOS_ASSERT(NULL != pdrgpul);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDML::~CDXLPhysicalDML
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLPhysicalDML::~CDXLPhysicalDML()
{
	m_table_descr_dxl->Release();
	m_pdrgpul->Release();
	CRefCount::SafeRelease(m_direct_dispatch_info);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDML::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalDML::GetDXLOperator() const
{
	return EdxlopPhysicalDML;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDML::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalDML::GetOpNameStr() const
{
	switch (m_edxldmltype)
	{
		case Edxldmlinsert:
				return CDXLTokens::PstrToken(EdxltokenPhysicalDMLInsert);
		case Edxldmldelete:
				return CDXLTokens::PstrToken(EdxltokenPhysicalDMLDelete);
		case Edxldmlupdate:
				return CDXLTokens::PstrToken(EdxltokenPhysicalDMLUpdate);
		default:
			return NULL;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDML::SerializeToDXL
//
//	@doc:
//		Serialize function descriptor in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalDML::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	CWStringDynamic *pstrCols = CDXLUtils::Serialize(m_memory_pool, m_pdrgpul);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenColumns), pstrCols);
	GPOS_DELETE(pstrCols);

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenActionColId), m_ulAction);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenOidColId), m_ulOid);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenCtidColId), m_ctid_colid);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenGpSegmentIdColId), m_segid_colid);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenInputSorted), m_fInputSorted);
	
	if (Edxldmlupdate == m_edxldmltype)
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenUpdatePreservesOids), m_preserve_oids);
	}

	if (m_preserve_oids)
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenTupleOidColId), m_tuple_oid);
	}
	
	pdxln->SerializePropertiesToDXL(xml_serializer);

	if (NULL != m_direct_dispatch_info)
	{
		m_direct_dispatch_info->Serialize(xml_serializer);
	}
	else
	{
		// TODO:  - Oct 22, 2014; clean this code once the direct dispatch code for DML and SELECT is unified
		xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenDirectDispatchInfo));
		xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenDirectDispatchInfo));
	}
	
	// serialize project list
	(*pdxln)[0]->SerializeToDXL(xml_serializer);

	// serialize table descriptor
	m_table_descr_dxl->SerializeToDXL(xml_serializer);
	
	// serialize physical child
	(*pdxln)[1]->SerializeToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDML::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLPhysicalDML::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	GPOS_ASSERT(2 == pdxln->Arity());
	CDXLNode *child_dxlnode = (*pdxln)[1];
	GPOS_ASSERT(EdxloptypePhysical == child_dxlnode->GetOperator()->GetDXLOperatorType());

	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}

#endif // GPOS_DEBUG


// EOF
