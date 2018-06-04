//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CStatsPredPoint.cpp
//
//	@doc:
//		Implementation of statistics filter that compares a column to a constant
//---------------------------------------------------------------------------

#include "naucrates/statistics/CStatsPredPoint.h"
#include "naucrates/md/CMDIdGPDB.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/CColRefTable.h"

using namespace gpnaucrates;
using namespace gpopt;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CStatsPredPoint::CStatisticsFilterPoint
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CStatsPredPoint::CStatsPredPoint
	(
	ULONG col_id,
	CStatsPred::EStatsCmpType escmpt,
	CPoint *ppoint
	)
	:
	CStatsPred(col_id),
	m_escmpt(escmpt),
	m_ppoint(ppoint)
{
	GPOS_ASSERT(NULL != ppoint);
}

//---------------------------------------------------------------------------
//	@function:
//		CStatsPredPoint::CStatisticsFilterPoint
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CStatsPredPoint::CStatsPredPoint
	(
	IMemoryPool *memory_pool,
	const CColRef *pcr,
	CStatsPred::EStatsCmpType escmpt,
	IDatum *pdatum
	)
	:
	CStatsPred(ULONG_MAX),
	m_escmpt(escmpt),
	m_ppoint(NULL)
{
	GPOS_ASSERT(NULL != pcr);
	GPOS_ASSERT(NULL != pdatum);

	m_ulColId = pcr->UlId();
	IDatum *pdatumPadded = PdatumPreprocess(memory_pool, pcr, pdatum);

	m_ppoint = GPOS_NEW(memory_pool) CPoint(pdatumPadded);
}

//---------------------------------------------------------------------------
//		CStatsPredPoint::PdatumPreprocess
//
//	@doc:
//		Add padding to datums when needed
//---------------------------------------------------------------------------
IDatum *
CStatsPredPoint::PdatumPreprocess
	(
	IMemoryPool *memory_pool,
	const CColRef *pcr,
	IDatum *pdatum
	)
{
	GPOS_ASSERT(NULL != pcr);
	GPOS_ASSERT(NULL != pdatum);

	if (!pdatum->FNeedsPadding() || CColRef::EcrtTable != pcr->Ecrt() || pdatum->IsNull())
	{
		// we do not pad datum for comparison against computed columns
		pdatum->AddRef();
		return pdatum;
	}

	const CColRefTable *pcrTable = CColRefTable::PcrConvert(const_cast<CColRef*>(pcr));

	return pdatum->PdatumPadded(memory_pool, pcrTable->Width());
}

// EOF

