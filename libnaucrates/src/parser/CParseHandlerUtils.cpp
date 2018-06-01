//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerUtils.cpp
//
//	@doc:
//		Implementation of the helper methods for parse handler
//		
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerUtils.h"

#include "naucrates/statistics/IStatistics.h"

using namespace gpos;
using namespace gpdxl;
using namespace gpnaucrates;

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerUtils::SetProperties
//
//	@doc:
//		Parse and the set operator's costing and statistical properties
//
//---------------------------------------------------------------------------
void
CParseHandlerUtils::SetProperties
	(
	CDXLNode *pdxln,
	CParseHandlerProperties *pphProp
	)
{
	GPOS_ASSERT(NULL != pphProp->GetProperties());
	// set physical properties
	CDXLPhysicalProperties *dxl_properties = pphProp->GetProperties();
	dxl_properties->AddRef();
	pdxln->SetProperties(dxl_properties);

	// set the statistical information
	CDXLStatsDerivedRelation *pdxlstatsderrel = pphProp->Pdxlstatsderrel();
	if (NULL != pdxlstatsderrel)
	{
		pdxlstatsderrel->AddRef();
		dxl_properties->SetStats(pdxlstatsderrel);
	}
}

// EOF
