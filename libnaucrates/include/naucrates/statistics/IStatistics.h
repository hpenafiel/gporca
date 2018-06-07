//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 Pivotal, Inc.
//
//	@filename:
//		IStatistics.h
//
//	@doc:
//		Abstract statistics API
//---------------------------------------------------------------------------
#ifndef GPNAUCRATES_IStatistics_H
#define GPNAUCRATES_IStatistics_H

#include "gpos/base.h"
#include "gpos/common/CBitSet.h"
#include "gpos/common/CHashMapIter.h"

#include "naucrates/statistics/CStatsPred.h"
#include "naucrates/statistics/CStatsPredPoint.h"
#include "naucrates/statistics/CStatsPredJoin.h"
#include "naucrates/statistics/CHistogram.h"
#include "naucrates/md/CDXLStatsDerivedRelation.h"

#include "gpopt/base/CColRef.h"

namespace gpopt
{
	class CMDAccessor;
	class CReqdPropRelational;
	class CColRefSet;
}

namespace gpnaucrates
{
	using namespace gpos;
	using namespace gpmd;
	using namespace gpopt;

	// fwd declarations
	class IStatistics;

	// hash map from column id to a histogram
	typedef CHashMap<ULONG, CHistogram, gpos::HashValue<ULONG>, gpos::Equals<ULONG>,
					CleanupDelete<ULONG>, CleanupDelete<CHistogram> > HMUlHist;

	// iterator
	typedef CHashMapIter<ULONG, CHistogram, gpos::HashValue<ULONG>, gpos::Equals<ULONG>,
					CleanupDelete<ULONG>, CleanupDelete<CHistogram> > HMIterUlHist;

	// hash map from column ULONG to CDouble
	typedef CHashMap<ULONG, CDouble, gpos::HashValue<ULONG>, gpos::Equals<ULONG>,
					CleanupDelete<ULONG>, CleanupDelete<CDouble> > HMUlDouble;

	// iterator
	typedef CHashMapIter<ULONG, CDouble, gpos::HashValue<ULONG>, gpos::Equals<ULONG>,
					CleanupDelete<ULONG>, CleanupDelete<CDouble> > HMIterUlDouble;

	typedef CHashMap<ULONG, ULONG, gpos::HashValue<ULONG>, gpos::Equals<ULONG>,
					CleanupDelete<ULONG>, CleanupDelete<ULONG> > HMUlUl;

	// hash maps mapping INT -> ULONG
	typedef CHashMap<INT, ULONG, gpos::HashValue<INT>, gpos::Equals<INT>,
					CleanupDelete<INT>, CleanupDelete<ULONG> > HMIUl;

	//---------------------------------------------------------------------------
	//	@class:
	//		IStatistics
	//
	//	@doc:
	//		Abstract statistics API
	//
	//---------------------------------------------------------------------------
	class IStatistics: public CRefCount
	{
		private:

			// private copy ctor
			IStatistics(const IStatistics &);

			// private assignment operator
			IStatistics& operator=(IStatistics &);

		public:

			enum EStatsJoinType
					{
					EsjtInnerJoin,
					EsjtLeftOuterJoin,
					EsjtLeftSemiJoin,
					EsjtLeftAntiSemiJoin,
					EstiSentinel // should be the last in this enum
					};

			// ctor
			IStatistics()
			{}

			// dtor
			virtual
			~IStatistics()
			{}

			// how many rows
			virtual
			CDouble DRows() const = 0;

			// is statistics on an empty input
			virtual
			BOOL IsEmpty() const = 0;

			// statistics could be computed using predicates with external parameters (outer references),
			// this is the total number of external parameters' values
			virtual
			CDouble DRebinds() const = 0;

			// skew estimate for given column
			virtual
			CDouble DSkew(ULONG col_id) const = 0;

			// what is the width in bytes
			virtual
			CDouble DWidth() const = 0;

			// what is the width in bytes of set of column id's
			virtual
			CDouble DWidth(ULongPtrArray *pdrgpulColIds) const = 0;

			// what is the width in bytes of set of column references
			virtual
			CDouble DWidth(IMemoryPool *memory_pool, CColRefSet *pcrs) const = 0;

			// the risk of errors in cardinality estimation
			virtual
			ULONG UlStatsEstimationRisk() const = 0;

			// update the risk of errors in cardinality estimation
			virtual
			void SetStatsEstimationRisk(ULONG ulRisk) = 0;

			// look up the number of distinct values of a particular column
			virtual
			CDouble DNDV(const CColRef *pcr) = 0;

			virtual
			ULONG UlNumberOfPredicates() const = 0;

			// inner join with another stats structure
			virtual
			IStatistics *PstatsInnerJoin
						(
						IMemoryPool *memory_pool,
						const IStatistics *pistatsOther,
						DrgPstatspredjoin *pdrgpstatspredjoin
						)
						const = 0;

			// LOJ with another stats structure
			virtual
			IStatistics *PstatsLOJ
						(
						IMemoryPool *memory_pool,
						const IStatistics *pistatsOther,
						DrgPstatspredjoin *pdrgpstatspredjoin
						)
						const = 0;

			// semi join stats computation
			virtual
			IStatistics *PstatsLSJoin
						(
						IMemoryPool *memory_pool,
						const IStatistics *pstatsInner,
						DrgPstatspredjoin *pdrgpstatspredjoin
						)
						const = 0;

			// anti semi join
			virtual
			IStatistics *PstatsLASJoin
						(
						IMemoryPool *memory_pool,
						const IStatistics *pistatsOther,
						DrgPstatspredjoin *pdrgpstatspredjoin,
						BOOL fIgnoreLasjHistComputation
						)
						const = 0;

			// return required props associated with stats object
			virtual
			CReqdPropRelational *Prprel(IMemoryPool *memory_pool) const = 0;

			// append given stats to current object
			virtual
			void AppendStats(IMemoryPool *memory_pool, IStatistics *pstats) = 0;

			// set number of rebinds
			virtual
			void SetRebinds(CDouble dRebinds) = 0;

			// copy stats
			virtual
			IStatistics *PstatsCopy(IMemoryPool *memory_pool) const = 0;

			// return a copy of this stats object scaled by a given factor
			virtual
			IStatistics *PstatsScale(IMemoryPool *memory_pool, CDouble dFactor) const = 0;

			// copy stats with remapped column ids
			virtual
			IStatistics *PstatsCopyWithRemap(IMemoryPool *memory_pool, HMUlCr *phmulcr, BOOL fMustExist = true) const = 0;

			// return a set of column references we have stats for
			virtual
			CColRefSet *Pcrs(IMemoryPool *memory_pool) const = 0;

			// print function
			virtual
			IOstream &OsPrint(IOstream &os) const = 0;

			// generate the DXL representation of the statistics object
			virtual
			CDXLStatsDerivedRelation *GetDxlStatsDrvdRelation(IMemoryPool *memory_pool, CMDAccessor *md_accessor) const = 0;

			// is the join type either a left semi join or left anti-semi join
			static
			BOOL FSemiJoin
					(
					IStatistics::EStatsJoinType esjt
					)
			{
				return (IStatistics::EsjtLeftAntiSemiJoin == esjt) || (IStatistics::EsjtLeftSemiJoin == esjt);
			}
	}; // class IStatistics

	// shorthand for printing
	inline
	IOstream &operator << (IOstream &os, IStatistics &stat)
	{
		return stat.OsPrint(os);
	}
	// release istats
	inline void CleanupStats(IStatistics *pstats)
	{
		if (NULL != pstats)
		{
			(dynamic_cast<CRefCount*>(pstats))->Release();
		}
	}

	// dynamic array for derived stats
	typedef CDynamicPtrArray<IStatistics, CleanupStats> DrgPstat;
}

#endif // !GPNAUCRATES_IStatistics_H

// EOF
