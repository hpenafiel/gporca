//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 Pivotal Software Inc.
//
//	@filename:
//		CMDIndexInfo.cpp
//
//	@doc:
//		Implementation of the class for representing indexinfo
//---------------------------------------------------------------------------

#include "naucrates/md/CMDIndexInfo.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpdxl;
using namespace gpmd;

// ctor
CMDIndexInfo::CMDIndexInfo
	(
	IMDId *pmdid,
	BOOL fPartial
	)
	:
	m_mdid(pmdid),
	m_fPartial(fPartial)
{
	GPOS_ASSERT(pmdid->IsValid());
}

// dtor
CMDIndexInfo::~CMDIndexInfo()
{
	m_mdid->Release();
}

// returns the metadata id of this index
IMDId *
CMDIndexInfo::MDId() const
{
	return m_mdid;
}

// is the index partial
BOOL
CMDIndexInfo::FPartial() const
{
	return m_fPartial;
}

// serialize indexinfo in DXL format
void
CMDIndexInfo::Serialize
	(
	gpdxl::CXMLSerializer *xml_serializer
	) const
{
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenIndexInfo));

	m_mdid->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenMdid));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenIndexPartial), m_fPartial);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenIndexInfo));
}

#ifdef GPOS_DEBUG
// prints a indexinfo to the provided output
void
CMDIndexInfo::DebugPrint
	(
	IOstream &os
	)
	const
{
	os << "Index id: ";
	MDId()->OsPrint(os);
	os << std::endl;
	os << "Is partial index: " << m_fPartial << std::endl;
}

#endif // GPOS_DEBUG
