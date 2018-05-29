//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CMDColumn.cpp
//
//	@doc:
//		Implementation of the class for representing metadata about relation's
//		columns
//---------------------------------------------------------------------------


#include "naucrates/md/CMDColumn.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CMDColumn::CMDColumn
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDColumn::CMDColumn
	(
	CMDName *pmdname,
	INT iAttNo,
	IMDId *pmdidType,
	INT iTypeModifier,
	BOOL fNullable,
	BOOL fDropped,
	CDXLNode *pdxnlDefaultValue,
	ULONG ulLength
	)
	:
	m_pmdname(pmdname),
	m_iAttNo(iAttNo),
	m_pmdidType(pmdidType),
	m_iTypeModifier(iTypeModifier),
	m_fNullable(fNullable),
	m_fDropped(fDropped),
	m_ulLength(ulLength),
	m_pdxlnDefaultValue(pdxnlDefaultValue)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CMDColumn::~CMDColumn
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDColumn::~CMDColumn()
{
	GPOS_DELETE(m_pmdname);
	m_pmdidType->Release();
	CRefCount::SafeRelease(m_pdxlnDefaultValue);
}


//---------------------------------------------------------------------------
//	@function:
//		CMDColumn::Mdname
//
//	@doc:
//		Returns the column name
//
//---------------------------------------------------------------------------
CMDName
CMDColumn::Mdname() const
{
	return *m_pmdname;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDColumn::IAttno
//
//	@doc:
//		Attribute number
//
//---------------------------------------------------------------------------
INT
CMDColumn::IAttno() const
{
	return m_iAttNo;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDColumn::PmdidType
//
//	@doc:
//		Attribute type id
//
//---------------------------------------------------------------------------
IMDId *
CMDColumn::PmdidType() const
{
	return m_pmdidType;
}

INT
CMDColumn::ITypeModifier() const
{
	return m_iTypeModifier;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDColumn::FNullable
//
//	@doc:
//		Returns whether NULLs are allowed for this column
//
//---------------------------------------------------------------------------
BOOL
CMDColumn::FNullable() const
{
	return m_fNullable;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDColumn::FDropped
//
//	@doc:
//		Returns whether column is dropped
//
//---------------------------------------------------------------------------
BOOL
CMDColumn::FDropped() const
{
	return m_fDropped;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDColumn::Serialize
//
//	@doc:
//		Serialize column metadata in DXL format
//
//---------------------------------------------------------------------------
void
CMDColumn::Serialize
	(
	CXMLSerializer *xml_serializer
	) 
	const
{
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), 
						CDXLTokens::PstrToken(EdxltokenColumn));
	
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenName), m_pmdname->Pstr());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenAttno), m_iAttNo);

	m_pmdidType->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenMdid));
	if (IDefaultTypeModifier != ITypeModifier())
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenTypeMod), ITypeModifier());
	}

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenColumnNullable), m_fNullable);
	if (ULONG_MAX != m_ulLength)
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenColWidth), m_ulLength);
	}

	if (m_fDropped)
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenColDropped), m_fDropped);
	}
	
	// serialize default value
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), 
						CDXLTokens::PstrToken(EdxltokenColumnDefaultValue));
	
	if (NULL != m_pdxlnDefaultValue)
	{
		m_pdxlnDefaultValue->SerializeToDXL(xml_serializer);
	}

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), 
						CDXLTokens::PstrToken(EdxltokenColumnDefaultValue));
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), 
						CDXLTokens::PstrToken(EdxltokenColumn));
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CMDColumn::DebugPrint
//
//	@doc:
//		Debug print of the column in the provided stream
//
//---------------------------------------------------------------------------
void
CMDColumn::DebugPrint
	(
	IOstream &os
	) 
	const
{
	os << "Attno: " << IAttno() << std::endl;
	
	os << "Column name: " << (Mdname()).Pstr()->GetBuffer() << std::endl;
	os << "Column type: ";
	PmdidType()->OsPrint(os);
	os << std::endl;

	const CWStringConst *pstrNullsAllowed = FNullable() ?
												CDXLTokens::PstrToken(EdxltokenTrue) :
												CDXLTokens::PstrToken(EdxltokenFalse);
	
	os << "Nulls allowed: " << pstrNullsAllowed->GetBuffer() << std::endl;
}

#endif // GPOS_DEBUG

// EOF

