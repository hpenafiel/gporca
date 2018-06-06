//	Greenplum Database
//	Copyright (C) 2016 Pivotal Software, Inc.

#include "gpopt/operators/CHashedDistributions.h"

using namespace gpopt;
CHashedDistributions::CHashedDistributions
		(
		IMemoryPool *memory_pool,
		DrgPcr *pdrgpcrOutput,
		DrgDrgPcr *pdrgpdrgpcrInput
		)
		:
		DrgPds(memory_pool)
{
	const ULONG ulCols = pdrgpcrOutput->Size();
	const ULONG arity = pdrgpdrgpcrInput->Size();
	for (ULONG ulChild = 0; ulChild < arity; ulChild++)
	{
		DrgPcr *pdrgpcr = (*pdrgpdrgpcrInput)[ulChild];
		DrgPexpr *pdrgpexpr = GPOS_NEW(memory_pool) DrgPexpr(memory_pool);
		for (ULONG ulCol = 0; ulCol < ulCols; ulCol++)
		{
			CColRef *pcr = (*pdrgpcr)[ulCol];
			CExpression *pexpr = CUtils::PexprScalarIdent(memory_pool, pcr);
			pdrgpexpr->Append(pexpr);
		}

		// create a hashed distribution on input columns of the current child
		BOOL fNullsColocated = true;
		CDistributionSpec *pdshashed = GPOS_NEW(memory_pool) CDistributionSpecHashed(pdrgpexpr, fNullsColocated);
		Append(pdshashed);
	}
}
