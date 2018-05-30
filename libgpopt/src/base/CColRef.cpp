//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2008 Greenplum, Inc.
//
//	@filename:
//		CColRef.cpp
//
//	@doc:
//		Implementation of column reference class
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpopt/base/CColRef.h"

using namespace gpopt;

// invalid key
const ULONG CColRef::m_ulInvalid = ULONG_MAX;

//---------------------------------------------------------------------------
//	@function:
//		CColRef::CColRef
//
//	@doc:
//		ctor
//		takes ownership of string; verify string is properly formatted
//
//---------------------------------------------------------------------------
CColRef::CColRef
	(
	const IMDType *pmdtype,
	const INT type_modifier,
	ULONG ulId,
	const CName *pname
	)
	:
	m_pmdtype(pmdtype),
	m_type_modifier(type_modifier),
	m_pname(pname),
	m_ulId(ulId)
{
	GPOS_ASSERT(NULL != pmdtype);
	GPOS_ASSERT(pmdtype->MDId()->IsValid());
	GPOS_ASSERT(NULL != pname);
}


//---------------------------------------------------------------------------
//	@function:
//		CColRef::~CColRef
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CColRef::~CColRef()
{
	// we own the name 
	GPOS_DELETE(m_pname);
}


//---------------------------------------------------------------------------
//	@function:
//		CColRef::HashValue
//
//	@doc:
//		static hash function
//
//---------------------------------------------------------------------------
ULONG
CColRef::HashValue
	(
	const ULONG &ulptr
	)
{
	return gpos::HashValue<ULONG>(&ulptr);
}

//---------------------------------------------------------------------------
//	@function:
//		CColRef::HashValue
//
//	@doc:
//		static hash function
//
//---------------------------------------------------------------------------
ULONG
CColRef::HashValue
	(
	const CColRef *pcr
	)
{
	ULONG ulId = pcr->UlId();
	return gpos::HashValue<ULONG>(&ulId);
}


//---------------------------------------------------------------------------
//	@function:
//		CColRef::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CColRef::OsPrint
	(
	IOstream &os
	)
	const
{
	m_pname->OsPrint(os);
	os << " (" << UlId() << ")";
	
	return os;
}

//---------------------------------------------------------------------------
//	@function:
//		CColRef::Pdrgpul
//
//	@doc:
//		Extract array of colids from array of colrefs
//
//---------------------------------------------------------------------------
ULongPtrArray *
CColRef::Pdrgpul
	(
	IMemoryPool *memory_pool,
	DrgPcr *pdrgpcr
	)
{
	ULongPtrArray *pdrgpul = GPOS_NEW(memory_pool) ULongPtrArray(memory_pool);
	const ULONG ulLen = pdrgpcr->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		CColRef *pcr = (*pdrgpcr)[ul];
		pdrgpul->Append(GPOS_NEW(memory_pool) ULONG(pcr->UlId()));
	}

	return pdrgpul;
}

//---------------------------------------------------------------------------
//	@function:
//		CColRef::Equals
//
//	@doc:
//		Are the two arrays of column references equivalent
//
//---------------------------------------------------------------------------
BOOL
CColRef::Equals
	(
	const DrgPcr *pdrgpcr1,
	const DrgPcr *pdrgpcr2
	)
{
	if (NULL == pdrgpcr1 || NULL == pdrgpcr2)
	{
		return  (NULL == pdrgpcr1 && NULL == pdrgpcr2);
	}

	return pdrgpcr1->Equals(pdrgpcr2);
}

// check if the the array of column references are equal. Note that since we have unique
// copy of the column references, we can compare pointers.
BOOL
CColRef::Equals
	(
	const DrgDrgPcr *pdrgdrgpcr1,
	const DrgDrgPcr *pdrgdrgpcr2
	)
{
	ULONG ulLen1 = (pdrgdrgpcr1 == NULL) ? 0 : pdrgdrgpcr1->Size();
	ULONG ulLen2 = (pdrgdrgpcr2 == NULL) ? 0 : pdrgdrgpcr2->Size();

	if (ulLen1 != ulLen2)
	{
		return false;
	}

	for (ULONG ulLevel = 0; ulLevel < ulLen1; ulLevel++)
	{
		BOOL fEqual = (*pdrgdrgpcr1)[ulLevel]->Equals((*pdrgdrgpcr2)[ulLevel]);
		if (!fEqual)
		{
			return false;
		}
	}

	return true;
}

// EOF

