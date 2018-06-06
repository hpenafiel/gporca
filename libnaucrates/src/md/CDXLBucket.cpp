//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLBucket.cpp
//
//	@doc:
//		Implementation of the class for representing buckets in DXL column stats
//---------------------------------------------------------------------------


#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/CDXLBucket.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CDXLBucket::CDXLBucket
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLBucket::CDXLBucket
	(
	CDXLDatum *pdxldatumLower,
	CDXLDatum *pdxldatumUpper,
	BOOL fLowerClosed,
	BOOL fUpperClosed,
	CDouble dFrequency,
	CDouble dDistinct
	)
	:
	m_lower_bound_datum_dxl(pdxldatumLower),
	m_upper_bound_datum_dxl(pdxldatumUpper),
	m_is_lower_closed(fLowerClosed),
	m_is_upper_closed(fUpperClosed),
	m_frequency(dFrequency),
	m_distinct(dDistinct)
{
	GPOS_ASSERT(NULL != pdxldatumLower);
	GPOS_ASSERT(NULL != pdxldatumUpper);
	GPOS_ASSERT(m_frequency >= 0.0 && m_frequency <= 1.0);
	GPOS_ASSERT(m_distinct >= 0);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLBucket::~CDXLBucket
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLBucket::~CDXLBucket()
{
	m_lower_bound_datum_dxl->Release();
	m_upper_bound_datum_dxl->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLBucket::PdxldatumLower
//
//	@doc:
//		Returns the lower bound for the bucket
//
//---------------------------------------------------------------------------
const CDXLDatum *
CDXLBucket::PdxldatumLower() const
{
	return m_lower_bound_datum_dxl;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLBucket::PdxldatumUpper
//
//	@doc:
//		Returns the upper bound for the bucket
//
//---------------------------------------------------------------------------
const CDXLDatum *
CDXLBucket::PdxldatumUpper() const
{
	return m_upper_bound_datum_dxl;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLBucket::DFrequency
//
//	@doc:
//		Returns the frequency for this bucket
//
//---------------------------------------------------------------------------
CDouble
CDXLBucket::DFrequency() const
{
	return m_frequency;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLBucket::DDistinct
//
//	@doc:
//		Returns the number of distinct in this bucket
//
//---------------------------------------------------------------------------
CDouble
CDXLBucket::DDistinct() const
{
	return m_distinct;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLBucket::Serialize
//
//	@doc:
//		Serialize bucket in DXL format
//
//---------------------------------------------------------------------------
void
CDXLBucket::Serialize
	(
	CXMLSerializer *xml_serializer
	) 
	const
{
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
						CDXLTokens::GetDXLTokenStr(EdxltokenColumnStatsBucket));
	
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenStatsFrequency), m_frequency);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenStatsDistinct), m_distinct);
	
	SerializeBoundaryValue(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenStatsBucketLowerBound), m_lower_bound_datum_dxl, m_is_lower_closed);
	SerializeBoundaryValue(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenStatsBucketUpperBound), m_upper_bound_datum_dxl, m_is_upper_closed);
	
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
						CDXLTokens::GetDXLTokenStr(EdxltokenColumnStatsBucket));

	GPOS_CHECK_ABORT;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLBucket::Serialize
//
//	@doc:
//		Serialize the bucket boundary
//
//---------------------------------------------------------------------------
void
CDXLBucket::SerializeBoundaryValue
	(
	CXMLSerializer *xml_serializer,
	const CWStringConst *pstrElem,
	CDXLDatum *datum_dxl,
	BOOL fBoundClosed
	)
	const
{
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), pstrElem);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenStatsBoundClosed), fBoundClosed);
	datum_dxl->Serialize(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), pstrElem);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLBucket::DebugPrint
//
//	@doc:
//		Debug print of the bucket object
//
//---------------------------------------------------------------------------
void
CDXLBucket::DebugPrint
	(
	IOstream & //os
	)
	const
{
	// TODO:  - Feb 13, 2012; implement
}

#endif // GPOS_DEBUG

// EOF

