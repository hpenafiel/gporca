//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CPhysicalPartitionSelector.h
//
//	@doc:
//		Physical partition selector operator used for property enforcement
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalPartitionSelector_H
#define GPOPT_CPhysicalPartitionSelector_H

#include "gpos/base.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CPhysical.h"


namespace gpopt
{

	//---------------------------------------------------------------------------
	//	@class:
	//		CPhysicalPartitionSelector
	//
	//	@doc:
	//		Physical partition selector operator used for property enforcement
	//
	//---------------------------------------------------------------------------
	class CPhysicalPartitionSelector : public CPhysical
	{
		protected:

			// Scan id
			ULONG m_scan_id;

			// mdid of partitioned table
			IMDId *m_mdid;

			// partition keys
			DrgDrgPcr *m_pdrgpdrgpcr;

			// part constraint map
			PartCnstrMap *m_ppartcnstrmap;

			// relation part constraint
			CPartConstraint *m_ppartcnstr;

			// expressions used in equality filters; for a filter of the form
			// pk1 = expr, we only store the expr
			HMUlExpr *m_phmulexprEqPredicates;

			// expressions used in general predicates; we store the whole predicate
			// in this case (e.g. pk1 > 50)
			HMUlExpr *m_phmulexprPredicates;

			// residual partition selection expression that cannot be split to
			// individual levels (e.g. pk1 < 5 OR pk2 = 6)
			CExpression *m_pexprResidual;

			// combined partition selection predicate
			CExpression *m_pexprCombinedPredicate;

			// ctor
			CPhysicalPartitionSelector(IMemoryPool *memory_pool, IMDId *pmdid, HMUlExpr *phmulexprEqPredicates);

			// return a single combined partition selection predicate
			CExpression *PexprCombinedPartPred(IMemoryPool *memory_pool) const;

			// check whether two expression maps match
			static
			BOOL FMatchExprMaps(HMUlExpr *phmulexprFst, HMUlExpr *phmulexprSnd);

		private:

			// private copy ctor
			CPhysicalPartitionSelector(const CPhysicalPartitionSelector &);

			// check whether part constraint maps match
			BOOL FMatchPartCnstr(PartCnstrMap *ppartcnstrmap) const;

			// check whether this operator has a partition selection filter
			BOOL FHasFilter() const;

			// check whether first part constraint map is contained in the second one
			static
			BOOL FSubsetPartCnstr(PartCnstrMap *ppartcnstrmapFst, PartCnstrMap *ppartcnstrmapSnd);

		public:

			// ctor
			CPhysicalPartitionSelector
				(
				IMemoryPool *memory_pool,
				ULONG scan_id,
				IMDId *pmdid,
				DrgDrgPcr *pdrgpdrgpcr,
				PartCnstrMap *ppartcnstrmap,
				CPartConstraint *ppartcnstr,
				HMUlExpr *phmulexprEqPredicates,
				HMUlExpr *phmulexprPredicates,
				CExpression *pexprResidual
				);

			// dtor
			virtual
			~CPhysicalPartitionSelector();

			// ident accessors
			virtual
			EOperatorId Eopid() const
			{
				return EopPhysicalPartitionSelector;
			}

			// operator name
			virtual
			const CHAR *SzId() const
			{
				return "CPhysicalPartitionSelector";
			}

			// scan id
			ULONG ScanId() const
			{
				return m_scan_id;
			}

			// partitioned table mdid
			IMDId *MDId() const
			{
				return m_mdid;
			}

			// partition keys
			DrgDrgPcr *Pdrgpdrgpcr() const
			{
				return m_pdrgpdrgpcr;
			}

			// number of partitioning levels
			virtual
			ULONG UlPartLevels() const;

			// return a combined printable version of the partition selection predicate
			CExpression *PexprCombinedPred() const
			{
				return m_pexprCombinedPredicate;
			}

			// return the equality filter expression for the given level
			CExpression *PexprEqFilter(ULONG ulPartLevel) const;

			// return the filter expression for the given level
			CExpression *PexprFilter(ULONG ulPartLevel) const;

			// return the partition selection predicate for the given level
			CExpression *PexprPartPred(IMemoryPool *memory_pool, ULONG ulPartLevel) const;

			// return the residual predicate
			CExpression *PexprResidualPred() const
			{
				return m_pexprResidual;
			}

			// match function
			virtual
			BOOL FMatch(COperator *pop) const;

			// hash function
			virtual
			ULONG HashValue() const;

			// sensitivity to order of inputs
			virtual
			BOOL FInputOrderSensitive() const
			{
				// operator has one child
				return false;
			}

			//-------------------------------------------------------------------------------------
			// Required Plan Properties
			//-------------------------------------------------------------------------------------

			// compute required output columns of the n-th child
			virtual
			CColRefSet *PcrsRequired
				(
				IMemoryPool *memory_pool,
				CExpressionHandle &exprhdl,
				CColRefSet *pcrsRequired,
				ULONG ulChildIndex,
				DrgPdp *pdrgpdpCtxt,
				ULONG ulOptReq
				);

			// compute required ctes of the n-th child
			virtual
			CCTEReq *PcteRequired
				(
				IMemoryPool *memory_pool,
				CExpressionHandle &exprhdl,
				CCTEReq *pcter,
				ULONG ulChildIndex,
				DrgPdp *pdrgpdpCtxt,
				ULONG ulOptReq
				)
				const;

			// compute required sort order of the n-th child
			virtual
			COrderSpec *PosRequired
				(
				IMemoryPool *memory_pool,
				CExpressionHandle &exprhdl,
				COrderSpec *posRequired,
				ULONG ulChildIndex,
				DrgPdp *pdrgpdpCtxt,
				ULONG ulOptReq
				)
				const;

			// compute required distribution of the n-th child
			virtual
			CDistributionSpec *PdsRequired
				(
				IMemoryPool *memory_pool,
				CExpressionHandle &exprhdl,
				CDistributionSpec *pdsRequired,
				ULONG ulChildIndex,
				DrgPdp *pdrgpdpCtxt,
				ULONG ulOptReq
				)
				const;

			// compute required partition propagation of the n-th child
			virtual
			CPartitionPropagationSpec *PppsRequired
				(
				IMemoryPool *memory_pool,
				CExpressionHandle &exprhdl,
				CPartitionPropagationSpec *pppsRequired,
				ULONG ulChildIndex,
				DrgPdp *pdrgpdpCtxt,
				ULONG ulOptReq
				);

			// compute required rewindability of the n-th child
			virtual
			CRewindabilitySpec *PrsRequired
				(
				IMemoryPool *memory_pool,
				CExpressionHandle &exprhdl,
				CRewindabilitySpec *prsRequired,
				ULONG ulChildIndex,
				DrgPdp *pdrgpdpCtxt,
				ULONG ulOptReq
				)
				const;

			// check if required columns are included in output columns
			virtual
			BOOL FProvidesReqdCols(CExpressionHandle &exprhdl, CColRefSet *pcrsRequired, ULONG ulOptReq) const;

			//-------------------------------------------------------------------------------------
			// Derived Plan Properties
			//-------------------------------------------------------------------------------------

			// derive sort order
			virtual
			COrderSpec *PosDerive(IMemoryPool *memory_pool, CExpressionHandle &exprhdl) const;

			// derive distribution
			virtual
			CDistributionSpec *PdsDerive(IMemoryPool *memory_pool, CExpressionHandle &exprhdl) const;

			// derive rewindability
			virtual
			CRewindabilitySpec *PrsDerive(IMemoryPool *memory_pool, CExpressionHandle &exprhdl) const;

			// derive partition index map
			virtual
			CPartIndexMap *PpimDerive(IMemoryPool *memory_pool, CExpressionHandle &exprhdl, CDrvdPropCtxt *pdpctxt) const;

			// derive partition filter map
			virtual
			CPartFilterMap *PpfmDerive(IMemoryPool *memory_pool, CExpressionHandle &exprhdl) const;

			//-------------------------------------------------------------------------------------
			// Enforced Properties
			//-------------------------------------------------------------------------------------

			// return distribution property enforcing type for this operator
			virtual
			CEnfdProp::EPropEnforcingType EpetDistribution
				(
				CExpressionHandle &exprhdl,
				const CEnfdDistribution *ped
				)
				const;

			// return rewindability property enforcing type for this operator
			virtual
			CEnfdProp::EPropEnforcingType EpetRewindability
				(
				CExpressionHandle &exprhdl,
				const CEnfdRewindability *per
				)
				const;

			// return order property enforcing type for this operator
			virtual
			CEnfdProp::EPropEnforcingType EpetOrder
				(
				CExpressionHandle &exprhdl,
				const CEnfdOrder *peo
				)
				const;

			// return true if operator passes through stats obtained from children,
			// this is used when computing stats during costing
			virtual
			BOOL FPassThruStats() const
			{
				return true;
			}

			//-------------------------------------------------------------------------------------
			//-------------------------------------------------------------------------------------
			//-------------------------------------------------------------------------------------

			// conversion function
			static
			CPhysicalPartitionSelector *PopConvert
				(
				COperator *pop
				)
			{
				GPOS_ASSERT(NULL != pop);
				GPOS_ASSERT(EopPhysicalPartitionSelector == pop->Eopid() ||
							EopPhysicalPartitionSelectorDML == pop->Eopid());

				return dynamic_cast<CPhysicalPartitionSelector*>(pop);
			}

			// debug print
			virtual
			IOstream &OsPrint(IOstream &os) const;

	}; // class CPhysicalPartitionSelector

}

#endif // !GPOPT_CPhysicalPartitionSelector_H

// EOF
