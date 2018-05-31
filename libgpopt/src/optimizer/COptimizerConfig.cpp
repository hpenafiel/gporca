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
	m_pec(pec),
	m_pstatsconf(pstatsconf),
	m_pcteconf(pcteconf),
	m_cost_model(cost_model),
	m_phint(phint),
	m_pwindowoids(pwindowoids)
{
	GPOS_ASSERT(NULL != pec);
	GPOS_ASSERT(NULL != pstatsconf);
	GPOS_ASSERT(NULL != pcteconf);
	GPOS_ASSERT(NULL != m_cost_model);
	GPOS_ASSERT(NULL != phint);
	GPOS_ASSERT(NULL != m_pwindowoids);
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
	m_pec->Release();
	m_pstatsconf->Release();
	m_pcteconf->Release();
	m_cost_model->Release();
	m_phint->Release();
	m_pwindowoids->Release();
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
						CWindowOids::Pwindowoids(memory_pool)
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
						CWindowOids::Pwindowoids(memory_pool)
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

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenOptimizerConfig));

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenEnumeratorConfig));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenPlanId), m_pec->GetPlanId());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenPlanSamples), m_pec->GetPlanId());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenCostThreshold), m_pec->GetPlanId());
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenEnumeratorConfig));

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenStatisticsConfig));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenDampingFactorFilter), m_pstatsconf->DDampingFactorFilter());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenDampingFactorJoin), m_pstatsconf->DDampingFactorJoin());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenDampingFactorGroupBy), m_pstatsconf->DDampingFactorGroupBy());
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenStatisticsConfig));

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenCTEConfig));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenCTEInliningCutoff), m_pcteconf->UlCTEInliningCutoff());
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenCTEConfig));

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenWindowOids));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenOidRowNumber), m_pwindowoids->OidRowNumber());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenOidRank), m_pwindowoids->OidRank());
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenWindowOids));

	CCostModelConfigSerializer cmcSerializer(m_cost_model);
	cmcSerializer.Serialize(*xml_serializer);

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenHint));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenMinNumOfPartsToRequireSortOnInsert), m_phint->UlMinNumOfPartsToRequireSortOnInsert());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenJoinArityForAssociativityCommutativity), m_phint->UlJoinArityForAssociativityCommutativity());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenArrayExpansionThreshold), m_phint->UlArrayExpansionThreshold());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenJoinOrderDPThreshold), m_phint->UlJoinOrderDPLimit());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenBroadcastThreshold), m_phint->UlBroadcastThreshold());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenEnforceConstraintsOnDML), m_phint->FEnforceConstraintsOnDML());
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenHint));

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

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenTraceFlags));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenValue), &wsTraceFlags);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenTraceFlags));

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenOptimizerConfig));
}

// EOF
