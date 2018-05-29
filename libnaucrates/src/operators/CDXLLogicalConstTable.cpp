//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CDXLLogicalConstTable.cpp
//
//	@doc:
//		Implementation of DXL logical constant tables
//		
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLLogicalConstTable.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "naucrates/dxl/xml/dxltokens.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalConstTable::CDXLLogicalConstTable
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLLogicalConstTable::CDXLLogicalConstTable
	(
	IMemoryPool *pmp,		
	DrgPdxlcd *pdrgpdxlcd,
	DrgPdrgPdxldatum *pdrgpdrgpdxldatum
	)
	:
	CDXLLogical(pmp),
	m_pdrgpdxlcd(pdrgpdxlcd),
	m_pdrgpdrgpdxldatum(pdrgpdrgpdxldatum)
{
	GPOS_ASSERT(NULL != pdrgpdxlcd);
	GPOS_ASSERT(NULL != pdrgpdrgpdxldatum);

#ifdef GPOS_DEBUG
	const ULONG ulLen = pdrgpdrgpdxldatum->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		DrgPdxldatum *pdrgpdxldatum = (*pdrgpdrgpdxldatum)[ul];
		GPOS_ASSERT(pdrgpdxldatum->Size() == pdrgpdxlcd->Size());
	}
#endif
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalConstTable::~CDXLLogicalConstTable
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLLogicalConstTable::~CDXLLogicalConstTable()
{
	m_pdrgpdxlcd->Release();
	m_pdrgpdrgpdxldatum->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalConstTable::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalConstTable::Edxlop() const
{
	return EdxlopLogicalConstTable;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalConstTable::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalConstTable::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenLogicalConstTable);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalConstTable::Pdxlcd
//
//	@doc:
//		Type of const table element at given position
//
//---------------------------------------------------------------------------
CDXLColDescr *
CDXLLogicalConstTable::Pdxlcd
	(
	ULONG ul
	) 
	const
{
	GPOS_ASSERT(m_pdrgpdxlcd->Size() > ul);
	return (*m_pdrgpdxlcd)[ul];
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalConstTable::UlArity
//
//	@doc:
//		Const table arity
//
//---------------------------------------------------------------------------
ULONG
CDXLLogicalConstTable::UlArity() const
{
	return m_pdrgpdxlcd->Size();
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalConstTable::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLLogicalConstTable::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *//pdxln
	)
	const
{
	const CWStringConst *pstrElemName = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);

	// serialize columns
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenColumns));
	
	for (ULONG i = 0; i < UlArity(); i++)
	{
		CDXLColDescr *pdxlcd = (*m_pdrgpdxlcd)[i];
		pdxlcd->SerializeToDXL(xml_serializer);
	}

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenColumns));
	
	const CWStringConst *pstrElemNameConstTuple = CDXLTokens::PstrToken(EdxltokenConstTuple);
	const CWStringConst *pstrElemNameDatum = CDXLTokens::PstrToken(EdxltokenDatum);

	const ULONG ulTuples = m_pdrgpdrgpdxldatum->Size();
	for (ULONG ulTuplePos = 0; ulTuplePos < ulTuples; ulTuplePos++)
	{
		// serialize a const tuple
		xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemNameConstTuple);
		DrgPdxldatum *pdrgpdxldatum = (*m_pdrgpdrgpdxldatum)[ulTuplePos];

		const ULONG ulCols = pdrgpdxldatum->Size();
		for (ULONG ulColPos = 0; ulColPos < ulCols; ulColPos++)
		{
			CDXLDatum *pdxldatum = (*pdrgpdxldatum)[ulColPos];
			pdxldatum->Serialize(xml_serializer, pstrElemNameDatum);
		}

		xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemNameConstTuple);
	}

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalConstTable::FDefinesColumn
//
//	@doc:
//		Check if given column is defined by operator
//
//---------------------------------------------------------------------------
BOOL
CDXLLogicalConstTable::FDefinesColumn
	(
	ULONG ulColId
	)
	const
{
	const ULONG ulSize = UlArity();
	for (ULONG ulDescr = 0; ulDescr < ulSize; ulDescr++)
	{
		ULONG ulId = Pdxlcd(ulDescr)->UlID();
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
//		CDXLLogicalConstTable::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLLogicalConstTable::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL //fValidateChildren
	) const
{
	// assert validity of col descr
	GPOS_ASSERT(m_pdrgpdxlcd != NULL);
	GPOS_ASSERT(0 < m_pdrgpdxlcd->Size());
	GPOS_ASSERT(0 == pdxln->UlArity());
}
#endif // GPOS_DEBUG

// EOF
