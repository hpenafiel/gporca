//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CPartKeys.cpp
//
//	@doc:
//		Implementation of partitioning keys
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpopt/base/CPartKeys.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CUtils.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CPartKeys::CPartKeys
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPartKeys::CPartKeys
	(
	DrgDrgPcr *pdrgpdrgpcr
	)
	:
	m_pdrgpdrgpcr(pdrgpdrgpcr)
{
	GPOS_ASSERT(NULL != pdrgpdrgpcr);
	m_ulLevels = pdrgpdrgpcr->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CPartKeys::~CPartKeys
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CPartKeys::~CPartKeys()
{
	m_pdrgpdrgpcr->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CPartKeys::PcrKey
//
//	@doc:
//		Return key at a given level
//
//---------------------------------------------------------------------------
CColRef *
CPartKeys::PcrKey
	(
	ULONG ulLevel
	)
	const
{
	GPOS_ASSERT(ulLevel < m_ulLevels);
	DrgPcr *pdrgpcr = (*m_pdrgpdrgpcr)[ulLevel];
	return (*pdrgpcr)[0];
}

//---------------------------------------------------------------------------
//	@function:
//		CPartKeys::FOverlap
//
//	@doc:
//		Check whether the key columns overlap the given column
//
//---------------------------------------------------------------------------
BOOL
CPartKeys::FOverlap
	(
	CColRefSet *pcrs
	)
	const
{
	for (ULONG ul = 0; ul < m_ulLevels; ul++)
	{
		CColRef *pcr = PcrKey(ul);
		if (pcrs->FMember(pcr))
		{
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartKeys::PpartkeysCopy
//
//	@doc:
//		Copy part key into the given memory pool
//
//---------------------------------------------------------------------------
CPartKeys *
CPartKeys::PpartkeysCopy
	(
	IMemoryPool *memory_pool
	)
{
	DrgDrgPcr *pdrgpdrgpcrCopy = GPOS_NEW(memory_pool) DrgDrgPcr(memory_pool);

	const ULONG length = m_pdrgpdrgpcr->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		DrgPcr *pdrgpcr = (*m_pdrgpdrgpcr)[ul];
		DrgPcr *pdrgpcrCopy = GPOS_NEW(memory_pool) DrgPcr(memory_pool);
		const ULONG ulCols = pdrgpcr->Size();
		for (ULONG ulCol = 0; ulCol < ulCols; ulCol++)
		{
			pdrgpcrCopy->Append((*pdrgpcr)[ulCol]);
		}
		pdrgpdrgpcrCopy->Append(pdrgpcrCopy);
	}

	return GPOS_NEW(memory_pool) CPartKeys(pdrgpdrgpcrCopy);
}

//---------------------------------------------------------------------------
//	@function:
//		CPartKeys::PdrgppartkeysCopy
//
//	@doc:
//		Copy array of part keys into given memory pool
//
//---------------------------------------------------------------------------
DrgPpartkeys *
CPartKeys::PdrgppartkeysCopy
	(
	IMemoryPool *memory_pool,
	const DrgPpartkeys *pdrgppartkeys
	)
{
	GPOS_ASSERT(NULL != pdrgppartkeys);

	DrgPpartkeys *pdrgppartkeysCopy = GPOS_NEW(memory_pool) DrgPpartkeys(memory_pool);
	const ULONG length = pdrgppartkeys->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		pdrgppartkeysCopy->Append((*pdrgppartkeys)[ul]->PpartkeysCopy(memory_pool));
	}
	return pdrgppartkeysCopy;
}


//---------------------------------------------------------------------------
//	@function:
//		CPartKeys::PpartkeysRemap
//
//	@doc:
//		Create a new PartKeys object from the current one by remapping the
//		keys using the given hashmap
//
//---------------------------------------------------------------------------
CPartKeys *
CPartKeys::PpartkeysRemap
	(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr
	)
	const
{
	GPOS_ASSERT(NULL != phmulcr);
	DrgDrgPcr *pdrgpdrgpcr = GPOS_NEW(memory_pool) DrgDrgPcr(memory_pool);

	for (ULONG ul = 0; ul < m_ulLevels; ul++)
	{
		CColRef *pcr = CUtils::PcrRemap(PcrKey(ul), phmulcr, false /*fMustExist*/);

		DrgPcr *pdrgpcr = GPOS_NEW(memory_pool) DrgPcr(memory_pool);
		pdrgpcr->Append(pcr);

		pdrgpdrgpcr->Append(pdrgpcr);
	}

	return GPOS_NEW(memory_pool) CPartKeys(pdrgpdrgpcr);
}

//---------------------------------------------------------------------------
//	@function:
//		CPartKeys::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CPartKeys::OsPrint
	(
	IOstream &os
	)
	const
{
	os << "(";
	for (ULONG ul = 0; ul < m_ulLevels; ul++)
	{
		CColRef *pcr = PcrKey(ul);
		os << *pcr;

		// separator
		os << (ul == m_ulLevels - 1 ? "" : ", ");
	}

	os << ")";

	return os;
}

#ifdef GPOS_DEBUG
void
CPartKeys::DbgPrint() const
{

	IMemoryPool *memory_pool = COptCtxt::PoctxtFromTLS()->Pmp();
	CAutoTrace at(memory_pool);
	(void) this->OsPrint(at.Os());
}
#endif // GPOS_DEBUG
// EOF
