//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CMDKey.cpp
//
//	@doc:
//		Implementation of a key for metadata cache objects
//---------------------------------------------------------------------------

#include "gpos/io/COstreamString.h"

#include "gpopt/mdcache/CMDKey.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpos;
using namespace gpmd;
using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CMDKey::CMDKey
//
//	@doc:
//		Constructs a md cache key
//
//---------------------------------------------------------------------------
CMDKey::CMDKey
	(
	const IMDId *pmdid
	)
	:
	m_pmdid(pmdid)
{
	GPOS_ASSERT(pmdid->FValid());
}

//---------------------------------------------------------------------------
//	@function:
//		CMDKey::Equals
//
//	@doc:
//		Equality function
//
//---------------------------------------------------------------------------
BOOL
CMDKey::Equals
	(
	const CMDKey &mdkey
	)
	const
{	
	return mdkey.Pmdid()->Equals(m_pmdid);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDKey::FEqualMDKey
//
//	@doc:
//		Equality function for using MD keys in a cache
//
//---------------------------------------------------------------------------
BOOL
CMDKey::FEqualMDKey
	(
	CMDKey* const &pvLeft,
	CMDKey* const &pvRight
	)
{
	if (NULL == pvLeft && NULL == pvRight)
	{
		return true;
	}

	if (NULL == pvLeft || NULL == pvRight)
	{
		return false;
	}
	
	GPOS_ASSERT(NULL != pvLeft && NULL != pvRight);
	
	return pvLeft->Pmdid()->Equals(pvRight->Pmdid());
}

//---------------------------------------------------------------------------
//	@function:
//		CMDKey::HashValue
//
//	@doc:
//		Hash function
//
//---------------------------------------------------------------------------
ULONG 
CMDKey::HashValue() const
{
	return m_pmdid->HashValue();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDKey::UlHashMDKey
//
//	@doc:
//		Hash function for using MD keys in a cache
//
//---------------------------------------------------------------------------
ULONG 
CMDKey::UlHashMDKey
	(
	CMDKey* const & pv
	)
{
	return pv->Pmdid()->HashValue();
}

// EOF
