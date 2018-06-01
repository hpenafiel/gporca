//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CDXLScalarBitmapIndexProbe.cpp
//
//	@doc:
//		Class for representing DXL bitmap index probe operators
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLIndexDescr.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLScalarBitmapIndexProbe.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/xml/dxltokens.h"

using namespace gpdxl;


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBitmapIndexProbe::CDXLScalarBitmapIndexProbe
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarBitmapIndexProbe::CDXLScalarBitmapIndexProbe
	(
	IMemoryPool *memory_pool,
	CDXLIndexDescr *pdxlid
	)
	:
	CDXLScalar(memory_pool),
	m_pdxlid(pdxlid)
{
	GPOS_ASSERT(NULL != m_pdxlid);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBitmapIndexProbe::~CDXLScalarBitmapIndexProbe
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLScalarBitmapIndexProbe::~CDXLScalarBitmapIndexProbe()
{
	m_pdxlid->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBitmapIndexProbe::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarBitmapIndexProbe::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarBitmapIndexProbe);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBitmapIndexProbe::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarBitmapIndexProbe::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);

	// serialize index descriptor
	m_pdxlid->SerializeToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBitmapIndexProbe::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarBitmapIndexProbe::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	// bitmap index probe has 1 child: the index condition list
	GPOS_ASSERT(1 == pdxln->Arity());

	if (validate_children)
	{
		CDXLNode *pdxlnIndexCondList = (*pdxln)[0];
		GPOS_ASSERT(EdxlopScalarIndexCondList == pdxlnIndexCondList->GetOperator()->Edxlop());
		pdxlnIndexCondList->GetOperator()->AssertValid(pdxlnIndexCondList, validate_children);
	}

	// assert validity of index descriptor
	GPOS_ASSERT(NULL != m_pdxlid->MdName());
	GPOS_ASSERT(m_pdxlid->MdName()->Pstr()->IsValid());
}
#endif // GPOS_DEBUG

// EOF
