//	Greenplum Database
//	Copyright (C) 2016 Pivotal Software, Inc.

#include "gpopt/operators/CStrictHashedDistributions.h"
#include "gpopt/base/CDistributionSpecStrictRandom.h"

using namespace gpopt;

CStrictHashedDistributions::CStrictHashedDistributions
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
			if (pcr->Pmdtype()->FRedistributable())
			{
				CExpression *pexpr = CUtils::PexprScalarIdent(memory_pool, pcr);
				pdrgpexpr->Append(pexpr);
			}
		}

		CDistributionSpec *pdshashed;
		ULONG ulColumnsToRedistribute = pdrgpexpr->Size();
		if (0 < ulColumnsToRedistribute)
		{
			// create a hashed distribution on input columns of the current child
			BOOL fNullsColocated = true;
			pdshashed = GPOS_NEW(memory_pool) CDistributionSpecStrictHashed(pdrgpexpr, fNullsColocated);
		}
		else
		{
			// None of the input columns are redistributable, but we want to
			// parallelize the relations we are concatenating, so we generate
			// a random redistribution.
			// When given a plan containing a "hash" redistribution on _no_ columns,
			// Some databases actually execute it as if it's a random redistribution.
			// We should not generate such a plan, for clarity and our own sanity

			pdshashed = GPOS_NEW(memory_pool) CDistributionSpecStrictRandom();
			pdrgpexpr->Release();
		}
		Append(pdshashed);
	}
}
