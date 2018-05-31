//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CMDScCmpGPDB.cpp
//
//	@doc:
//		Implementation of the class for representing GPDB-specific comparisons
//		in the MD cache
//---------------------------------------------------------------------------


#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/CMDScCmpGPDB.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpmd;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CMDScCmpGPDB::CMDScCmpGPDB
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDScCmpGPDB::CMDScCmpGPDB
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid,
	CMDName *mdname,
	IMDId *pmdidLeft,
	IMDId *pmdidRight,
	IMDType::ECmpType ecmpt,
	IMDId *pmdidOp
	)
	:
	m_memory_pool(memory_pool),
	m_mdid(pmdid),
	m_mdname(mdname),
	m_pmdidLeft(pmdidLeft),
	m_pmdidRight(pmdidRight),
	m_ecmpt(ecmpt),
	m_pmdidOp(pmdidOp)
{
	GPOS_ASSERT(m_mdid->IsValid());
	GPOS_ASSERT(m_pmdidLeft->IsValid());
	GPOS_ASSERT(m_pmdidRight->IsValid());
	GPOS_ASSERT(m_pmdidOp->IsValid());
	GPOS_ASSERT(IMDType::EcmptOther != m_ecmpt);

	m_pstr = CDXLUtils::SerializeMDObj(m_memory_pool, this, false /*fSerializeHeader*/, false /*indentation*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScCmpGPDB::~CMDScCmpGPDB
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDScCmpGPDB::~CMDScCmpGPDB()
{
	m_mdid->Release();
	m_pmdidLeft->Release();
	m_pmdidRight->Release();
	m_pmdidOp->Release();
	GPOS_DELETE(m_mdname);
	GPOS_DELETE(m_pstr);
}


//---------------------------------------------------------------------------
//	@function:
//		CMDScCmpGPDB::MDId
//
//	@doc:
//		Mdid of comparison object
//
//---------------------------------------------------------------------------
IMDId *
CMDScCmpGPDB::MDId() const
{
	return m_mdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScCmpGPDB::Mdname
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
CMDName
CMDScCmpGPDB::Mdname() const
{
	return *m_mdname;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScCmpGPDB::PmdidLeft
//
//	@doc:
//		Left type id
//
//---------------------------------------------------------------------------
IMDId *
CMDScCmpGPDB::PmdidLeft() const
{
	return m_pmdidLeft;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScCmpGPDB::PmdidRight
//
//	@doc:
//		Destination type id
//
//---------------------------------------------------------------------------
IMDId *
CMDScCmpGPDB::PmdidRight() const
{
	return m_pmdidRight;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScCmpGPDB::PmdidOp
//
//	@doc:
//		Cast function id
//
//---------------------------------------------------------------------------
IMDId *
CMDScCmpGPDB::PmdidOp() const
{
	return m_pmdidOp;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScCmpGPDB::FBinaryCoercible
//
//	@doc:
//		Returns whether this is a cast between binary coercible types, i.e. the 
//		types are binary compatible
//
//---------------------------------------------------------------------------
IMDType::ECmpType
CMDScCmpGPDB::Ecmpt() const
{
	return m_ecmpt;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScCmpGPDB::Serialize
//
//	@doc:
//		Serialize comparison operator metadata in DXL format
//
//---------------------------------------------------------------------------
void
CMDScCmpGPDB::Serialize
	(
	CXMLSerializer *xml_serializer
	) 
	const
{
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), 
						CDXLTokens::PstrToken(EdxltokenGPDBMDScCmp));
	
	m_mdid->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenMdid));

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenName), m_mdname->Pstr());
	
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenGPDBScalarOpCmpType), IMDType::PstrCmpType(m_ecmpt));

	m_pmdidLeft->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenGPDBScalarOpLeftTypeId));
	m_pmdidRight->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenGPDBScalarOpRightTypeId));
	m_pmdidOp->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenOpNo));

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), 
						CDXLTokens::PstrToken(EdxltokenGPDBMDScCmp));

	GPOS_CHECK_ABORT;
}


#ifdef GPOS_DEBUG

//---------------------------------------------------------------------------
//	@function:
//		CMDScCmpGPDB::DebugPrint
//
//	@doc:
//		Prints a metadata cache relation to the provided output
//
//---------------------------------------------------------------------------
void
CMDScCmpGPDB::DebugPrint
	(
	IOstream &os
	)
	const
{
	os << "ComparisonOp ";
	PmdidLeft()->OsPrint(os);
	os << (Mdname()).Pstr()->GetBuffer() << "(";
	PmdidOp()->OsPrint(os);
	os << ") ";
	PmdidLeft()->OsPrint(os);


	os << ", type: " << IMDType::PstrCmpType(m_ecmpt);

	os << std::endl;	
}

#endif // GPOS_DEBUG

// EOF
