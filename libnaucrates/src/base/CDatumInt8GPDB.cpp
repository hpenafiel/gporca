//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDatumInt8GPDB.cpp
//
//	@doc:
//		Implementation of GPDB Int8
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/gpdb_types.h"

#include "naucrates/base/CDatumInt8GPDB.h"
#include "gpopt/base/CAutoOptCtxt.h"
#include "gpopt/mdcache/CMDAccessor.h"

#include "naucrates/md/IMDType.h"
#include "naucrates/md/IMDTypeInt8.h"
#include "naucrates/md/CMDIdGPDB.h"

using namespace gpopt;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CDatumInt8GPDB::CDatumInt8GPDB
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDatumInt8GPDB::CDatumInt8GPDB
	(
	CSystemId sysid,
	LINT lVal,
	BOOL is_null
	)
	:
	m_mdid(NULL),
	m_lVal(lVal),
	m_is_null(is_null)
{
	CMDAccessor *pmda = COptCtxt::PoctxtFromTLS()->Pmda();
	IMDId *pmdid = dynamic_cast<const CMDTypeInt8GPDB *>(pmda->PtMDType<IMDTypeInt8>(sysid))->MDId();
	pmdid->AddRef();
	m_mdid = pmdid;
	
	if (IsNull())
	{
		// needed for hash computation
		m_lVal = LINT(ULONG_MAX >> 1);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumInt8GPDB::CDatumInt8GPDB
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDatumInt8GPDB::CDatumInt8GPDB
	(
	IMDId *pmdid,
	LINT lVal,
	BOOL is_null
	)
	:
	m_mdid(pmdid),
	m_lVal(lVal),
	m_is_null(is_null)
{
	GPOS_ASSERT(NULL != m_mdid);
	GPOS_ASSERT(GPDB_INT8_OID == CMDIdGPDB::PmdidConvert(m_mdid)->OidObjectId());

	if (IsNull())
	{
		// needed for hash computation
		m_lVal = LINT(ULONG_MAX >> 1);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumInt8GPDB::~CDatumInt8GPDB
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDatumInt8GPDB::~CDatumInt8GPDB()
{
	m_mdid->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumInt8GPDB::Value
//
//	@doc:
//		Accessor of integer value
//
//---------------------------------------------------------------------------
LINT
CDatumInt8GPDB::Value() const
{
	return m_lVal;
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumInt8GPDB::IsNull
//
//	@doc:
//		Accessor of is null
//
//---------------------------------------------------------------------------
BOOL
CDatumInt8GPDB::IsNull() const
{
	return m_is_null;
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumInt8GPDB::UlSize
//
//	@doc:
//		Accessor of size
//
//---------------------------------------------------------------------------
ULONG
CDatumInt8GPDB::UlSize() const
{
	return 8;
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumInt8GPDB::MDId
//
//	@doc:
//		Accessor of type information
//
//---------------------------------------------------------------------------
IMDId *
CDatumInt8GPDB::MDId() const
{
	return m_mdid;
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumInt8GPDB::HashValue
//
//	@doc:
//		Hash function
//
//---------------------------------------------------------------------------
ULONG
CDatumInt8GPDB::HashValue() const
{
	return gpos::CombineHashes(m_mdid->HashValue(), gpos::HashValue<LINT>(&m_lVal));
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumInt8GPDB::GetMDName
//
//	@doc:
//		Return string representation
//
//---------------------------------------------------------------------------
const CWStringConst *
CDatumInt8GPDB::Pstr
	(
	IMemoryPool *memory_pool
	)
	const
{
	CWStringDynamic str(memory_pool);
	if (!IsNull())
	{
		str.AppendFormat(GPOS_WSZ_LIT("%ld"), m_lVal);
	}
	else
	{
		str.AppendFormat(GPOS_WSZ_LIT("null"));
	}

	return GPOS_NEW(memory_pool) CWStringConst(memory_pool, str.GetBuffer());
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumInt8GPDB::FMatch
//
//	@doc:
//		Matches the values of datums
//
//---------------------------------------------------------------------------
BOOL
CDatumInt8GPDB::FMatch
	(
	const IDatum *pdatum
	)
	const
{
	if(!m_mdid->Equals(pdatum->MDId()))
	{
		return false;
	}

	const CDatumInt8GPDB *pdatumInt8 = dynamic_cast<const CDatumInt8GPDB *>(pdatum);

	if(!pdatumInt8->IsNull() && !IsNull())
	{
		return (pdatumInt8->Value() == Value());
	}

	if(pdatumInt8->IsNull() && IsNull())
	{
		return true;
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumInt8GPDB::PdatumCopy
//
//	@doc:
//		Returns a copy of the datum
//
//---------------------------------------------------------------------------
IDatum *
CDatumInt8GPDB::PdatumCopy
	(
	IMemoryPool *memory_pool
	)
	const
{
	m_mdid->AddRef();
	return GPOS_NEW(memory_pool) CDatumInt8GPDB(m_mdid, m_lVal, m_is_null);
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumInt8GPDB::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CDatumInt8GPDB::OsPrint
	(
	IOstream &os
	)
	const
{
	if (!IsNull())
	{
		os << m_lVal;
	}
	else
	{
		os << "null";
	}

	return os;
}

// EOF

