//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 Pivotal, Inc.
//
//	@filename:
//		COptimizerConfig.cpp
//
//	@doc:
//		Implementation of configuration used by the optimizer
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpos/io/COstreamFile.h"

#include "gpopt/cost/ICostModel.h"
#include "gpopt/optimizer/COptimizerConfig.h"
#include "naucrates/dxl/CCostModelConfigSerializer.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "gpos/common/CBitSetIter.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		COptimizerConfig::COptimizerConfig
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
COptimizerConfig::COptimizerConfig
	(
	CEnumeratorConfig *pec,
	CStatisticsConfig *pstatsconf,
	CCTEConfig *pcteconf,
	ICostModel *cost_model,
	CHint *phint,
	CWindowOids *pwindowoids
	)
	:
	m_enumerator_cfg(pec),
	m_pstatsconf(pstatsconf),
	m_pcteconf(pcteconf),
	m_cost_model(cost_model),
	m_hint(phint),
	m_window_oids(pwindowoids)
{
	GPOS_ASSERT(NULL != pec);
	GPOS_ASSERT(NULL != pstatsconf);
	GPOS_ASSERT(NULL != pcteconf);
	GPOS_ASSERT(NULL != m_cost_model);
	GPOS_ASSERT(NULL != phint);
	GPOS_ASSERT(NULL != m_window_oids);
}

//---------------------------------------------------------------------------
//	@function:
//		COptimizerConfig::~COptimizerConfig
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
COptimizerConfig::~COptimizerConfig()
{
	m_enumerator_cfg->Release();
	m_pstatsconf->Release();
	m_pcteconf->Release();
	m_cost_model->Release();
	m_hint->Release();
	m_window_oids->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		COptimizerConfig::PocDefault
//
//	@doc:
//		Default optimizer configuration
//
//---------------------------------------------------------------------------
COptimizerConfig *
COptimizerConfig::PoconfDefault
	(
	IMemoryPool *memory_pool
	)
{
	return GPOS_NEW(memory_pool) COptimizerConfig
						(
						GPOS_NEW(memory_pool) CEnumeratorConfig(memory_pool, 0 /*plan_id*/, 0 /*ullSamples*/),
						CStatisticsConfig::PstatsconfDefault(memory_pool),
						CCTEConfig::PcteconfDefault(memory_pool),
						ICostModel::PcmDefault(memory_pool),
						CHint::PhintDefault(memory_pool),
						CWindowOids::GetWindowOids(memory_pool)
						);
}

//---------------------------------------------------------------------------
//	@function:
//		COptimizerConfig::PocDefault
//
//	@doc:
//		Default optimizer configuration with the given cost model
//
//---------------------------------------------------------------------------
COptimizerConfig *
COptimizerConfig::PoconfDefault
	(
	IMemoryPool *memory_pool,
	ICostModel *pcm
	)
{
	GPOS_ASSERT(NULL != pcm);
	
	return GPOS_NEW(memory_pool) COptimizerConfig
						(
						GPOS_NEW(memory_pool) CEnumeratorConfig(memory_pool, 0 /*plan_id*/, 0 /*ullSamples*/),
						CStatisticsConfig::PstatsconfDefault(memory_pool),
						CCTEConfig::PcteconfDefault(memory_pool),
						pcm,
						CHint::PhintDefault(memory_pool),
						CWindowOids::GetWindowOids(memory_pool)
						);
}

//---------------------------------------------------------------------------
//	@function:
//		COptimizerConfig::Serialize
//
//	@doc:
//		Serialize optimizer configuration
//
//---------------------------------------------------------------------------
void
COptimizerConfig::Serialize(IMemoryPool *memory_pool, CXMLSerializer *xml_serializer, CBitSet *pbsTrace) const
{

	GPOS_ASSERT(NULL != xml_serializer);
	GPOS_ASSERT(NULL != pbsTrace);

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), CDXLTokens::GetDXLTokenStr(EdxltokenOptimizerConfig));

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), CDXLTokens::GetDXLTokenStr(EdxltokenEnumeratorConfig));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenPlanId), m_enumerator_cfg->GetPlanId());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenPlanSamples), m_enumerator_cfg->GetPlanId());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenCostThreshold), m_enumerator_cfg->GetPlanId());
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), CDXLTokens::GetDXLTokenStr(EdxltokenEnumeratorConfig));

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), CDXLTokens::GetDXLTokenStr(EdxltokenStatisticsConfig));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenDampingFactorFilter), m_pstatsconf->DDampingFactorFilter());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenDampingFactorJoin), m_pstatsconf->DDampingFactorJoin());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenDampingFactorGroupBy), m_pstatsconf->DDampingFactorGroupBy());
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), CDXLTokens::GetDXLTokenStr(EdxltokenStatisticsConfig));

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), CDXLTokens::GetDXLTokenStr(EdxltokenCTEConfig));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenCTEInliningCutoff), m_pcteconf->UlCTEInliningCutoff());
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), CDXLTokens::GetDXLTokenStr(EdxltokenCTEConfig));

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), CDXLTokens::GetDXLTokenStr(EdxltokenWindowOids));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenOidRowNumber), m_window_oids->OidRowNumber());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenOidRank), m_window_oids->OidRank());
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), CDXLTokens::GetDXLTokenStr(EdxltokenWindowOids));

	CCostModelConfigSerializer cmcSerializer(m_cost_model);
	cmcSerializer.Serialize(*xml_serializer);

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), CDXLTokens::GetDXLTokenStr(EdxltokenHint));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenMinNumOfPartsToRequireSortOnInsert), m_hint->UlMinNumOfPartsToRequireSortOnInsert());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenJoinArityForAssociativityCommutativity), m_hint->UlJoinArityForAssociativityCommutativity());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenArrayExpansionThreshold), m_hint->UlArrayExpansionThreshold());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenJoinOrderDPThreshold), m_hint->UlJoinOrderDPLimit());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenBroadcastThreshold), m_hint->UlBroadcastThreshold());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenEnforceConstraintsOnDML), m_hint->FEnforceConstraintsOnDML());
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), CDXLTokens::GetDXLTokenStr(EdxltokenHint));

	// Serialize traceflags represented in bitset into stream
	gpos::CBitSetIter bsi(*pbsTrace);
	CWStringDynamic wsTraceFlags(memory_pool);
	for (ULONG ul = 0; bsi.Advance(); ul++)
	{
		if (0 < ul)
		{
			wsTraceFlags.AppendCharArray(",");
		}

		wsTraceFlags.AppendFormat(GPOS_WSZ_LIT("%d"), bsi.Bit());
	}

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), CDXLTokens::GetDXLTokenStr(EdxltokenTraceFlags));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenValue), &wsTraceFlags);
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), CDXLTokens::GetDXLTokenStr(EdxltokenTraceFlags));

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), CDXLTokens::GetDXLTokenStr(EdxltokenOptimizerConfig));
}

// EOF
