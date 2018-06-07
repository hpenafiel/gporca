//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLLogicalCTEConsumer.cpp
//
//	@doc:
//		Implementation of DXL logical CTE Consumer operator
//		
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/operators/CDXLLogicalCTEConsumer.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/dxltokens.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTEConsumer::CDXLLogicalCTEConsumer
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLLogicalCTEConsumer::CDXLLogicalCTEConsumer
	(
	IMemoryPool *memory_pool,
	ULONG id,
	ULongPtrArray *pdrgpulColIds
	)
	:
	CDXLLogical(memory_pool),
	m_id(id),
	m_pdrgpulColIds(pdrgpulColIds)
{
	GPOS_ASSERT(NULL != pdrgpulColIds);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTEConsumer::~CDXLLogicalCTEConsumer
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLLogicalCTEConsumer::~CDXLLogicalCTEConsumer()
{
	m_pdrgpulColIds->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTEConsumer::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalCTEConsumer::GetDXLOperator() const
{
	return EdxlopLogicalCTEConsumer;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTEConsumer::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalCTEConsumer::GetOpNameStr() const
{
	return CDXLTokens::GetDXLTokenStr(EdxltokenLogicalCTEConsumer);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTEConsumer::IsColDefined
//
//	@doc:
//		Check if given column is defined by operator
//
//---------------------------------------------------------------------------
BOOL
CDXLLogicalCTEConsumer::IsColDefined
	(
	ULONG col_id
	)
	const
{
	const ULONG size = m_pdrgpulColIds->Size();
	for (ULONG ul = 0; ul < size; ul++)
	{
		ULONG id = *((*m_pdrgpulColIds)[ul]);
		if (id == col_id)
		{
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTEConsumer::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLLogicalCTEConsumer::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode * //dxlnode
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenCTEId), Id());
	
	CWStringDynamic *pstrColIds = CDXLUtils::Serialize(m_memory_pool, m_pdrgpulColIds);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenColumns), pstrColIds);
	GPOS_DELETE(pstrColIds);
	
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTEConsumer::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLLogicalCTEConsumer::AssertValid
	(
	const CDXLNode *dxlnode,
	BOOL // validate_children
	) const
{
	GPOS_ASSERT(0 == dxlnode->Arity());

}
#endif // GPOS_DEBUG

// EOF
