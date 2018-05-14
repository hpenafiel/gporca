//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2008 Greenplum, Inc.
//
//	@filename:
//		CBitVector.cpp
//
//	@doc:
//		Implementation of simple, static bit vector class
//---------------------------------------------------------------------------


#include "gpos/base.h"
#include "gpos/utils.h"
#include "gpos/common/CBitVector.h"
#include "gpos/common/CAutoRg.h"
#include "gpos/common/clibwrapper.h"

using namespace gpos;

#define BYTES_PER_UNIT	GPOS_SIZEOF(ULLONG)
#define BITS_PER_UNIT	(8 * BYTES_PER_UNIT)

//---------------------------------------------------------------------------
//	@function:
//		CBitVector::Clear
//
//	@doc:
//		wipe all units
//
//---------------------------------------------------------------------------
void
CBitVector::Clear()
{
	GPOS_ASSERT(NULL != m_vec);
	clib::PvMemSet(m_vec, 0, m_len * BYTES_PER_UNIT);
}


//---------------------------------------------------------------------------
//	@function:
//		CBitVector::CBitVector
//
//	@doc:
//		ctor -- allocates actual vector, clears it
//
//---------------------------------------------------------------------------
CBitVector::CBitVector
	(
	IMemoryPool *pmp,
	ULONG cBits
	)
	:
	m_nbits(cBits),
	m_len(0),
	m_vec(NULL)
{
	// determine units needed to represent the number
	m_len = m_nbits / BITS_PER_UNIT;
	if (m_len * BITS_PER_UNIT < m_nbits)
	{
		m_len++;
	}
	
	GPOS_ASSERT(m_len * BITS_PER_UNIT >= m_nbits && "Bit vector sized incorrectly");
	
	// allocate and clear
	m_vec = GPOS_NEW_ARRAY(pmp, ULLONG, m_len);
	
	CAutoRg<ULLONG> argull;
	argull = m_vec;
	
	Clear();
	
	// unhook from protector
	argull.RgtReset();
}


//---------------------------------------------------------------------------
//	@function:
//		CBitVector::~CBitVector
//
//	@doc:
//		dtor
//
//---------------------------------------------------------------------------
CBitVector::~CBitVector()
{
	GPOS_DELETE_ARRAY(m_vec);
}


//---------------------------------------------------------------------------
//	@function:
//		CBitVector::CBitVector
//
//	@doc:
//		copy ctor;
//
//---------------------------------------------------------------------------
CBitVector::CBitVector
	(
	IMemoryPool *pmp,
	const CBitVector &bv
	)
	:
	m_nbits(bv.m_nbits),
	m_len(bv.m_len),
	m_vec(NULL)
{
	
	// deep copy
	m_vec = GPOS_NEW_ARRAY(pmp, ULLONG, m_len);
	
	// Using auto range for cleanliness only;
	// NOTE: 03/25/2008; strictly speaking not necessary since there is
	//		no operation that could fail and it's the only allocation in the 
	//		ctor; 
	CAutoRg<ULLONG> argull;
	argull = m_vec;
	
	clib::PvMemCpy(m_vec, bv.m_vec, BYTES_PER_UNIT * m_len);

	// unhook from protector
	argull.RgtReset();
}


//---------------------------------------------------------------------------
//	@function:
//		CBitVector::Get
//
//	@doc:
//		Check if given bit is set
//
//---------------------------------------------------------------------------
BOOL 
CBitVector::Get
	(
	ULONG ulBit
	)
	const
{
	GPOS_ASSERT(ulBit < m_nbits && "Bit index out of bounds.");

	ULONG cUnit = ulBit / BITS_PER_UNIT;
	ULLONG ullMask = ((ULLONG)1) << (ulBit % BITS_PER_UNIT);
	
	return m_vec[cUnit] & ullMask;
}


//---------------------------------------------------------------------------
//	@function:
//		CBitVector::ExchangeSet
//
//	@doc:
//		Set given bit; return previous value
//
//---------------------------------------------------------------------------
BOOL 
CBitVector::ExchangeSet
	(
	ULONG ulBit
	)
{
	GPOS_ASSERT(ulBit < m_nbits  && "Bit index out of bounds.");

	// CONSIDER: 03/25/2008; make testing for the bit part of this routine and
	// avoid function call
	BOOL fSet = Get(ulBit);
	
	ULONG cUnit = ulBit / BITS_PER_UNIT;
	ULLONG ullMask = ((ULLONG)1) << (ulBit % BITS_PER_UNIT);
	
	// OR the target unit with the mask
	m_vec[cUnit] |= ullMask;
	
	return fSet;
}


//---------------------------------------------------------------------------
//	@function:
//		CBitVector::ExchangeClear
//
//	@doc:
//		Clear given bit; return previous value
//
//---------------------------------------------------------------------------
BOOL 
CBitVector::ExchangeClear
	(
	ULONG ulBit
	)
{
	GPOS_ASSERT(ulBit < m_nbits && "Bit index out of bounds.");

	// CONSIDER: 03/25/2008; make testing for the bit part of this routine and
	// avoid function call
	BOOL fSet = Get(ulBit);
	
	ULONG cUnit = ulBit / BITS_PER_UNIT;
	ULLONG ullMask = ((ULLONG)1) << (ulBit % BITS_PER_UNIT);
	
	// AND the target unit with the inverted mask
	m_vec[cUnit] &= ~ullMask;
	
	return fSet;
}


//---------------------------------------------------------------------------
//	@function:
//		CBitVector::Union
//
//	@doc:
//		Union with given other vector
//
//---------------------------------------------------------------------------
void
CBitVector::Or
	(
	const CBitVector *pbv
	)
{
	GPOS_ASSERT(m_nbits == pbv->m_nbits && m_len == pbv->m_len && 
		"vectors must be of same size");
		
	// OR all components
	for(ULONG i = 0; i < m_len; i++)
	{
		m_vec[i] |= pbv->m_vec[i];
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CBitVector::Intersection
//
//	@doc:
//		Intersect with given other vector
//
//---------------------------------------------------------------------------
void
CBitVector::And
	(
	const CBitVector *pbv
	)
{
	GPOS_ASSERT(m_nbits == pbv->m_nbits && m_len == pbv->m_len && 
		"vectors must be of same size");
		
	// AND all components
	for(ULONG i = 0; i < m_len; i++)
	{
		m_vec[i] &= pbv->m_vec[i];
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CBitVector::FSubset
//
//	@doc:
//		Determine if given vector is subset
//
//---------------------------------------------------------------------------
BOOL
CBitVector::Contains
	(
	const CBitVector *pbv
	)
	const
{
	GPOS_ASSERT(m_nbits == pbv->m_nbits && m_len == pbv->m_len && 
		"vectors must be of same size");
		
	// OR all components
	for(ULONG i = 0; i < m_len; i++)
	{
		ULLONG ull = m_vec[i] & pbv->m_vec[i];
		if (ull != pbv->m_vec[i])
		{
			return false;
		}
	}
	
	return true;
}


//---------------------------------------------------------------------------
//	@function:
//		CBitVector::FDisjoint
//
//	@doc:
//		Determine if given vector is disjoint
//
//---------------------------------------------------------------------------
BOOL
CBitVector::IsDisjoint
	(
	const CBitVector *pbv
	)
	const
{
	GPOS_ASSERT(m_nbits == pbv->m_nbits && m_len == pbv->m_len && 
		"vectors must be of same size");

	for(ULONG i = 0; i < m_len; i++)
	{
		if (0 != (m_vec[i] & pbv->m_vec[i]))
		{
			return false;
		}
	}
	
	return true;
}


//---------------------------------------------------------------------------
//	@function:
//		CBitVector::FEqual
//
//	@doc:
//		Determine if equal
//
//---------------------------------------------------------------------------
BOOL
CBitVector::FEqual
	(
	const CBitVector *pbv
	)
	const
{
	GPOS_ASSERT(m_nbits == pbv->m_nbits && m_len == pbv->m_len && 
		"vectors must be of same size");
		
	// compare all components
	if (0 == clib::IMemCmp(m_vec, pbv->m_vec, m_len * BYTES_PER_UNIT))
	{
		GPOS_ASSERT(this->Contains(pbv) && pbv->Contains(this));
		return true;
	}
	
	return false;
}



//---------------------------------------------------------------------------
//	@function:
//		CBitVector::FEmpty
//
//	@doc:
//		Determine if vector is empty
//
//---------------------------------------------------------------------------
BOOL
CBitVector::FEmpty() const
{
	for (ULONG i = 0; i < m_len; i++)
	{
		if (0 != m_vec[i])
		{
			return false;
		}
	}
	
	return true;
}


//---------------------------------------------------------------------------
//	@function:
//		CBitVector::GetNextBit
//
//	@doc:
//		Determine the next bit set greater or equal than the provided position
//
//---------------------------------------------------------------------------
BOOL
CBitVector::GetNextBit
	(
	ULONG ulStart,
	ULONG &ulNext
	)
	const
{	
	ULONG ulOffset = ulStart % BITS_PER_UNIT;
	for (ULONG cUnit = ulStart / BITS_PER_UNIT; cUnit < m_len; cUnit++)
	{
		ULLONG ull = m_vec[cUnit] >> ulOffset;
		
		ULONG ulBit = ulOffset;
		while(0 != ull && 0 == (ull & (ULLONG)1))
		{
			ull >>= 1;
			ulBit++;
		}
		
		// if any bits left we found the next set position
		if (0 != ull)
		{
			ulNext = ulBit + (cUnit * BITS_PER_UNIT);
			return true;
		}
		
		// the initial offset applies only to the first chunk
		ulOffset = 0;
	}
	
	return false;
}


//---------------------------------------------------------------------------
//	@function:
//		CBitVector::CElements
//
//	@doc:
//		Count bits in vector
//
//---------------------------------------------------------------------------
ULONG
CBitVector::CountSetBits() const
{
	ULONG cBits = 0;
	for (ULONG i = 0; i < m_len; i++)
	{	
		ULLONG ull = m_vec[i];
		ULONG j = 0;
	
		for(j = 0; ull != 0; j++)
		{
			ull &= (ull - 1);
		}

		cBits += j;
	}
	
	return cBits;
}


//---------------------------------------------------------------------------
//	@function:
//		CBitVector::UlHash
//
//	@doc:
//		Compute hash value for bit vector
//
//---------------------------------------------------------------------------
ULONG
CBitVector::UlHash() const
{
	return gpos::UlHashByteArray((BYTE*)&m_vec[0], GPOS_SIZEOF(m_vec[0]) * m_len);
}
		
		
// EOF

