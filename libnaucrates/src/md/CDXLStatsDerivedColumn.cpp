//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLStatsDerivedColumn.cpp
//
//	@doc:
//		Implementation of the class for representing dxl derived column statistics
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"
#include "naucrates/md/CDXLStatsDerivedColumn.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/statistics/CStatistics.h"

using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CDXLStatsDerivedColumn::CDXLStatsDerivedColumn
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLStatsDerivedColumn::CDXLStatsDerivedColumn
	(
	ULONG col_id,
	CDouble dWidth,
	CDouble dNullFreq,
	CDouble dDistinctRemain,
	CDouble dFreqRemain,
	DrgPdxlbucket *stats_bucket_dxl_array
	)
	:
	m_ulColId(col_id),
	m_dWidth(dWidth),
	m_dNullFreq(dNullFreq),
	m_dDistinctRemain(dDistinctRemain),
	m_dFreqRemain(dFreqRemain),
	m_pdrgpdxlbucket(stats_bucket_dxl_array)
{
	GPOS_ASSERT(0 <= m_dWidth);
	GPOS_ASSERT(0 <= m_dNullFreq);
	GPOS_ASSERT(0 <= m_dDistinctRemain);
	GPOS_ASSERT(0 <= m_dFreqRemain);
	GPOS_ASSERT(NULL != m_pdrgpdxlbucket);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLStatsDerivedColumn::~CDXLStatsDerivedColumn
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLStatsDerivedColumn::~CDXLStatsDerivedColumn()
{
	m_pdrgpdxlbucket->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLStatsDerivedColumn::Pdrgpdxlbucket
//
//	@doc:
//		Returns the array of buckets
//
//---------------------------------------------------------------------------
const DrgPdxlbucket *
CDXLStatsDerivedColumn::Pdrgpdxlbucket() const
{
	return m_pdrgpdxlbucket;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLStatsDerivedColumn::Serialize
//
//	@doc:
//		Serialize bucket in DXL format
//
//---------------------------------------------------------------------------
void
CDXLStatsDerivedColumn::Serialize
	(
	CXMLSerializer *xml_serializer
	)
	const
{
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix),
						CDXLTokens::PstrToken(EdxltokenStatsDerivedColumn));

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenColId), m_ulColId);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenWidth), m_dWidth);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenColNullFreq), m_dNullFreq);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenColNdvRemain), m_dDistinctRemain);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenColFreqRemain), m_dFreqRemain);


	const ULONG ulBuckets = m_pdrgpdxlbucket->Size();
	for (ULONG ul = 0; ul < ulBuckets; ul++)
	{
		GPOS_CHECK_ABORT;

		CDXLBucket *pdxlbucket = (*m_pdrgpdxlbucket)[ul];
		pdxlbucket->Serialize(xml_serializer);

		GPOS_CHECK_ABORT;
	}

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix),
						CDXLTokens::PstrToken(EdxltokenStatsDerivedColumn));
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLStatsDerivedColumn::DebugPrint
//
//	@doc:
//		Debug print of the bucket object
//
//---------------------------------------------------------------------------
void
CDXLStatsDerivedColumn::DebugPrint
	(
	IOstream &os
	)
	const
{
	os << "Column id: " << m_ulColId;
	os << std::endl;
	os << "Width : " << m_dWidth;
	os << std::endl;

	const ULONG ulBuckets = m_pdrgpdxlbucket->Size();
	for (ULONG ul = 0; ul < ulBuckets; ul++)
	{
		const CDXLBucket *pdxlbucket = (*m_pdrgpdxlbucket)[ul];
		pdxlbucket->DebugPrint(os);
	}
}

#endif // GPOS_DEBUG

// EOF

