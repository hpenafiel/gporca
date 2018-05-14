//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CBitSet.cpp
//
//	@doc:
//		Implementation of bit sets
//
//		Underlying assumption: a set contains only a few links of bitvectors
//		hence, keeping them in a linked list is efficient;
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpos/common/CAutoRef.h"
#include "gpos/common/CBitSet.h"
#include "gpos/common/CBitSetIter.h"

#ifdef GPOS_DEBUG
#include "gpos/error/CAutoTrace.h"
#endif // GPOS_DEBUG

using namespace gpos;


//---------------------------------------------------------------------------
//	@function:
//		CBitSetLink
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CBitSet::CBitSetLink::CBitSetLink
	(
	IMemoryPool *pmp, 
	ULONG offset, 
	ULONG vector_size
	)
	: 
	m_offset(offset)
{
	m_vec = GPOS_NEW(pmp) CBitVector(pmp, vector_size);
}


//---------------------------------------------------------------------------
//	@function:
//		CBitSetLink
//
//	@doc:
//		copy ctor
//
//---------------------------------------------------------------------------
CBitSet::CBitSetLink::CBitSetLink
	(
	IMemoryPool *pmp, 
	const CBitSetLink &bsl
	)
	: 
	m_offset(bsl.m_offset)
{
	m_vec = GPOS_NEW(pmp) CBitVector(pmp, *bsl.GetVec());
}


//---------------------------------------------------------------------------
//	@function:
//		~CBitSetLink
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CBitSet::CBitSetLink::~CBitSetLink()
{
	GPOS_DELETE(m_vec);
}


//---------------------------------------------------------------------------
//	@function:
//		CBitSet::FindLinkByOffset
//
//	@doc:
//		Find bit set link for a given offset; if non-existent return previous
//		link (may be NULL);
//		By providing a starting link we can implement a number of operations
//		in one sweep, ie O(N) 
//
//---------------------------------------------------------------------------
CBitSet::CBitSetLink *
CBitSet::FindLinkByOffset
	(
	ULONG offset,
	CBitSetLink *pbsl
	)
	const
{
	CBitSetLink *found = NULL;
	CBitSetLink *cursor = pbsl;
	
	if (NULL == pbsl)
	{
		// if no cursor provided start with first element
		cursor = m_bsllist.First();
	}
	else
	{
		GPOS_ASSERT(pbsl->GetOffset() <= offset && "invalid start cursor");
		found = pbsl;
	}
	
	GPOS_ASSERT_IMP(NULL != cursor, GPOS_OK == m_bsllist.Find(cursor) && "cursor not in list");
	
	while(1)
	{
		// no more links or we've overshot the target
		if (NULL == cursor || cursor->GetOffset() > offset)
		{
			break;
		}
		
		found = cursor;		
		cursor = m_bsllist.Next(cursor);
	}
	
	GPOS_ASSERT_IMP(found, found->GetOffset() <= offset);
	return found;
}


//---------------------------------------------------------------------------
//	@function:
//		CBitSet::RecomputeSize
//
//	@doc:
//		Compute size of set by adding up sizes of links
//
//---------------------------------------------------------------------------
void
CBitSet::RecomputeSize()
{
	m_size = 0;
	CBitSetLink *pbsl = NULL;
	
	for (
		pbsl = m_bsllist.First();
		pbsl != NULL;
		pbsl = m_bsllist.Next(pbsl)
		)
	{
		m_size += pbsl->GetVec()->CountSetBits();
	}	
}


//---------------------------------------------------------------------------
//	@function:
//		CBitSet::Clear
//
//	@doc:
//		release all links
//
//---------------------------------------------------------------------------
void
CBitSet::Clear()
{
	CBitSetLink *pbsl = NULL;
	
	while(NULL != (pbsl = m_bsllist.First()))
	{		
		CBitSetLink *pbslRemove = pbsl;
		pbsl = m_bsllist.Next(pbsl);
		
		m_bsllist.Remove(pbslRemove);
		GPOS_DELETE(pbslRemove);
	}
	
	RecomputeSize();
}


//---------------------------------------------------------------------------
//	@function:
//		CBitSet::GetOffset
//
//	@doc:
//		Compute offset 
//
//---------------------------------------------------------------------------
ULONG
CBitSet::ComputeOffset
	(
	ULONG ul
	)
	const
{
	return (ul / m_vector_size) * m_vector_size;
}



//---------------------------------------------------------------------------
//	@function:
//		CBitSet::CBitSet
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CBitSet::CBitSet
	(
	IMemoryPool *pmp,
	ULONG vector_size
	)
	:
	m_pmp(pmp),
	m_vector_size(vector_size),
	m_size(0)
{
	m_bsllist.Init(GPOS_OFFSET(CBitSetLink, m_link));
}


//---------------------------------------------------------------------------
//	@function:
//		CBitSet::CBitSet
//
//	@doc:
//		copy ctor;
//
//---------------------------------------------------------------------------
CBitSet::CBitSet
	(
	IMemoryPool *pmp,
	const CBitSet &bs
	)
	:
	m_pmp(pmp),
	m_vector_size(bs.m_vector_size),
	m_size(0)
{
	m_bsllist.Init(GPOS_OFFSET(CBitSetLink, m_link));
	Union(&bs);
}


//---------------------------------------------------------------------------
//	@function:
//		CBitSet::~CBitSet
//
//	@doc:
//		dtor
//
//---------------------------------------------------------------------------
CBitSet::~CBitSet()
{
	Clear();
}


//---------------------------------------------------------------------------
//	@function:
//		CBitSet::Get
//
//	@doc:
//		Check if given bit is set
//
//---------------------------------------------------------------------------
BOOL 
CBitSet::Get
	(
	ULONG pos
	)
	const
{
	ULONG offset = ComputeOffset(pos);
	
	CBitSetLink *pbsl = FindLinkByOffset(offset);
	if (NULL != pbsl && pbsl->GetOffset() == offset)
	{
		return pbsl->GetVec()->Get(pos - offset);
	}
	
	return false;
}


//---------------------------------------------------------------------------
//	@function:
//		CBitSet::ExchangeSet
//
//	@doc:
//		Set given bit; return previous value; allocate new link if necessary
//
//---------------------------------------------------------------------------
BOOL 
CBitSet::ExchangeSet
	(
	ULONG pos
	)
{
	ULONG offset = ComputeOffset(pos);
	
	CBitSetLink *pbsl = FindLinkByOffset(offset);
	if (NULL == pbsl || pbsl->GetOffset() != offset)
	{
		CBitSetLink *pbsl_new = GPOS_NEW(m_pmp) CBitSetLink(m_pmp, offset, m_vector_size);
		if (NULL == pbsl)
		{
			m_bsllist.Prepend(pbsl_new);
		}
		else
		{
			// insert after found link
			m_bsllist.Append(pbsl_new, pbsl);
		}
		
		pbsl = pbsl_new;
	}
	
	GPOS_ASSERT(pbsl->GetOffset() == offset);
	
	BOOL fBit = pbsl->GetVec()->ExchangeSet(pos - offset);
	if (!fBit)
	{
		m_size++;
	}
	
	return fBit;
}


//---------------------------------------------------------------------------
//	@function:
//		CBitSet::ExchangeClear
//
//	@doc:
//		Clear given bit; return previous value
//
//---------------------------------------------------------------------------
BOOL 
CBitSet::ExchangeClear
	(
	ULONG pos
	)
{
	ULONG offset = ComputeOffset(pos);
	
	CBitSetLink *pbsl = FindLinkByOffset(offset);
	if (NULL != pbsl && pbsl->GetOffset() == offset)
	{
		BOOL fBit = pbsl->GetVec()->ExchangeClear(pos - offset);
		
		// remove empty link
		if (pbsl->GetVec()->IsEmpty())
		{
			m_bsllist.Remove(pbsl);
			GPOS_DELETE(pbsl);
		}
		
		if (fBit)
		{
			m_size--;
		}
		
		return fBit;
	}
	
	return false;
}


//---------------------------------------------------------------------------
//	@function:
//		CBitSet::Union
//
//	@doc:
//		Union with given other set;
//		(1) determine which links need to be allocated before(!) modifying
//			the set allocate and copy missing links aside
//		(2) insert the new links into the list
//		(3) union all links, old and new, on a per-bitvector basis
//
//		For clarity step (2) and (3) are separated;
//
//---------------------------------------------------------------------------
void
CBitSet::Union
	(
	const CBitSet *pbsOther
	)
{
	CBitSetLink *pbsl = NULL;
	CBitSetLink *pbslOther = NULL;

	// dynamic array of CBitSetLink
	typedef CDynamicPtrArray <CBitSetLink, CleanupNULL> DrgBSL;

	CAutoRef<DrgBSL> a_drgpbsl;
	a_drgpbsl = GPOS_NEW(m_pmp) DrgBSL(m_pmp);
	
	// iterate through other's links and copy missing links to array
	for (
		pbslOther = pbsOther->m_bsllist.First();
		pbslOther != NULL;
		pbslOther = pbsOther->m_bsllist.Next(pbslOther)
		)
	{
		pbsl = FindLinkByOffset(pbslOther->GetOffset(), pbsl);
		if (NULL == pbsl || pbsl->GetOffset() != pbslOther->GetOffset())
		{
			// need to copy this link
			CAutoP<CBitSetLink> a_pbsl;
			a_pbsl = GPOS_NEW(m_pmp) CBitSetLink(m_pmp, *pbslOther);
			a_drgpbsl->Append(a_pbsl.Pt());
			
			a_pbsl.PtReset();
		}
	}

	// insert all new links
	pbsl = NULL;
	for (ULONG i = 0; i < a_drgpbsl->Size(); i++)
	{
		CBitSetLink *pbslInsert = (*a_drgpbsl)[i];
		pbsl = FindLinkByOffset(pbslInsert->GetOffset(), pbsl);
		
		GPOS_ASSERT_IMP(NULL != pbsl, pbsl->GetOffset() < pbslInsert->GetOffset());
		if (NULL == pbsl)
		{
			m_bsllist.Prepend(pbslInsert);
		}
		else
		{
			m_bsllist.Append(pbslInsert, pbsl);
		}
	}
	
	// iterate through all links and union them up
	pbslOther = NULL;
	pbsl = m_bsllist.First();
	while (NULL != pbsl)
	{
		pbslOther = pbsOther->FindLinkByOffset(pbsl->GetOffset(), pbslOther);
		if (NULL != pbslOther && pbslOther->GetOffset() == pbsl->GetOffset())
		{
			pbsl->GetVec()->Or(pbslOther->GetVec());
		}
		
		pbsl = m_bsllist.Next(pbsl);
	}
	
	RecomputeSize();
}


//---------------------------------------------------------------------------
//	@function:
//		CBitSet::Intersection
//
//	@doc:
//		Iterate through all links and intersect them; release unused links
//
//---------------------------------------------------------------------------
void
CBitSet::Intersection
	(
	const CBitSet *pbsOther
	)
{
	CBitSetLink *pbslOther = NULL;
	CBitSetLink *pbsl = m_bsllist.First();
	
	while (NULL != pbsl)
	{
		CBitSetLink *pbslRemove = NULL;

		pbslOther = pbsOther->FindLinkByOffset(pbsl->GetOffset(), pbslOther);
		if (NULL != pbslOther && pbslOther->GetOffset() == pbsl->GetOffset())
		{
			pbsl->GetVec()->And(pbslOther->GetVec());
			pbsl = m_bsllist.Next(pbsl);
		}
		else
		{
			pbslRemove = pbsl;
			pbsl = m_bsllist.Next(pbsl);
			
			m_bsllist.Remove(pbslRemove);
			GPOS_DELETE(pbslRemove);
		}
	}
	
	RecomputeSize();
}


//---------------------------------------------------------------------------
//	@function:
//		CBitSet::Difference
//
//	@doc:
//		Substract other set from this by iterating through other set and
//		explicit removal of elements;
//
//---------------------------------------------------------------------------
void
CBitSet::Difference
	(
	const CBitSet *pbs
	)
{
	if (IsDisjoint(pbs))
	{
		return;
	}
	
	CBitSetIter bsiter(*pbs);
	while (bsiter.FAdvance())
	{
		(void) ExchangeClear(bsiter.UlBit());
	}
}	


//---------------------------------------------------------------------------
//	@function:
//		CBitSet::FSubset
//
//	@doc:
//		Determine if given vector is subset
//
//---------------------------------------------------------------------------
BOOL
CBitSet::ContainsAll
	(
	const CBitSet *pbsOther
	)
	const
{
	// skip iterating if we can already tell by the sizes
	if (Size() < pbsOther->Size())
	{
		return false;
	}

	CBitSetLink *pbsl = NULL;
	CBitSetLink *pbslOther = NULL;

	// iterate through other's links and check for subsets
	for (
		pbslOther = pbsOther->m_bsllist.First();
		pbslOther != NULL;
		pbslOther = pbsOther->m_bsllist.Next(pbslOther)
		)
	{
		pbsl = FindLinkByOffset(pbslOther->GetOffset(), pbsl);
		
		if (NULL == pbsl ||
			pbsl->GetOffset() != pbslOther->GetOffset() ||
			!pbsl->GetVec()->ContainsAll(pbslOther->GetVec()))
		{
			return false;
		}
	}
	
	return true;
}


//---------------------------------------------------------------------------
//	@function:
//		CBitSet::Equals
//
//	@doc:
//		Determine if equal
//
//---------------------------------------------------------------------------
BOOL
CBitSet::Equals
	(
	const CBitSet *pbsOther
	)
	const
{
	// check pointer equality first
	if (this == pbsOther)
	{
		return true;
	}

	// skip iterating if we can already tell by the sizes
	if (Size() != pbsOther->Size())
	{
		return false;
	}

	CBitSetLink *pbsl = m_bsllist.First();
	CBitSetLink *pbslOther = pbsOther->m_bsllist.First();

	while(NULL != pbsl)
	{
		if (NULL == pbslOther || 
			pbsl->GetOffset() != pbslOther->GetOffset() ||
			!pbsl->GetVec()->Equals(pbslOther->GetVec()))
		{
			return false;
		}
				
		pbsl = m_bsllist.Next(pbsl);
		pbslOther = pbsOther->m_bsllist.Next(pbslOther);
	}
	
	// same length implies pbslOther must have reached end as well
	return pbslOther == NULL;
}



//---------------------------------------------------------------------------
//	@function:
//		CBitSet::FDisjoint
//
//	@doc:
//		Determine if disjoint
//
//---------------------------------------------------------------------------
BOOL
CBitSet::IsDisjoint
	(
	const CBitSet *pbsOther
	)
	const
{
	CBitSetLink *pbsl = NULL;
	CBitSetLink *pbslOther = NULL;

	// iterate through other's links an check if disjoint
	for (
		pbslOther = pbsOther->m_bsllist.First();
		pbslOther != NULL;
		pbslOther = pbsOther->m_bsllist.Next(pbslOther)
		)
	{
		pbsl = FindLinkByOffset(pbslOther->GetOffset(), pbsl);
		
		if (NULL != pbsl && 
			pbsl->GetOffset() == pbslOther->GetOffset() &&
			!pbsl->GetVec()->IsDisjoint(pbslOther->GetVec()))
		{
			return false;
		}
	}
	
	return true;
}


//---------------------------------------------------------------------------
//	@function:
//		CBitSet::HashValue
//
//	@doc:
//		Compute hash value for set
//
//---------------------------------------------------------------------------
ULONG
CBitSet::HashValue() const
{
	ULONG ulHash = 0;

	CBitSetLink *pbsl = m_bsllist.First();
	while (NULL != pbsl)
	{
		ulHash = gpos::UlCombineHashes(ulHash, pbsl->GetVec()->HashValue());
		pbsl = m_bsllist.Next(pbsl);
	}

	return ulHash;
}


//---------------------------------------------------------------------------
//	@function:
//		CBitSet::OsPrint
//
//	@doc:
//		Debug print function
//
//---------------------------------------------------------------------------
IOstream &
CBitSet::OsPrint
	(
	IOstream &os
	)
	const
{
	os << "{";

	ULONG ulElems = Size();
	CBitSetIter bsiter(*this);

	for (ULONG ul = 0; ul < ulElems; ul++)
	{
		(void) bsiter.FAdvance();
		os << bsiter.UlBit();

		if (ul < ulElems - 1)
		{
			os << ", ";
		}
	}
	
	os << "} " << "Hash:" << HashValue();
	
	return os;
}

#ifdef GPOS_DEBUG
void
CBitSet::DbgPrint() const
{
	CAutoTrace at(m_pmp);
	(void) this->OsPrint(at.Os());
}
#endif // GPOS_DEBUG
// EOF
