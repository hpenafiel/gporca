//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLLogicalGet.cpp
//
//	@doc:
//		Implementation of DXL logical get operator
//		
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLLogicalGet.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "naucrates/dxl/xml/dxltokens.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalGet::CDXLLogicalGet
//
//	@doc:
//		Construct a logical get operator node given its table descriptor rtable entry
//
//---------------------------------------------------------------------------
CDXLLogicalGet::CDXLLogicalGet
	(
	IMemoryPool *memory_pool,
	CDXLTableDescr *pdxltabdesc
	)
	:CDXLLogical(memory_pool),
	 m_pdxltabdesc(pdxltabdesc)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalGet::~CDXLLogicalGet
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLLogicalGet::~CDXLLogicalGet()
{
	CRefCount::SafeRelease(m_pdxltabdesc);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalGet::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalGet::GetDXLOperator() const
{
	return EdxlopLogicalGet;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalGet::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalGet::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenLogicalGet);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalGet::Pdxltabdesc
//
//	@doc:
//		Table descriptor for the table scan
//
//---------------------------------------------------------------------------
CDXLTableDescr *
CDXLLogicalGet::Pdxltabdesc() const
{
	return m_pdxltabdesc;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalGet::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLLogicalGet::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *//pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	// serialize table descriptor
	m_pdxltabdesc->SerializeToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalGet::FDefinesColumn
//
//	@doc:
//		Check if given column is defined by operator
//
//---------------------------------------------------------------------------
BOOL
CDXLLogicalGet::FDefinesColumn
	(
	ULONG ulColId
	)
	const
{
	const ULONG ulSize = m_pdxltabdesc->Arity();
	for (ULONG ulDescr = 0; ulDescr < ulSize; ulDescr++)
	{
		ULONG ulId = m_pdxltabdesc->GetColumnDescrAt(ulDescr)->Id();
		if (ulId == ulColId)
		{
			return true;
		}
	}

	return false;
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalGet::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLLogicalGet::AssertValid
	(
	const CDXLNode *, //pdxln
	BOOL // validate_children
	) const
{
	// assert validity of table descriptor
	GPOS_ASSERT(NULL != m_pdxltabdesc);
	GPOS_ASSERT(NULL != m_pdxltabdesc->MdName());
	GPOS_ASSERT(m_pdxltabdesc->MdName()->Pstr()->IsValid());
}
#endif // GPOS_DEBUG

// EOF
