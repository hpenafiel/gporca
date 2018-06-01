//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 Pivotal, Inc.
//
//	@filename:
//		COptimizerConfig.h
//
//	@doc:
//		Configurations used by the optimizer
//---------------------------------------------------------------------------

#ifndef GPOPT_COptimizerConfig_H
#define GPOPT_COptimizerConfig_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"
#include "gpos/common/CRefCount.h"

#include "gpopt/engine/CStatisticsConfig.h"
#include "gpopt/engine/CEnumeratorConfig.h"
#include "gpopt/engine/CCTEConfig.h"
#include "gpopt/engine/CHint.h"
#include "gpopt/base/CWindowOids.h"

namespace gpopt
{
	using namespace gpos;

	// forward decl
	class ICostModel;

	//---------------------------------------------------------------------------
	//	@class:
	//		COptimizerConfig
	//
	//	@doc:
	//		Configuration parameters of the optimizer including damping factors used
	//		during statistics derivation, CTE inlining cut-off threshold, Id of plan to
	//		be extracted (if plan enumeration is enabled), number of plans to be sampled
	//		from the space (if plan sampling is enabled) etc.
	//
	//		Most of these configurations can be changed from outside ORCA through
	//		GUCs. They are also included in optimizer’s minidumps under
	//		<dxl:OptimizerConfig> element
	//
	//---------------------------------------------------------------------------
	class COptimizerConfig : public CRefCount
	{

		private:
			
			// plan enumeration configuration
			CEnumeratorConfig *m_enumerator_cfg;

			// statistics configuration
			CStatisticsConfig *m_pstatsconf;

			// CTE configuration
			CCTEConfig *m_pcteconf;
			
			// cost model configuration
			ICostModel *m_cost_model;

			// hint configuration
			CHint *m_hint;

			// default window oids
			CWindowOids *m_pwindowoids;

		public:

			// ctor
			COptimizerConfig
				(
				CEnumeratorConfig *pec,
				CStatisticsConfig *pstatsconf,
				CCTEConfig *pcteconf,
				ICostModel *pcm,
				CHint *phint,
				CWindowOids *pdefoidsGPDB
				);

			// dtor
			virtual
			~COptimizerConfig();

			
			// plan enumeration configuration
			CEnumeratorConfig *GetEnumeratorCfg() const
			{
				return m_enumerator_cfg;
			}

			// statistics configuration
			CStatisticsConfig *Pstatsconf() const
			{
				return m_pstatsconf;
			}

			// CTE configuration
			CCTEConfig *Pcteconf() const
			{
				return m_pcteconf;
			}

			// cost model configuration
			ICostModel *GetCostModel() const
			{
				return m_cost_model;
			}
			
			// default window oids
			CWindowOids *Pwindowoids() const
			{
				return m_pwindowoids;
			}

			// hint configuration
			CHint *GetHint() const
			{
				return m_hint;
			}

			// generate default optimizer configurations
			static
			COptimizerConfig *PoconfDefault(IMemoryPool *memory_pool);
			
			// generate default optimizer configurations with the given cost model
			static
			COptimizerConfig *PoconfDefault(IMemoryPool *memory_pool, ICostModel *pcm);

			void Serialize(IMemoryPool *memory_pool, CXMLSerializer *xml_serializer, CBitSet *pbsTrace) const;

	}; // class COptimizerConfig

}

#endif // !GPOPT_COptimizerConfig_H

// EOF
