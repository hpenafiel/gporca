//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CPhysicalTableScan.cpp
//
//	@doc:
//		Implementation of basic table scan operator
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/base/CDistributionSpec.h"
#include "gpopt/base/CDistributionSpecHashed.h"
#include "gpopt/base/CDistributionSpecRandom.h"
#include "gpopt/base/CDistributionSpecSingleton.h"

#include "gpopt/operators/CPhysicalTableScan.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/metadata/CName.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalTableScan::CPhysicalTableScan
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CPhysicalTableScan::CPhysicalTableScan
	(
	IMemoryPool *pmp,
	const CName *pnameAlias,
	CTableDescriptor *ptabdesc,
	DrgPcr *pdrgpcrOutput
	)
	:
	CPhysicalScan(pmp, pnameAlias, ptabdesc, pdrgpcrOutput)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalTableScan::HashValue
//
//	@doc:
//		Combine pointer for table descriptor and Eop
//
//---------------------------------------------------------------------------
ULONG
CPhysicalTableScan::HashValue() const
{
	ULONG ulHash = gpos::CombineHashes(COperator::HashValue(), m_ptabdesc->Pmdid()->HashValue());
	ulHash = gpos::CombineHashes(ulHash, CUtils::UlHashColArray(m_pdrgpcrOutput));

	return ulHash;
}

	
//---------------------------------------------------------------------------
//	@function:
//		CPhysicalTableScan::FMatch
//
//	@doc:
//		match operator
//
//---------------------------------------------------------------------------
BOOL
CPhysicalTableScan::FMatch
	(
	COperator *pop
	)
	const
{
	if (Eopid() != pop->Eopid())
	{
		return false;
	}

	CPhysicalTableScan *popTableScan = CPhysicalTableScan::PopConvert(pop);
	return m_ptabdesc->Pmdid()->Equals(popTableScan->Ptabdesc()->Pmdid()) &&
			m_pdrgpcrOutput->Equals(popTableScan->PdrgpcrOutput());
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalTableScan::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CPhysicalTableScan::OsPrint
	(
	IOstream &os
	)
	const
{
	os << SzId() << " ";
	
	// alias of table as referenced in the query
	m_pnameAlias->OsPrint(os);

	// actual name of table in catalog and columns
	os << " (";
	m_ptabdesc->Name().OsPrint(os);
	os << ")";
	
	return os;
}



// EOF

