//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CDatumInt2GPDB.cpp
//
//	@doc:
//		Implementation of GPDB int2
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/gpdb_types.h"

#include "naucrates/base/CDatumInt2GPDB.h"
#include "gpopt/base/CAutoOptCtxt.h"
#include "gpopt/mdcache/CMDAccessor.h"

#include "naucrates/md/IMDType.h"
#include "naucrates/md/IMDTypeInt2.h"
#include "naucrates/md/CMDIdGPDB.h"

using namespace gpnaucrates;
using namespace gpmd;
using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CDatumInt2GPDB::CDatumInt2GPDB
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDatumInt2GPDB::CDatumInt2GPDB
	(
	CSystemId sysid,
	SINT sVal,
	BOOL is_null
	)
	:
	m_pmdid(NULL),
	m_sVal(sVal),
	m_is_null(is_null)
{
	CMDAccessor *pmda = COptCtxt::PoctxtFromTLS()->Pmda();
	IMDId *pmdid = dynamic_cast<const CMDTypeInt2GPDB *>(pmda->PtMDType<IMDTypeInt2>(sysid))->MDId();
	pmdid->AddRef();
	
	m_pmdid = pmdid;

	if (IsNull())
	{
		// needed for hash computation
		m_sVal = SINT_MAX;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumInt2GPDB::CDatumInt2GPDB
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDatumInt2GPDB::CDatumInt2GPDB
	(
	IMDId *pmdid,
	SINT sVal,
	BOOL is_null
	)
	:
	m_pmdid(pmdid),
	m_sVal(sVal),
	m_is_null(is_null)
{
	GPOS_ASSERT(NULL != m_pmdid);
	GPOS_ASSERT(GPDB_INT2_OID == CMDIdGPDB::PmdidConvert(m_pmdid)->OidObjectId());

	if (IsNull())
	{
		// needed for hash computation
		m_sVal = SINT_MAX;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumInt2GPDB::~CDatumInt2GPDB
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDatumInt2GPDB::~CDatumInt2GPDB()
{
	m_pmdid->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumInt2GPDB::Value
//
//	@doc:
//		Accessor of integer value
//
//---------------------------------------------------------------------------
SINT
CDatumInt2GPDB::Value() const
{
	return m_sVal;
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumInt2GPDB::IsNull
//
//	@doc:
//		Accessor of is null
//
//---------------------------------------------------------------------------
BOOL
CDatumInt2GPDB::IsNull() const
{
	return m_is_null;
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumInt2GPDB::UlSize
//
//	@doc:
//		Accessor of size
//
//---------------------------------------------------------------------------
ULONG
CDatumInt2GPDB::UlSize() const
{
	return 2;
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumInt2GPDB::MDId
//
//	@doc:
//		Accessor of type information
//
//---------------------------------------------------------------------------
IMDId *
CDatumInt2GPDB::MDId() const
{
	return m_pmdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumInt2GPDB::HashValue
//
//	@doc:
//		Hash function
//
//---------------------------------------------------------------------------
ULONG
CDatumInt2GPDB::HashValue() const
{
	return gpos::CombineHashes(m_pmdid->HashValue(), gpos::HashValue<SINT>(&m_sVal));
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumInt2GPDB::Pstr
//
//	@doc:
//		Return string representation
//
//---------------------------------------------------------------------------
const CWStringConst *
CDatumInt2GPDB::Pstr
	(
	IMemoryPool *memory_pool
	)
	const
{
	CWStringDynamic str(memory_pool);
	if (!IsNull())
	{
		str.AppendFormat(GPOS_WSZ_LIT("%d"), m_sVal);
	}
	else
	{
		str.AppendFormat(GPOS_WSZ_LIT("null"));
	}

	return GPOS_NEW(memory_pool) CWStringConst(memory_pool, str.GetBuffer());
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumInt2GPDB::FMatch
//
//	@doc:
//		Matches the values of datums
//
//---------------------------------------------------------------------------
BOOL
CDatumInt2GPDB::FMatch
	(
	const IDatum *pdatum
	)
	const
{
	if (!pdatum->MDId()->Equals(m_pmdid))
	{
		return false;
	}

	const CDatumInt2GPDB *pdatumint2 = dynamic_cast<const CDatumInt2GPDB *>(pdatum);

	if (!pdatumint2->IsNull() && !IsNull())
	{
		return (pdatumint2->Value() == Value());
	}

	return pdatumint2->IsNull() && IsNull();
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumInt2GPDB::PdatumCopy
//
//	@doc:
//		Returns a copy of the datum
//
//---------------------------------------------------------------------------
IDatum *
CDatumInt2GPDB::PdatumCopy
	(
	IMemoryPool *memory_pool
	)
	const
{
	m_pmdid->AddRef();
	return GPOS_NEW(memory_pool) CDatumInt2GPDB(m_pmdid, m_sVal, m_is_null);
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumInt2GPDB::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CDatumInt2GPDB::OsPrint
	(
	IOstream &os
	)
	const
{
	if (!IsNull())
	{
		os << m_sVal;
	}
	else
	{
		os << "null";
	}

	return os;
}

// EOF

