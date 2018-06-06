//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDatumOidGPDB.cpp
//
//	@doc:
//		Implementation of GPDB oid datum
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/gpdb_types.h"

#include "naucrates/base/CDatumOidGPDB.h"
#include "gpopt/base/CAutoOptCtxt.h"
#include "gpopt/mdcache/CMDAccessor.h"

#include "naucrates/md/IMDType.h"
#include "naucrates/md/IMDTypeOid.h"
#include "naucrates/md/CMDTypeOidGPDB.h"
#include "naucrates/md/CMDIdGPDB.h"

using namespace gpnaucrates;
using namespace gpmd;
using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CDatumOidGPDB::CDatumOidGPDB
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDatumOidGPDB::CDatumOidGPDB
	(
	CSystemId sysid,
	OID oidVal,
	BOOL is_null
	)
	:
	m_mdid(NULL),
	m_oidVal(oidVal),
	m_is_null(is_null)
{
	CMDAccessor *pmda = COptCtxt::PoctxtFromTLS()->Pmda();
	IMDId *pmdid = dynamic_cast<const CMDTypeOidGPDB *>(pmda->PtMDType<IMDTypeOid>(sysid))->MDId();
	pmdid->AddRef();

	m_mdid = pmdid;

	if (IsNull())
	{
		// needed for hash computation
		m_oidVal = INT_MAX;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumOidGPDB::CDatumOidGPDB
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDatumOidGPDB::CDatumOidGPDB
	(
	IMDId *pmdid,
	OID oidVal,
	BOOL is_null
	)
	:
	m_mdid(pmdid),
	m_oidVal(oidVal),
	m_is_null(is_null)
{
	GPOS_ASSERT(NULL != m_mdid);
	GPOS_ASSERT(GPDB_OID_OID == CMDIdGPDB::PmdidConvert(m_mdid)->OidObjectId());

	if (IsNull())
	{
		// needed for hash computation
		m_oidVal = INT_MAX;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumOidGPDB::~CDatumOidGPDB
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDatumOidGPDB::~CDatumOidGPDB()
{
	m_mdid->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumOidGPDB::OidValue
//
//	@doc:
//		Accessor of oid value
//
//---------------------------------------------------------------------------
OID
CDatumOidGPDB::OidValue() const
{
	return m_oidVal;
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumOidGPDB::IsNull
//
//	@doc:
//		Accessor of is null
//
//---------------------------------------------------------------------------
BOOL
CDatumOidGPDB::IsNull() const
{
	return m_is_null;
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumOidGPDB::UlSize
//
//	@doc:
//		Accessor of size
//
//---------------------------------------------------------------------------
ULONG
CDatumOidGPDB::UlSize() const
{
	return 4;
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumOidGPDB::MDId
//
//	@doc:
//		Accessor of type information
//
//---------------------------------------------------------------------------
IMDId *
CDatumOidGPDB::MDId() const
{
	return m_mdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumOidGPDB::HashValue
//
//	@doc:
//		Hash function
//
//---------------------------------------------------------------------------
ULONG
CDatumOidGPDB::HashValue() const
{
	return gpos::CombineHashes(m_mdid->HashValue(), gpos::HashValue<OID>(&m_oidVal));
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumOidGPDB::GetMDName
//
//	@doc:
//		Return string representation
//
//---------------------------------------------------------------------------
const CWStringConst *
CDatumOidGPDB::Pstr
	(
	IMemoryPool *memory_pool
	)
	const
{
	CWStringDynamic str(memory_pool);
	if (!IsNull())
	{
		str.AppendFormat(GPOS_WSZ_LIT("%d"), m_oidVal);
	}
	else
	{
		str.AppendFormat(GPOS_WSZ_LIT("null"));
	}

	return GPOS_NEW(memory_pool) CWStringConst(memory_pool, str.GetBuffer());
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumOidGPDB::FMatch
//
//	@doc:
//		Matches the values of datums
//
//---------------------------------------------------------------------------
BOOL
CDatumOidGPDB::FMatch
	(
	const IDatum *pdatum
	)
	const
{
	if(!pdatum->MDId()->Equals(m_mdid))
	{
		return false;
	}

	const CDatumOidGPDB *pdatumoid = dynamic_cast<const CDatumOidGPDB *>(pdatum);

	if(!pdatumoid->IsNull() && !IsNull())
	{
		return (pdatumoid->OidValue() == OidValue());
	}

	if(pdatumoid->IsNull() && IsNull())
	{
		return true;
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumOidGPDB::PdatumCopy
//
//	@doc:
//		Returns a copy of the datum
//
//---------------------------------------------------------------------------
IDatum *
CDatumOidGPDB::PdatumCopy
	(
	IMemoryPool *memory_pool
	)
	const
{
	m_mdid->AddRef();
	return GPOS_NEW(memory_pool) CDatumOidGPDB(m_mdid, m_oidVal, m_is_null);
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumOidGPDB::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CDatumOidGPDB::OsPrint
	(
	IOstream &os
	)
	const
{
	if (!IsNull())
	{
		os << m_oidVal;
	}
	else
	{
		os << "null";
	}

	return os;
}

// EOF
