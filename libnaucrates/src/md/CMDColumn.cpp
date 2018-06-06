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
	CMDName *mdname,
	INT iAttNo,
	IMDId *mdid_type,
	INT type_modifier,
	BOOL fNullable,
	BOOL fDropped,
	CDXLNode *pdxnlDefaultValue,
	ULONG length
	)
	:
	m_mdname(mdname),
	m_iAttNo(iAttNo),
	m_mdid_type(mdid_type),
	m_type_modifier(type_modifier),
	m_fNullable(fNullable),
	m_fDropped(fDropped),
	m_length(length),
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
	GPOS_DELETE(m_mdname);
	m_mdid_type->Release();
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
	return *m_mdname;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDColumn::AttrNum
//
//	@doc:
//		Attribute number
//
//---------------------------------------------------------------------------
INT
CMDColumn::AttrNum() const
{
	return m_iAttNo;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDColumn::MDIdType
//
//	@doc:
//		Attribute type id
//
//---------------------------------------------------------------------------
IMDId *
CMDColumn::MDIdType() const
{
	return m_mdid_type;
}

INT
CMDColumn::TypeModifier() const
{
	return m_type_modifier;
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
//		CMDColumn::IsDropped
//
//	@doc:
//		Returns whether column is dropped
//
//---------------------------------------------------------------------------
BOOL
CMDColumn::IsDropped() const
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
	
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenName), m_mdname->GetMDName());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenAttno), m_iAttNo);

	m_mdid_type->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenMdid));
	if (IDefaultTypeModifier != TypeModifier())
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenTypeMod), TypeModifier());
	}

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenColumnNullable), m_fNullable);
	if (ULONG_MAX != m_length)
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenColWidth), m_length);
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
	os << "Attno: " << AttrNum() << std::endl;
	
	os << "Column name: " << (Mdname()).GetMDName()->GetBuffer() << std::endl;
	os << "Column type: ";
	MDIdType()->OsPrint(os);
	os << std::endl;

	const CWStringConst *pstrNullsAllowed = FNullable() ?
												CDXLTokens::PstrToken(EdxltokenTrue) :
												CDXLTokens::PstrToken(EdxltokenFalse);
	
	os << "Nulls allowed: " << pstrNullsAllowed->GetBuffer() << std::endl;
}

#endif // GPOS_DEBUG

// EOF

