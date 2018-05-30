//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CDatumInt4GPDB.cpp
//
//	@doc:
//		Implementation of GPDB int4
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/gpdb_types.h"

#include "naucrates/base/CDatumInt4GPDB.h"
#include "gpopt/base/CAutoOptCtxt.h"
#include "gpopt/mdcache/CMDAccessor.h"

#include "naucrates/md/IMDType.h"
#include "naucrates/md/IMDTypeInt4.h"
#include "naucrates/md/CMDIdGPDB.h"

using namespace gpnaucrates;
using namespace gpmd;
using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CDatumInt4GPDB::CDatumInt4GPDB
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDatumInt4GPDB::CDatumInt4GPDB
	(
	CSystemId sysid,
	INT iVal,
	BOOL is_null
	)
	:
	m_mdid(NULL),
	m_iVal(iVal),
	m_is_null(is_null)
{
	CMDAccessor *pmda = COptCtxt::PoctxtFromTLS()->Pmda();
	IMDId *pmdid = dynamic_cast<const CMDTypeInt4GPDB *>(pmda->PtMDType<IMDTypeInt4>(sysid))->MDId();
	pmdid->AddRef();
	
	m_mdid = pmdid;

	if (IsNull())
	{
		// needed for hash computation
		m_iVal = INT_MAX;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumInt4GPDB::CDatumInt4GPDB
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDatumInt4GPDB::CDatumInt4GPDB
	(
	IMDId *pmdid,
	INT iVal,
	BOOL is_null
	)
	:
	m_mdid(pmdid),
	m_iVal(iVal),
	m_is_null(is_null)
{
	GPOS_ASSERT(NULL != m_mdid);
	GPOS_ASSERT(GPDB_INT4_OID == CMDIdGPDB::PmdidConvert(m_mdid)->OidObjectId());

	if (IsNull())
	{
		// needed for hash computation
		m_iVal = INT_MAX;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumInt4GPDB::~CDatumInt4GPDB
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDatumInt4GPDB::~CDatumInt4GPDB()
{
	m_mdid->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumInt4GPDB::Value
//
//	@doc:
//		Accessor of integer value
//
//---------------------------------------------------------------------------
INT
CDatumInt4GPDB::Value() const
{
	return m_iVal;
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumInt4GPDB::IsNull
//
//	@doc:
//		Accessor of is null
//
//---------------------------------------------------------------------------
BOOL
CDatumInt4GPDB::IsNull() const
{
	return m_is_null;
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumInt4GPDB::UlSize
//
//	@doc:
//		Accessor of size
//
//---------------------------------------------------------------------------
ULONG
CDatumInt4GPDB::UlSize() const
{
	return 4;
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumInt4GPDB::MDId
//
//	@doc:
//		Accessor of type information
//
//---------------------------------------------------------------------------
IMDId *
CDatumInt4GPDB::MDId() const
{
	return m_mdid;
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumInt4GPDB::HashValue
//
//	@doc:
//		Hash function
//
//---------------------------------------------------------------------------
ULONG
CDatumInt4GPDB::HashValue() const
{
	return gpos::CombineHashes(m_mdid->HashValue(), gpos::HashValue<INT>(&m_iVal));
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumInt4GPDB::Pstr
//
//	@doc:
//		Return string representation
//
//---------------------------------------------------------------------------
const CWStringConst *
CDatumInt4GPDB::Pstr
	(
	IMemoryPool *memory_pool
	)
	const
{
	CWStringDynamic str(memory_pool);
	if (!IsNull())
	{
		str.AppendFormat(GPOS_WSZ_LIT("%d"), m_iVal);
	}
	else
	{
		str.AppendFormat(GPOS_WSZ_LIT("null"));
	}

	return GPOS_NEW(memory_pool) CWStringConst(memory_pool, str.GetBuffer());
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumInt4GPDB::FMatch
//
//	@doc:
//		Matches the values of datums
//
//---------------------------------------------------------------------------
BOOL
CDatumInt4GPDB::FMatch
	(
	const IDatum *pdatum
	)
	const
{
	if(!pdatum->MDId()->Equals(m_mdid))
	{
		return false;
	}

	const CDatumInt4GPDB *pdatumint4 = dynamic_cast<const CDatumInt4GPDB *>(pdatum);

	if(!pdatumint4->IsNull() && !IsNull())
	{
		return (pdatumint4->Value() == Value());
	}

	if(pdatumint4->IsNull() && IsNull())
	{
		return true;
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumInt4GPDB::PdatumCopy
//
//	@doc:
//		Returns a copy of the datum
//
//---------------------------------------------------------------------------
IDatum *
CDatumInt4GPDB::PdatumCopy
	(
	IMemoryPool *memory_pool
	)
	const
{
	m_mdid->AddRef();
	return GPOS_NEW(memory_pool) CDatumInt4GPDB(m_mdid, m_iVal, m_is_null);
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumInt4GPDB::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CDatumInt4GPDB::OsPrint
	(
	IOstream &os
	)
	const
{
	if (!IsNull())
	{
		os << m_iVal;
	}
	else
	{
		os << "null";
	}

	return os;
}

// EOF

