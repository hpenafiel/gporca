//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CDatumBoolGPDB.cpp
//
//	@doc:
//		Implementation of GPDB bool
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/gpdb_types.h"

#include "naucrates/base/CDatumBoolGPDB.h"
#include "gpopt/base/CAutoOptCtxt.h"
#include "gpopt/mdcache/CMDAccessor.h"

#include "naucrates/md/IMDType.h"
#include "naucrates/md/IMDTypeBool.h"
#include "naucrates/md/CMDIdGPDB.h"

using namespace gpnaucrates;
using namespace gpopt;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CDatumBoolGPDB::CDatumBoolGPDB
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDatumBoolGPDB::CDatumBoolGPDB
	(
	CSystemId sysid,
	BOOL value,
	BOOL is_null
	)
	:
	m_value(value),
	m_is_null(is_null)
{
	CMDAccessor *pmda = COptCtxt::PoctxtFromTLS()->Pmda();
	IMDId *pmdid = dynamic_cast<const CMDTypeBoolGPDB *>(pmda->PtMDType<IMDTypeBool>(sysid))->MDId();
	pmdid->AddRef();
	
	m_pmdid = pmdid;

	if (IsNull())
	{
		// needed for hash computation
		m_value = false;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumBoolGPDB::CDatumBoolGPDB
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDatumBoolGPDB::CDatumBoolGPDB
	(
	IMDId *pmdid,
	BOOL value,
	BOOL is_null
	)
	:
	m_pmdid(pmdid),
	m_value(value),
	m_is_null(is_null)
{
	GPOS_ASSERT(NULL != m_pmdid);
	GPOS_ASSERT(GPDB_BOOL_OID == CMDIdGPDB::PmdidConvert(m_pmdid)->OidObjectId());

	if (IsNull())
	{
		// needed for hash computation
		m_value = false;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumBoolGPDB::~CDatumBoolGPDB
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDatumBoolGPDB::~CDatumBoolGPDB()
{
	m_pmdid->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumBoolGPDB::FValue
//
//	@doc:
//		Accessor of boolean value
//
//---------------------------------------------------------------------------
BOOL
CDatumBoolGPDB::FValue() const
{
	return m_value;
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumBoolGPDB::IsNull
//
//	@doc:
//		Accessor of is null
//
//---------------------------------------------------------------------------
BOOL
CDatumBoolGPDB::IsNull() const
{
	return m_is_null;
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumBoolGPDB::UlSize
//
//	@doc:
//		Accessor of size
//
//---------------------------------------------------------------------------
ULONG
CDatumBoolGPDB::UlSize() const
{
	return 1;
}


//---------------------------------------------------------------------------
//	@function:
//		CDatumBoolGPDB::MDId
//
//	@doc:
//		Accessor of type information (MDId)
//
//---------------------------------------------------------------------------
IMDId *
CDatumBoolGPDB::MDId() const
{
	return m_pmdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumBoolGPDB::HashValue
//
//	@doc:
//		Hash function
//
//---------------------------------------------------------------------------
ULONG
CDatumBoolGPDB::HashValue() const
{
	return gpos::CombineHashes(m_pmdid->HashValue(), gpos::HashValue<BOOL>(&m_value));
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumBoolGPDB::Pstr
//
//	@doc:
//		Return string representation
//
//---------------------------------------------------------------------------
const CWStringConst *
CDatumBoolGPDB::Pstr
	(
	IMemoryPool *memory_pool
	)
	const
{
	CWStringDynamic str(memory_pool);
	if (!IsNull())
	{
		str.AppendFormat(GPOS_WSZ_LIT("%d"), m_value);
	}
	else
	{
		str.AppendFormat(GPOS_WSZ_LIT("null"));
	}

	return GPOS_NEW(memory_pool) CWStringConst(memory_pool, str.GetBuffer());
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumBoolGPDB::FMatch
//
//	@doc:
//		Matches the values of datums
//
//---------------------------------------------------------------------------
BOOL
CDatumBoolGPDB::FMatch
	(
	const IDatum *pdatum
	)
	const
{
	if(!pdatum->MDId()->Equals(m_pmdid))
	{
		return false;
	}

	const CDatumBoolGPDB *pdatumbool = dynamic_cast<const CDatumBoolGPDB *>(pdatum);

	if(!pdatumbool->IsNull() && !IsNull())
	{
		return (pdatumbool->FValue() == FValue());
	}

	if(pdatumbool->IsNull() && IsNull())
	{
		return true;
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumBoolGPDB::PdatumCopy
//
//	@doc:
//		Returns a copy of the datum
//
//---------------------------------------------------------------------------
IDatum *
CDatumBoolGPDB::PdatumCopy
	(
	IMemoryPool *memory_pool
	)
	const
{
	m_pmdid->AddRef();
	return GPOS_NEW(memory_pool) CDatumBoolGPDB(m_pmdid, m_value, m_is_null);
}

//---------------------------------------------------------------------------
//	@function:
//		CDatumBoolGPDB::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CDatumBoolGPDB::OsPrint
	(
	IOstream &os
	)
	const
{
	if (!IsNull())
	{
		os << m_value;
	}
	else
	{
		os << "null";
	}

	return os;
}

// EOF

