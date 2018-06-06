//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CColRefSet.cpp
//
//	@doc:
//		Implementation of column reference set based on bit sets
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpopt/base/CColRefSet.h"

#include "gpopt/base/CColRefSetIter.h"

#include "gpopt/base/CAutoOptCtxt.h"
#include "gpopt/base/CColumnFactory.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CColRefSet::CColRefSet
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CColRefSet::CColRefSet
	(
	IMemoryPool *memory_pool,
	ULONG ulSizeBits
	)
	:
	CBitSet(memory_pool, ulSizeBits)
{}


//---------------------------------------------------------------------------
//	@function:
//		CColRefSet::CColRefSet
//
//	@doc:
//		copy ctor;
//
//---------------------------------------------------------------------------
CColRefSet::CColRefSet
	(
	IMemoryPool *memory_pool,
	const CColRefSet &bs
	)
	:
	CBitSet(memory_pool, bs)
{}


//---------------------------------------------------------------------------
//	@function:
//		CColRefSet::CColRefSet
//
//	@doc:
//		ctor, copy from col refs array
//
//---------------------------------------------------------------------------
CColRefSet::CColRefSet
	(
	IMemoryPool *memory_pool,
	const DrgPcr *pdrgpcr,
	ULONG ulSize
	)
	:
	CBitSet(memory_pool, ulSize)
{
	Include(pdrgpcr);
}


//---------------------------------------------------------------------------
//	@function:
//		CColRefSet::~CColRefSet
//
//	@doc:
//		dtor
//
//---------------------------------------------------------------------------
CColRefSet::~CColRefSet()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CColRefSet::FMember
//
//	@doc:
//		Check if given column ref is in the set
//
//---------------------------------------------------------------------------
BOOL 
CColRefSet::FMember
	(
	const CColRef *pcr
	)
	const
{
	return CBitSet::Get(pcr->Id());
}


//---------------------------------------------------------------------------
//	@function:
//		CColRefSet::PcrAny
//
//	@doc:
//		Return random member
//
//---------------------------------------------------------------------------
CColRef *
CColRefSet::PcrAny() const
{
	// for now return the first column
	return PcrFirst();
}

//---------------------------------------------------------------------------
//	@function:
//		CColRefSet::PcrFirst
//
//	@doc:
//		Return first member
//
//---------------------------------------------------------------------------
CColRef *
CColRefSet::PcrFirst() const
{
	CColRefSetIter crsi(*this);
	if (crsi.Advance())
	{
		return crsi.Pcr();
	}
	
	GPOS_ASSERT(0 == Size());
	return NULL;
}

//---------------------------------------------------------------------------
//	@function:
//		CColRefSet::Include
//
//	@doc:
//		Include a constant column ref in set
//
//---------------------------------------------------------------------------
void
CColRefSet::Include
	(
	const CColRef *pcr
	)
{
	CBitSet::ExchangeSet(pcr->Id());
}


//---------------------------------------------------------------------------
//	@function:
//		CColRefSet::Include
//
//	@doc:
//		Include column refs from an array
//
//---------------------------------------------------------------------------
void 
CColRefSet::Include
	(
	const DrgPcr *pdrgpcr
	)
{
	ULONG length = pdrgpcr->Size();
	for (ULONG i = 0; i < length; i++)
	{
		Include((*pdrgpcr)[i]);
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CColRefSet::Include
//
//	@doc:
//		Include a set of columns in bitset
//
//---------------------------------------------------------------------------
void
CColRefSet::Include
	(
	const CColRefSet *pcrs
	)
{
	CColRefSetIter crsi(*pcrs);
	while(crsi.Advance())
	{
		Include(crsi.Pcr());
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CColRefSet::Exclude
//
//	@doc:
//		Remove column from bitset
//
//---------------------------------------------------------------------------
void 
CColRefSet::Exclude
	(
	const CColRef *pcr
	)
{
	CBitSet::ExchangeClear(pcr->Id());
}


//---------------------------------------------------------------------------
//	@function:
//		CColRefSet::Exclude
//
//	@doc:
//		Remove a set of columns from bitset
//
//---------------------------------------------------------------------------
void
CColRefSet::Exclude
	(
	const CColRefSet *pcrs
	)
{
	CColRefSetIter crsi(*pcrs);
	while(crsi.Advance())
	{
		Exclude(crsi.Pcr());
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CColRefSet::Exclude
//
//	@doc:
//		Remove an array of columns from bitset
//
//---------------------------------------------------------------------------
void
CColRefSet::Exclude
	(
	const DrgPcr *pdrgpcr
	)
{
	for (ULONG i = 0; i < pdrgpcr->Size(); i++)
	{
		Exclude((*pdrgpcr)[i]);
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CColRefSet::Replace
//
//	@doc:
//		Replace column with another column in bitset
//
//---------------------------------------------------------------------------
void
CColRefSet::Replace
	(
	const CColRef *pcrOut,
	const CColRef *pcrIn
	)
{
	if (FMember(pcrOut))
	{
		Exclude(pcrOut);
		Include(pcrIn);
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CColRefSet::Replace
//
//	@doc:
//		Replace an array of columns with another array of columns
//
//---------------------------------------------------------------------------
void
CColRefSet::Replace
	(
	const DrgPcr *pdrgpcrOut,
	const DrgPcr *pdrgpcrIn
	)
{
	const ULONG ulLen = pdrgpcrOut->Size();
	GPOS_ASSERT(ulLen == pdrgpcrIn->Size());

	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		Replace((*pdrgpcrOut)[ul], (*pdrgpcrIn)[ul]);
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CColRefSet::Pdrgpcr
//
//	@doc:
//		Convert set into array
//
//---------------------------------------------------------------------------
DrgPcr *
CColRefSet::Pdrgpcr
	(
	IMemoryPool *memory_pool
	)
	const
{
	DrgPcr *pdrgpcr = GPOS_NEW(memory_pool) DrgPcr(memory_pool);
	
	CColRefSetIter crsi(*this);
	while(crsi.Advance())
	{
		pdrgpcr->Append(crsi.Pcr());
	}
	
	return pdrgpcr;
}


//---------------------------------------------------------------------------
//	@function:
//		CColRefSet::HashValue
//
//	@doc:
//		Compute hash value by combining hashes of components
//
//---------------------------------------------------------------------------
ULONG
CColRefSet::HashValue()
{
	ULONG ulSize = this->Size();
	ULONG ulHash = gpos::HashValue<ULONG>(&ulSize);
	
	// limit the number of columns used in hash computation
	ULONG ulLen = std::min(ulSize, (ULONG) 8);

	CColRefSetIter crsi(*this);
	for (ULONG i = 0; i < ulLen; i++)
	{
		(void) crsi.Advance();
		ulHash = gpos::CombineHashes(ulHash, gpos::HashPtr<CColRef>(crsi.Pcr()));
	}

	return ulHash;
}


//---------------------------------------------------------------------------
//	@function:
//		CColRefSet::OsPrint
//
//	@doc:
//		Helper function to print a colref set
//
//---------------------------------------------------------------------------
IOstream &
CColRefSet::OsPrint
	(
	IOstream &os,
	ULONG ulLenMax
	)
	const
{
	ULONG ulLen = Size();
	ULONG ul = 0;
	
	CColRefSetIter crsi(*this);
	while(crsi.Advance() && ul < std::min(ulLen, ulLenMax))
	{
		CColRef *pcr = crsi.Pcr();
		pcr->OsPrint(os);
		if (ul < ulLen - 1)
		{
			os << ", ";
		}
		ul++;
	}
	
	if (ulLenMax < ulLen)
	{
		os << "...";
	}

	return os;
}

//---------------------------------------------------------------------------
//	@function:
//		CColRefSet::ExtractColIds
//
//	@doc:
//		Extract array of column ids from colrefset
//
//---------------------------------------------------------------------------
void
CColRefSet::ExtractColIds
	(
	IMemoryPool *memory_pool,
	ULongPtrArray *pdrgpulColIds
	)
	const
{
	CColRefSetIter crsi(*this);
	while (crsi.Advance())
	{
		CColRef *pcr = crsi.Pcr();
		ULONG col_id = pcr->Id();
		pdrgpulColIds->Append(GPOS_NEW(memory_pool) ULONG(col_id));
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CColRefSet::FContained
//
//	@doc:
//		Check if the current colrefset is a subset of any of the colrefsets in
//		the given array
//
//---------------------------------------------------------------------------
BOOL
CColRefSet::FContained
	(
	const DrgPcrs *pdrgpcrs
	)
{
	GPOS_ASSERT(NULL != pdrgpcrs);

	const ULONG ulLen = pdrgpcrs->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		if ((*pdrgpcrs)[ul]->ContainsAll(this))
		{
			return true;
		}
	}

	return false;
}


//---------------------------------------------------------------------------
//	@function:
//		CColRefSet::FCovered
//
//	@doc:
//		Are the columns in the column reference set covered by the array of
//		column ref sets
//---------------------------------------------------------------------------
BOOL
CColRefSet::FCovered
	(
	DrgPcrs *pdrgpcrs,
	CColRefSet *pcrs
	)
{
	GPOS_ASSERT(NULL != pdrgpcrs);
	GPOS_ASSERT(NULL != pcrs);
	GPOS_ASSERT(0 < pdrgpcrs->Size());

	if (0 == pcrs->Size())
	{
		return false;
	}

	CColRefSetIter crsi(*pcrs);
	while (crsi.Advance())
	{
		CColRef *pcr = crsi.Pcr();
		BOOL fFound = false;
		const ULONG ulLen =  pdrgpcrs->Size();
		for (ULONG ul = 0; ul < ulLen && !fFound; ul++)
		{
			CColRefSet *pcrs = (*pdrgpcrs)[ul];
			if (pcrs->FMember(pcr))
			{
				fFound = true;
			}
		}

		if (!fFound)
		{
			return false;
		}
	}

	return true;
}


// EOF
