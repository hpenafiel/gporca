//	Greenplum Database
//	Copyright (C) 2018 Pivotal Software, Inc.

#include "naucrates/dxl/CCostModelConfigSerializer.h"
#include "naucrates/dxl/xml/dxltokens.h"
#include "gpdbcost/CCostModelParamsGPDB.h"

#include "gpos/common/CAutoRef.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpdxl;
using gpos::CAutoRef;

void CCostModelConfigSerializer::Serialize(CXMLSerializer &xml_serializer) const
{
	if (ICostModel::EcmtGPDBLegacy == m_cost_model->Ecmt())
	{
		return;
	}

	xml_serializer.OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenCostModelConfig));
	xml_serializer.AddAttribute(CDXLTokens::PstrToken(EdxltokenCostModelType), m_cost_model->Ecmt());
	xml_serializer.AddAttribute(CDXLTokens::PstrToken(EdxltokenSegmentsForCosting), m_cost_model->UlHosts());

	xml_serializer.OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenCostParams));

	xml_serializer.OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenCostParam));

	xml_serializer.AddAttribute(CDXLTokens::PstrToken(EdxltokenName), m_cost_model->Pcp()->SzNameLookup(CCostModelParamsGPDB::EcpNLJFactor));
	xml_serializer.AddAttribute(CDXLTokens::PstrToken(EdxltokenValue), m_cost_model->Pcp()->PcpLookup(CCostModelParamsGPDB::EcpNLJFactor)->Get());
	xml_serializer.AddAttribute(CDXLTokens::PstrToken(EdxltokenCostParamLowerBound), m_cost_model->Pcp()->PcpLookup(CCostModelParamsGPDB::EcpNLJFactor)->GetLowerBoundVal());
	xml_serializer.AddAttribute(CDXLTokens::PstrToken(EdxltokenCostParamUpperBound), m_cost_model->Pcp()->PcpLookup(CCostModelParamsGPDB::EcpNLJFactor)->GetUpperBoundVal());
	xml_serializer.CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenCostParam));

	xml_serializer.CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenCostParams));

	xml_serializer.CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenCostModelConfig));
}

CCostModelConfigSerializer::CCostModelConfigSerializer
	(
	const gpopt::ICostModel *cost_model
	)
	:
	m_cost_model(cost_model)
{
}
