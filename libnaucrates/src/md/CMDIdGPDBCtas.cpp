//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CMDIdGPDBCtas.cpp
//
//	@doc:
//		Implementation of metadata identifiers for GPDB CTAS queries
//---------------------------------------------------------------------------

#include "naucrates/md/CMDIdGPDBCtas.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpmd;

#define GPMD_GPDB_CTAS_SYSID GPOS_WSZ_LIT("CTAS")

// invalid key
CMDIdGPDBCtas CMDIdGPDBCtas::m_mdidInvalidKey(0);

//---------------------------------------------------------------------------
//	@function:
//		CMDIdGPDBCtas::CMDIdGPDBCtas
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDIdGPDBCtas::CMDIdGPDBCtas
	(
	OID oid
	)
	:
	CMDIdGPDB(CSystemId(IMDId::EmdidGPDB, GPMD_GPDB_CTAS_SYSID), oid)
{
	Serialize();
}


//---------------------------------------------------------------------------
//	@function:
//		CMDIdGPDBCtas::CMDIdGPDBCtas
//
//	@doc:
//		Copy constructor
//
//---------------------------------------------------------------------------
CMDIdGPDBCtas::CMDIdGPDBCtas
	(
	const CMDIdGPDBCtas &mdidSource
	)
	:
	CMDIdGPDB(mdidSource.Sysid(), mdidSource.OidObjectId())
{
	GPOS_ASSERT(mdidSource.IsValid());
	GPOS_ASSERT(IMDId::EmdidGPDBCtas == mdidSource.Emdidt());
	Serialize();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdGPDBCtas::Equals
//
//	@doc:
//		Checks if the version of the current object is compatible with another version
//		of the same object
//
//---------------------------------------------------------------------------
BOOL
CMDIdGPDBCtas::Equals
	(
	const IMDId *mdid
	) 
	const
{
	if (NULL == mdid || EmdidGPDBCtas != mdid->Emdidt())
	{
		return false;
	}
	
	const CMDIdGPDBCtas *pmdidGPDBCTAS = CMDIdGPDBCtas::PmdidConvert(const_cast<IMDId *>(mdid));
	
	return m_oid == pmdidGPDBCTAS->OidObjectId(); 
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdGPDBCtas::IsValid
//
//	@doc:
//		Is the mdid valid
//
//---------------------------------------------------------------------------
BOOL CMDIdGPDBCtas::IsValid() const
{
	return !Equals(&CMDIdGPDBCtas::m_mdidInvalidKey);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdGPDBCtas::OsPrint
//
//	@doc:
//		Debug print of the id in the provided stream
//
//---------------------------------------------------------------------------
IOstream &
CMDIdGPDBCtas::OsPrint
	(
	IOstream &os
	) 
	const
{
	os << "(" << OidObjectId() << "," << 
				UlVersionMajor() << "." << UlVersionMinor() << ")";
	return os;
}

// EOF
