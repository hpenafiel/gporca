//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CPartInfo.cpp
//
//	@doc:
//		Implementation of derived partition information at the logical level
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpopt/base/CPartInfo.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/metadata/CPartConstraint.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::CPartInfoEntry::CPartInfoEntry
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPartInfo::CPartInfoEntry::CPartInfoEntry
	(
	ULONG ulScanId,
	IMDId *pmdid,
	DrgPpartkeys *pdrgppartkeys,
	CPartConstraint *ppartcnstrRel
	)
	:
	m_ulScanId(ulScanId),
	m_pmdid(pmdid),
	m_pdrgppartkeys(pdrgppartkeys),
	m_ppartcnstrRel(ppartcnstrRel)
{
	GPOS_ASSERT(pmdid->IsValid());
	GPOS_ASSERT(pdrgppartkeys != NULL);
	GPOS_ASSERT(0 < pdrgppartkeys->Size());
	GPOS_ASSERT(NULL != ppartcnstrRel);
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::CPartInfoEntry::~CPartInfoEntry
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CPartInfo::CPartInfoEntry::~CPartInfoEntry()
{
	m_pmdid->Release();
	m_pdrgppartkeys->Release();
	m_ppartcnstrRel->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::CPartInfoEntry::PpartinfoentryAddRemappedKeys
//
//	@doc:
//		Create a copy of the current object, and add a set of remapped
//		part keys to this entry, using the existing keys and the given hashmap
//
//---------------------------------------------------------------------------
CPartInfo::CPartInfoEntry *
CPartInfo::CPartInfoEntry::PpartinfoentryAddRemappedKeys
	(
	IMemoryPool *memory_pool,
	CColRefSet *pcrs,
	HMUlCr *phmulcr
	)
{
	GPOS_ASSERT(NULL != pcrs);
	GPOS_ASSERT(NULL != phmulcr);

    DrgPpartkeys *pdrgppartkeys = CPartKeys::PdrgppartkeysCopy(memory_pool, m_pdrgppartkeys);

	const ULONG ulSize = m_pdrgppartkeys->Size();
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CPartKeys *ppartkeys = (*m_pdrgppartkeys)[ul];

		if (ppartkeys->FOverlap(pcrs))
		{
			pdrgppartkeys->Append(ppartkeys->PpartkeysRemap(memory_pool, phmulcr));
			break;
		}
	}

	m_pmdid->AddRef();
	CPartConstraint *ppartcnstrRel = m_ppartcnstrRel->PpartcnstrCopyWithRemappedColumns(memory_pool, phmulcr, false /*fMustExist*/);

	return GPOS_NEW(memory_pool) CPartInfoEntry(m_ulScanId, m_pmdid, pdrgppartkeys, ppartcnstrRel);
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::CPartInfoEntry::OsPrint
//
//	@doc:
//		Print functions
//
//---------------------------------------------------------------------------
IOstream &
CPartInfo::CPartInfoEntry::OsPrint
	(
	IOstream &os
	)
	const
{
	os << m_ulScanId;

	return os;
}

#ifdef GPOS_DEBUG
void
CPartInfo::CPartInfoEntry::DbgPrint() const
{
	IMemoryPool *memory_pool = COptCtxt::PoctxtFromTLS()->Pmp();
	CAutoTrace at(memory_pool);
	(void) this->OsPrint(at.Os());
}
#endif // GPOS_DEBUG


//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::CPartInfoEntry::PpartinfoentryCopy
//
//	@doc:
//		Copy part info entry into given memory pool
//
//---------------------------------------------------------------------------
CPartInfo::CPartInfoEntry *
CPartInfo::CPartInfoEntry::PpartinfoentryCopy
	(
	IMemoryPool *memory_pool
	)
{
	IMDId *pmdid = MDId();
	pmdid->AddRef();

	// copy part keys
	DrgPpartkeys *pdrgppartkeysCopy = CPartKeys::PdrgppartkeysCopy(memory_pool, Pdrgppartkeys());

	// copy part constraint using empty remapping to get exact copy
	HMUlCr *phmulcr = GPOS_NEW(memory_pool) HMUlCr(memory_pool);
	CPartConstraint *ppartcnstrRel = PpartcnstrRel()->PpartcnstrCopyWithRemappedColumns(memory_pool, phmulcr, false /*fMustExist*/);
	phmulcr->Release();

	return GPOS_NEW(memory_pool) CPartInfoEntry(UlScanId(), pmdid, pdrgppartkeysCopy, ppartcnstrRel);
}


//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::CPartInfo
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPartInfo::CPartInfo
	(
	DrgPpartentries *pdrgppartentries
	)
	:
	m_pdrgppartentries(pdrgppartentries)
{
	GPOS_ASSERT(NULL != pdrgppartentries);
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::CPartInfo
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPartInfo::CPartInfo
	(
	IMemoryPool *memory_pool
	)
{
	m_pdrgppartentries = GPOS_NEW(memory_pool) DrgPpartentries(memory_pool);
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::~CPartInfo
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CPartInfo::~CPartInfo()
{
	CRefCount::SafeRelease(m_pdrgppartentries);
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::AddPartConsumer
//
//	@doc:
//		Add part table consumer
//
//---------------------------------------------------------------------------
void
CPartInfo::AddPartConsumer
	(
	IMemoryPool *memory_pool,
	ULONG ulScanId,
	IMDId *pmdid,
	DrgDrgPcr *pdrgpdrgpcrPart,
	CPartConstraint *ppartcnstrRel
	)
{
	DrgPpartkeys *pdrgppartkeys = GPOS_NEW(memory_pool) DrgPpartkeys(memory_pool);
	pdrgppartkeys->Append(GPOS_NEW(memory_pool) CPartKeys(pdrgpdrgpcrPart));

	m_pdrgppartentries->Append(GPOS_NEW(memory_pool) CPartInfoEntry(ulScanId, pmdid, pdrgppartkeys, ppartcnstrRel));
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::FContainsScanId
//
//	@doc:
//		Check if part info contains given scan id
//
//---------------------------------------------------------------------------
BOOL
CPartInfo::FContainsScanId
	(
	ULONG ulScanId
	)
	const
{
	const ULONG ulSize = m_pdrgppartentries->Size();

	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CPartInfoEntry *ppartinfoentry = (*m_pdrgppartentries)[ul];
		if (ulScanId == ppartinfoentry->UlScanId())
		{
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::UlScanId
//
//	@doc:
//		Return scan id of the entry at the given position
//
//---------------------------------------------------------------------------
ULONG
CPartInfo::UlScanId
	(
	ULONG ulPos
	)
	const
{
	return (*m_pdrgppartentries)[ulPos]->UlScanId();
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::PmdidRel
//
//	@doc:
//		Return relation mdid of the entry at the given position
//
//---------------------------------------------------------------------------
IMDId *
CPartInfo::PmdidRel
	(
	ULONG ulPos
	)
	const
{
	return (*m_pdrgppartentries)[ulPos]->MDId();
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::Pdrgppartkeys
//
//	@doc:
//		Return part keys of the entry at the given position
//
//---------------------------------------------------------------------------
DrgPpartkeys *
CPartInfo::Pdrgppartkeys
	(
	ULONG ulPos
	)
	const
{
	return (*m_pdrgppartentries)[ulPos]->Pdrgppartkeys();
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::Ppartcnstr
//
//	@doc:
//		Return part constraint of the entry at the given position
//
//---------------------------------------------------------------------------
CPartConstraint *
CPartInfo::Ppartcnstr
	(
	ULONG ulPos
	)
	const
{
	return (*m_pdrgppartentries)[ulPos]->PpartcnstrRel();
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::PdrgppartkeysByScanId
//
//	@doc:
//		Return part keys of the entry with the given scan id
//
//---------------------------------------------------------------------------
DrgPpartkeys *
CPartInfo::PdrgppartkeysByScanId
	(
	ULONG ulScanId
	)
	const
{
	const ULONG ulSize = m_pdrgppartentries->Size();

	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CPartInfoEntry *ppartinfoentry = (*m_pdrgppartentries)[ul];
		if (ulScanId == ppartinfoentry->UlScanId())
		{
			return ppartinfoentry->Pdrgppartkeys();
		}
	}

	return NULL;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::PpartinfoWithRemappedKeys
//
//	@doc:
//		Return a new part info object with an additional set of remapped keys
//
//---------------------------------------------------------------------------
CPartInfo *
CPartInfo::PpartinfoWithRemappedKeys
	(
	IMemoryPool *memory_pool,
	DrgPcr *pdrgpcrSrc,
	DrgPcr *pdrgpcrDest
	)
	const
{
	GPOS_ASSERT(NULL != pdrgpcrSrc);
	GPOS_ASSERT(NULL != pdrgpcrDest);

	CColRefSet *pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool, pdrgpcrSrc);
	HMUlCr *phmulcr = CUtils::PhmulcrMapping(memory_pool, pdrgpcrSrc, pdrgpcrDest);

	DrgPpartentries *pdrgppartentries = GPOS_NEW(memory_pool) DrgPpartentries(memory_pool);

	const ULONG ulSize = m_pdrgppartentries->Size();
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CPartInfoEntry *ppartinfoentry = (*m_pdrgppartentries)[ul];

		// if this entry has keys that overlap with the source columns then
		// add another set of keys to it using the destination columns
		CPartInfoEntry *ppartinfoentryNew = ppartinfoentry->PpartinfoentryAddRemappedKeys(memory_pool, pcrs, phmulcr);
		pdrgppartentries->Append(ppartinfoentryNew);
	}

	pcrs->Release();
	phmulcr->Release();

	return GPOS_NEW(memory_pool) CPartInfo(pdrgppartentries);
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::PpartinfoCombine
//
//	@doc:
//		Combine two part info objects
//
//---------------------------------------------------------------------------
CPartInfo *
CPartInfo::PpartinfoCombine
	(
	IMemoryPool *memory_pool,
	CPartInfo *ppartinfoFst,
	CPartInfo *ppartinfoSnd
	)
{
	GPOS_ASSERT(NULL != ppartinfoFst);
	GPOS_ASSERT(NULL != ppartinfoSnd);

	DrgPpartentries *pdrgppartentries = GPOS_NEW(memory_pool) DrgPpartentries(memory_pool);

	// copy part entries from first part info object
	CUtils::AddRefAppend(pdrgppartentries, ppartinfoFst->m_pdrgppartentries);

	// copy part entries from second part info object, except those which already exist
	const ULONG ulLen = ppartinfoSnd->m_pdrgppartentries->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		CPartInfoEntry *ppartinfoentry = (*(ppartinfoSnd->m_pdrgppartentries))[ul];
		DrgPpartkeys *pdrgppartkeys = ppartinfoFst->PdrgppartkeysByScanId(ppartinfoentry->UlScanId());

		if (NULL != pdrgppartkeys)
		{
			// there is already an entry with the same scan id; need to add to it
			// the keys from the current entry
			DrgPpartkeys *pdrgppartkeysCopy = CPartKeys::PdrgppartkeysCopy(memory_pool, ppartinfoentry->Pdrgppartkeys());
			CUtils::AddRefAppend(pdrgppartkeys, pdrgppartkeysCopy);
			pdrgppartkeysCopy->Release();
		}
		else
		{
			CPartInfoEntry *ppartinfoentryCopy = ppartinfoentry->PpartinfoentryCopy(memory_pool);
			pdrgppartentries->Append(ppartinfoentryCopy);
		}
	}

	return GPOS_NEW(memory_pool) CPartInfo(pdrgppartentries);
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CPartInfo::OsPrint
	(
	IOstream &os
	)
	const
{
	const ULONG length = m_pdrgppartentries->Size();
	os << "Part Consumers: ";
	for (ULONG ul = 0; ul < length; ul++)
	{
		CPartInfoEntry *ppartinfoentry = (*m_pdrgppartentries)[ul];
		ppartinfoentry->OsPrint(os);

		// separator
		os << (ul == length - 1 ? "" : ", ");
	}

	os << ", Part Keys: ";
	for (ULONG ulCons = 0; ulCons < length; ulCons++)
	{
		DrgPpartkeys *pdrgppartkeys = Pdrgppartkeys(ulCons);
		os << "(";
		const ULONG ulPartKeys = pdrgppartkeys->Size();;
		for (ULONG ulPartKey = 0; ulPartKey < ulPartKeys; ulPartKey++)
		{
			os << *(*pdrgppartkeys)[ulPartKey];
			os << (ulPartKey == ulPartKeys - 1 ? "" : ", ");
		}
		os << ")";
		os << (ulCons == length - 1 ? "" : ", ");
	}

	return os;
}

#ifdef GPOS_DEBUG
void
CPartInfo::DbgPrint() const
{
	IMemoryPool *memory_pool = COptCtxt::PoctxtFromTLS()->Pmp();
	CAutoTrace at(memory_pool);
	(void) this->OsPrint(at.Os());
}
#endif // GPOS_DEBUG


// EOF
