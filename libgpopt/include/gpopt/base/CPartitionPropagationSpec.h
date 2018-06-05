//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPartitionPropagationSpec.h
//
//	@doc:
//		Partition Propagation spec in required properties
//---------------------------------------------------------------------------
#ifndef GPOPT_CPartitionPropagationSpec_H
#define GPOPT_CPartitionPropagationSpec_H

#include "gpos/base.h"

#include "gpopt/base/CPropSpec.h"
#include "gpopt/base/CPartFilterMap.h"
#include "gpopt/base/CPartIndexMap.h"
#include "gpos/common/CHashMap.h"
#include "gpos/common/CRefCount.h"


namespace gpopt
{		
	using namespace gpos;

	//---------------------------------------------------------------------------
	//	@class:
	//		CPartitionPropagationSpec
	//
	//	@doc:
	//		Partition Propagation specification
	//
	//---------------------------------------------------------------------------
	class CPartitionPropagationSpec : public CPropSpec
	{

		private:

			// unresolved partitions map
			CPartIndexMap *m_ppim;
			
			// filter expressions indexed by the part index id
			CPartFilterMap *m_ppfm;		

			// check if given part index id needs to be enforced on top of the given expression
			BOOL FRequiresPartitionPropagation
				(
				IMemoryPool *memory_pool, 
				CExpression *pexpr, 
				CExpressionHandle &exprhdl,
				ULONG part_idx_id
				)
				const;
			
			// private copy ctor
			CPartitionPropagationSpec(const CPartitionPropagationSpec&);

			// split the partition elimination predicates over the various levels
			// as well as the residual predicate
			void SplitPartPredicates
				(
				IMemoryPool *memory_pool,
				CExpression *pexprScalar,
				DrgDrgPcr *pdrgpdrgpcrKeys,
				HMUlExpr *phmulexprEqFilter,
				HMUlExpr *phmulexprFilter,
				CExpression **ppexprResidual
				);

			// return a residual filter given an array of predicates and a bitset
			// indicating which predicates have already been used
			CExpression *PexprResidualFilter
				(
				IMemoryPool *memory_pool,
				DrgPexpr *pdrgpexpr,
				CBitSet *pbsUsed
				);

			// return an array of predicates on the given partitioning key given
			// an array of predicates on all keys
			DrgPexpr *PdrgpexprPredicatesOnKey
				(
				IMemoryPool *memory_pool,
				DrgPexpr *pdrgpexpr,
				CColRef *pcr,
				CColRefSet *pcrsKeys,
				CBitSet **ppbs
				);

			// return a colrefset containing all the part keys
			CColRefSet *PcrsKeys(IMemoryPool *memory_pool, DrgDrgPcr *pdrgpdrgpcrKeys);

			// return the filter expression for the given Scan Id
			CExpression *PexprFilter(IMemoryPool *memory_pool, ULONG scan_id);

		public:

			// ctor
			CPartitionPropagationSpec(CPartIndexMap *ppim, CPartFilterMap *ppfm);

			// dtor
			virtual
			~CPartitionPropagationSpec();

			// accessor of part index map
			CPartIndexMap *Ppim() const
			{
				return m_ppim;
			}

			// accessor of part filter map
			CPartFilterMap *Ppfm() const
			{
				return m_ppfm;
			}

			// append enforcers to dynamic array for the given plan properties
			virtual
			void AppendEnforcers(IMemoryPool *memory_pool, CExpressionHandle &exprhdl, CReqdPropPlan *prpp, DrgPexpr *pdrgpexpr, CExpression *pexpr);

			// hash function
			virtual
			ULONG HashValue() const;

			// extract columns used by the rewindability spec
			virtual
			CColRefSet *PcrsUsed
				(
				IMemoryPool *memory_pool
				)
				const
			{
				// return an empty set
				return GPOS_NEW(memory_pool) CColRefSet(memory_pool);
			}

			// property type
			virtual
			EPropSpecType Epst() const
			{
				return EpstPartPropagation;
			}

			// equality function
			BOOL FMatch(const CPartitionPropagationSpec *ppps) const;
			
			// is partition propagation required
			BOOL FPartPropagationReqd() const
			{
				return m_ppim->FContainsUnresolvedZeroPropagators();
			}

			
			// print
			IOstream &OsPrint(IOstream &os) const;

	}; // class CPartitionPropagationSpec

}

#endif // !GPOPT_CPartitionPropagationSpec_H

// EOF
