//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CMDArrayCoerceCastGPDB.cpp
//
//	@doc:
//		Implementation of the class for representing GPDB-specific array coerce
//		casts in the MD cache
//---------------------------------------------------------------------------


#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/CMDArrayCoerceCastGPDB.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpmd;
using namespace gpdxl;

// ctor
CMDArrayCoerceCastGPDB::CMDArrayCoerceCastGPDB
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid,
	CMDName *mdname,
	IMDId *pmdidSrc,
	IMDId *pmdidDest,
	BOOL fBinaryCoercible,
	IMDId *pmdidCastFunc,
	EmdCoercepathType emdPathType,
	INT type_modifier,
	BOOL fIsExplicit,
	EdxlCoercionForm edxlcf,
	INT iLoc
	)
	:
	CMDCastGPDB(memory_pool, pmdid, mdname, pmdidSrc, pmdidDest, fBinaryCoercible, pmdidCastFunc, emdPathType),
	m_type_modifier(type_modifier),
	m_fIsExplicit(fIsExplicit),
	m_edxlcf(edxlcf),
	m_iLoc(iLoc)
{
	m_pstr = CDXLUtils::SerializeMDObj(memory_pool, this, false /*fSerializeHeader*/, false /*indentation*/);
}

// dtor
CMDArrayCoerceCastGPDB::~CMDArrayCoerceCastGPDB()
{
	GPOS_DELETE(m_pstr);
}

// return type modifier
INT
CMDArrayCoerceCastGPDB::TypeModifier() const
{
	return m_type_modifier;
}

// return is explicit cast
BOOL
CMDArrayCoerceCastGPDB::FIsExplicit() const
{
	return m_fIsExplicit;
}

// return coercion form
EdxlCoercionForm
CMDArrayCoerceCastGPDB::Ecf() const
{
	return m_edxlcf;
}

// return token location
INT
CMDArrayCoerceCastGPDB::ILoc() const
{
	return m_iLoc;
}

// serialize function metadata in DXL format
void
CMDArrayCoerceCastGPDB::Serialize
	(
	CXMLSerializer *xml_serializer
	)
	const
{
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix),
						CDXLTokens::PstrToken(EdxltokenGPDBArrayCoerceCast));

	m_mdid->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenMdid));

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenName), m_mdname->GetMDName());

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenGPDBCastCoercePathType), m_emdPathType);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenGPDBCastBinaryCoercible), m_fBinaryCoercible);

	m_pmdidSrc->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenGPDBCastSrcType));
	m_pmdidDest->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenGPDBCastDestType));
	m_pmdidCastFunc->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenGPDBCastFuncId));

	if (IDefaultTypeModifier != TypeModifier())
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenTypeMod), TypeModifier());
	}

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenIsExplicit), m_fIsExplicit);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenCoercionForm), (ULONG) m_edxlcf);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenLocation), m_iLoc);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix),
						CDXLTokens::PstrToken(EdxltokenGPDBArrayCoerceCast));
}


#ifdef GPOS_DEBUG

// prints a metadata cache relation to the provided output
void
CMDArrayCoerceCastGPDB::DebugPrint
	(
	IOstream &os
	)
	const
{
	CMDCastGPDB::DebugPrint(os);
	os << ", Result Type Mod: ";
	os << m_type_modifier;
	os << ", isExplicit: ";
	os << m_fIsExplicit;
	os << ", coercion form: ";
	os << m_edxlcf;

	os << std::endl;
}

#endif // GPOS_DEBUG

// EOF
