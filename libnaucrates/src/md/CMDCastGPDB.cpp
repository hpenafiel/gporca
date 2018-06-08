//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CMDCastGPDB.cpp
//
//	@doc:
//		Implementation of the class for representing GPDB-specific casts
//		in the MD cache
//---------------------------------------------------------------------------


#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/CMDCastGPDB.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpmd;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CMDCastGPDB::CMDCastGPDB
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDCastGPDB::CMDCastGPDB
	(
	IMemoryPool *memory_pool,
	IMDId *mdid,
	CMDName *mdname,
	IMDId *mdid_src,
	IMDId *mdid_dest,
	BOOL is_binary_coercible,
	IMDId *mdid_cast_func,
	EmdCoercepathType emdPathType
	)
	:
	m_memory_pool(memory_pool),
	m_mdid(mdid),
	m_mdname(mdname),
	m_pmdidSrc(mdid_src),
	m_pmdidDest(mdid_dest),
	m_fBinaryCoercible(is_binary_coercible),
	m_pmdidCastFunc(mdid_cast_func),
	m_emdPathType(emdPathType)
{
	GPOS_ASSERT(m_mdid->IsValid());
	GPOS_ASSERT(m_pmdidSrc->IsValid());
	GPOS_ASSERT(m_pmdidDest->IsValid());
	GPOS_ASSERT_IMP(!is_binary_coercible, m_pmdidCastFunc->IsValid());

	m_pstr = CDXLUtils::SerializeMDObj(m_memory_pool, this, false /*fSerializeHeader*/, false /*indentation*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDCastGPDB::~CMDCastGPDB
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDCastGPDB::~CMDCastGPDB()
{
	m_mdid->Release();
	m_pmdidSrc->Release();
	m_pmdidDest->Release();
	CRefCount::SafeRelease(m_pmdidCastFunc);
	GPOS_DELETE(m_mdname);
	GPOS_DELETE(m_pstr);
}


//---------------------------------------------------------------------------
//	@function:
//		CMDCastGPDB::MDId
//
//	@doc:
//		Mdid of cast object
//
//---------------------------------------------------------------------------
IMDId *
CMDCastGPDB::MDId() const
{
	return m_mdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDCastGPDB::Mdname
//
//	@doc:
//		Func name
//
//---------------------------------------------------------------------------
CMDName
CMDCastGPDB::Mdname() const
{
	return *m_mdname;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDCastGPDB::PmdidSrc
//
//	@doc:
//		Source type id
//
//---------------------------------------------------------------------------
IMDId *
CMDCastGPDB::PmdidSrc() const
{
	return m_pmdidSrc;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDCastGPDB::PmdidDest
//
//	@doc:
//		Destination type id
//
//---------------------------------------------------------------------------
IMDId *
CMDCastGPDB::PmdidDest() const
{
	return m_pmdidDest;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDCastGPDB::GetCastFuncMdId
//
//	@doc:
//		Cast function id
//
//---------------------------------------------------------------------------
IMDId *
CMDCastGPDB::GetCastFuncMdId() const
{
	return m_pmdidCastFunc;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDCastGPDB::FBinaryCoercible
//
//	@doc:
//		Returns whether this is a cast between binary coercible types, i.e. the 
//		types are binary compatible
//
//---------------------------------------------------------------------------
BOOL
CMDCastGPDB::FBinaryCoercible() const
{
	return m_fBinaryCoercible;
}

// returns coercion path type
IMDCast::EmdCoercepathType
CMDCastGPDB::EmdPathType() const
{
	return m_emdPathType;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDCastGPDB::Serialize
//
//	@doc:
//		Serialize function metadata in DXL format
//
//---------------------------------------------------------------------------
void
CMDCastGPDB::Serialize
	(
	CXMLSerializer *xml_serializer
	) 
	const
{
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
						CDXLTokens::GetDXLTokenStr(EdxltokenGPDBCast));
	
	m_mdid->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMdid));

	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenName), m_mdname->GetMDName());

	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenGPDBCastBinaryCoercible), m_fBinaryCoercible);
	m_pmdidSrc->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenGPDBCastSrcType));
	m_pmdidDest->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenGPDBCastDestType));
	m_pmdidCastFunc->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenGPDBCastFuncId));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenGPDBCastCoercePathType), m_emdPathType);

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
						CDXLTokens::GetDXLTokenStr(EdxltokenGPDBCast));
}


#ifdef GPOS_DEBUG

//---------------------------------------------------------------------------
//	@function:
//		CMDCastGPDB::DebugPrint
//
//	@doc:
//		Prints a metadata cache relation to the provided output
//
//---------------------------------------------------------------------------
void
CMDCastGPDB::DebugPrint
	(
	IOstream &os
	)
	const
{
	os << "Cast " << (Mdname()).GetMDName()->GetBuffer() << ": ";
	PmdidSrc()->OsPrint(os);
	os << "->";
	PmdidDest()->OsPrint(os);
	os << std::endl;
		
	if (m_fBinaryCoercible)
	{
		os << ", binary-coercible";
	}
	
	if (IMDId::IsValid(m_pmdidCastFunc))
	{
		os << ", Cast func id: ";
		GetCastFuncMdId()->OsPrint(os);
	}
	
	os << std::endl;	
}

#endif // GPOS_DEBUG

// EOF
