//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		COrderSpec.h
//
//	@doc:
//		Description of sort order; 
//		Can be used as required or derived property;
//---------------------------------------------------------------------------
#ifndef GPOPT_COrderSpec_H
#define GPOPT_COrderSpec_H

#include "gpos/base.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/CPropSpec.h"

#include "naucrates/md/IMDId.h"


namespace gpopt
{

	// type definition of corresponding dynamic pointer array
	class COrderSpec;
	typedef CDynamicPtrArray<COrderSpec, CleanupRelease> DrgPos;

	using namespace gpos;
	
	// fwd declaration
	class CColRefSet;

	//---------------------------------------------------------------------------
	//	@class:
	//		COrderSpec
	//
	//	@doc:
	//		Array of Order Expressions
	//
	//---------------------------------------------------------------------------
	class COrderSpec : public CPropSpec
	{
		public:
		
			enum ENullTreatment
			{
				EntAuto,  // default behavior, as implemented by operator
				
				EntFirst,
				EntLast,
			
				EntSentinel
			};
			
		private:
			//---------------------------------------------------------------------------
			//	@class:
			//		COrderExpression
			//
			//	@doc:
			//		Spec of sort order component consisting of 
			//
			//			1. sort operator's mdid
			//			2. column reference
			//			3. definition of NULL treatment
			//
			//---------------------------------------------------------------------------
			class COrderExpression
			{
			
				private:
				
					// MD id of sort operator
					gpmd::IMDId *m_mdid;
					
					// sort column
					const CColRef *m_pcr;
					
					// null treatment
					ENullTreatment m_ent;
					
					// private copy ctor
					COrderExpression(const COrderExpression &);
				
				public:
					
					// ctor
					COrderExpression(gpmd::IMDId *pmdid, const CColRef *pcr, ENullTreatment ent);
					
					// dtor
					virtual
					~COrderExpression();
					
					// accessor of sort operator midid
					gpmd::IMDId *GetMdIdSortOp() const
					{
						return m_mdid;
					}
					
					// accessor of sort column
					const CColRef *Pcr() const
					{
						return m_pcr;
					}

					// accessor of null treatment
					ENullTreatment Ent() const
					{
						return m_ent;
					}
					
					// check if order specs match
					BOOL FMatch(const COrderExpression *poe) const;
					
					// print
					IOstream &OsPrint(IOstream &os) const;

#ifdef GPOS_DEBUG
					// debug print for interactive debugging sessions only
					void DbgPrint() const;
#endif // GPOS_DEBUG
				
			}; // class COrderExpression

			// array of order expressions
			typedef CDynamicPtrArray<COrderExpression, CleanupDelete> DrgPoe;

		
			// memory pool
			IMemoryPool *m_memory_pool;
		
			// components of order spec
			DrgPoe *m_pdrgpoe;

			// private copy ctor
			COrderSpec(const COrderSpec &);
			
			// extract columns from order spec into the given column set
			void ExtractCols(CColRefSet *pcrs) const;

		public:
		
			// ctor
			explicit
			COrderSpec(IMemoryPool *memory_pool);
			
			// dtor
			virtual
			~COrderSpec();
			
			// number of sort expressions
			ULONG UlSortColumns() const
			{
				return m_pdrgpoe->Size();
			}
			
			// accessor of sort operator of the n-th component
			IMDId *GetMdIdSortOp(ULONG ul) const
			{				
				COrderExpression *poe = (*m_pdrgpoe)[ul]; 
				return poe->GetMdIdSortOp();
			}
			
			// accessor of sort column of the n-th component
			const CColRef *Pcr(ULONG ul) const
			{
				COrderExpression *poe = (*m_pdrgpoe)[ul]; 
				return poe->Pcr();
			}
			
			// accessor of null treatment of the n-th component
			ENullTreatment Ent(ULONG ul) const
			{
				COrderExpression *poe = (*m_pdrgpoe)[ul]; 
				return poe->Ent();
			}

			// check if order spec has no columns
			BOOL IsEmpty() const
			{
				return UlSortColumns() == 0;
			}
			
			// append new component
			void Append(gpmd::IMDId *pmdid, const CColRef *pcr, ENullTreatment ent);
			
			// extract colref set of order columns
			virtual
			CColRefSet *PcrsUsed(IMemoryPool *memory_pool) const;

			// property type
			virtual
			EPropSpecType Epst() const
			{
				return EpstOrder;
			}

			// check if order specs match
 			BOOL FMatch(const COrderSpec *pos) const;
			
			// check if order specs satisfies req'd spec
			BOOL FSatisfies(const COrderSpec *pos) const;
			
			// append enforcers to dynamic array for the given plan properties
			virtual
			void AppendEnforcers(IMemoryPool *memory_pool, CExpressionHandle &exprhdl, CReqdPropPlan *prpp, DrgPexpr *pdrgpexpr, CExpression *pexpr);

			// hash function
			virtual
			ULONG HashValue() const;

			// return a copy of the order spec with remapped columns
			virtual
			COrderSpec *PosCopyWithRemappedColumns(IMemoryPool *memory_pool, HMUlCr *phmulcr, BOOL fMustExist);

			// return a copy of the order spec after excluding the given columns
			virtual
			COrderSpec *PosExcludeColumns(IMemoryPool *memory_pool, CColRefSet *pcrs);

			// print
			virtual
			IOstream &OsPrint(IOstream &os) const;

			// matching function over order spec arrays
			static
			BOOL Equals(const DrgPos *pdrgposFirst, const DrgPos *pdrgposSecond);

			// combine hash values of a maximum number of entries
			static
			ULONG HashValue(const DrgPos *pdrgpos, ULONG ulMaxSize);

			// print array of order spec objects
			static
			IOstream &OsPrint(IOstream &os, const DrgPos *pdrgpos);

			// extract colref set of order columns used by elements of order spec array
			static
			CColRefSet *Pcrs(IMemoryPool *memory_pool, DrgPos *pdrgpos);

			// filter out array of order specs from order expressions using the passed columns
			static
			DrgPos *PdrgposExclude(IMemoryPool *memory_pool, DrgPos *pdrgpos, CColRefSet *pcrsToExclude);

						
	}; // class COrderSpec

}

#endif // !GPOPT_COrderSpec_H

// EOF
