//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLColStats.cpp
//
//	@doc:
//		Implementation of the class for representing column stats in DXL
//---------------------------------------------------------------------------


#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/CDXLColStats.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

#include "gpos/common/CAutoRef.h"

#include "naucrates/statistics/CStatistics.h"

using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CDXLColStats::CDXLColStats
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLColStats::CDXLColStats
	(
	IMemoryPool *memory_pool,
	CMDIdColStats *mdid_col_stats,
	CMDName *mdname,
	CDouble width,
	CDouble null_freq,
	CDouble distinct_remaining,
	CDouble freq_remaining,
	DrgPdxlbucket *stats_bucket_dxl_array,
	BOOL is_col_stats_missing
	)
	:
	m_memory_pool(memory_pool),
	m_mdid_col_stats(mdid_col_stats),
	m_mdname(mdname),
	m_width(width),
	m_null_freq(null_freq),
	m_distint_remaining(distinct_remaining),
	m_freq_remaining(freq_remaining),
	m_stats_bucket_dxl_array(stats_bucket_dxl_array),
	m_is_col_stats_missing(is_col_stats_missing)
{
	GPOS_ASSERT(mdid_col_stats->IsValid());
	GPOS_ASSERT(NULL != stats_bucket_dxl_array);
	m_pstr = CDXLUtils::SerializeMDObj(m_memory_pool, this, false /*fSerializeHeader*/, false /*indentation*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLColStats::~CDXLColStats
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLColStats::~CDXLColStats()
{
	GPOS_DELETE(m_mdname);
	GPOS_DELETE(m_pstr);
	m_mdid_col_stats->Release();
	m_stats_bucket_dxl_array->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLColStats::MDId
//
//	@doc:
//		Returns the metadata id of this column stats object
//
//---------------------------------------------------------------------------
IMDId *
CDXLColStats::MDId() const
{
	return m_mdid_col_stats;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLColStats::Mdname
//
//	@doc:
//		Returns the name of this column
//
//---------------------------------------------------------------------------
CMDName
CDXLColStats::Mdname() const
{
	return *m_mdname;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLColStats::GetMDName
//
//	@doc:
//		Returns the DXL string for this object
//
//---------------------------------------------------------------------------
const CWStringDynamic *
CDXLColStats::Pstr() const
{
	return m_pstr;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLColStats::UlBuckets
//
//	@doc:
//		Returns the number of buckets in the histogram
//
//---------------------------------------------------------------------------
ULONG
CDXLColStats::UlBuckets() const
{
	return m_stats_bucket_dxl_array->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLColStats::GetBucketDXL
//
//	@doc:
//		Returns the bucket at the given position
//
//---------------------------------------------------------------------------
const CDXLBucket *
CDXLColStats::GetBucketDXL
	(
	ULONG ul
	) 
	const
{
	return (*m_stats_bucket_dxl_array)[ul];
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLColStats::Serialize
//
//	@doc:
//		Serialize column stats in DXL format
//
//---------------------------------------------------------------------------
void
CDXLColStats::Serialize
	(
	CXMLSerializer *xml_serializer
	) 
	const
{
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
						CDXLTokens::GetDXLTokenStr(EdxltokenColumnStats));
	
	m_mdid_col_stats->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMdid));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenName), m_mdname->GetMDName());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenWidth), m_width);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenColNullFreq), m_null_freq);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenColNdvRemain), m_distint_remaining);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenColFreqRemain), m_freq_remaining);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenColStatsMissing), m_is_col_stats_missing);

	GPOS_CHECK_ABORT;

	ULONG num_of_buckets = UlBuckets();
	for (ULONG ul = 0; ul < num_of_buckets; ul++)
	{
		const CDXLBucket *pdxlbucket = GetBucketDXL(ul);
		pdxlbucket->Serialize(xml_serializer);

		GPOS_CHECK_ABORT;
	}
	
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
						CDXLTokens::GetDXLTokenStr(EdxltokenColumnStats));
}



#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLColStats::DebugPrint
//
//	@doc:
//		Dbug print of the column stats object
//
//---------------------------------------------------------------------------
void
CDXLColStats::DebugPrint
	(
	IOstream &os
	)
	const
{
	os << "Column id: ";
	MDId()->OsPrint(os);
	os << std::endl;
	
	os << "Column name: " << (Mdname()).GetMDName()->GetBuffer() << std::endl;
	
	for (ULONG ul = 0; ul < UlBuckets(); ul++)
	{
		const CDXLBucket *pdxlbucket = GetBucketDXL(ul);
		pdxlbucket->DebugPrint(os);
	}
}

#endif // GPOS_DEBUG

//---------------------------------------------------------------------------
//	@function:
//		CDXLColStats::PdxlcolstatsDummy
//
//	@doc:
//		Dummy statistics
//
//---------------------------------------------------------------------------
CDXLColStats *
CDXLColStats::PdxlcolstatsDummy
	(
	IMemoryPool *memory_pool,
	IMDId *mdid,
	CMDName *mdname,
	CDouble width
	)
{
	CMDIdColStats *mdid_col_stats = CMDIdColStats::PmdidConvert(pmdid);

	CAutoRef<DrgPdxlbucket> a_pdrgpdxlbucket;
	a_pdrgpdxlbucket = GPOS_NEW(memory_pool) DrgPdxlbucket(memory_pool);
	CAutoRef<CDXLColStats> a_pdxlcolstats;
	a_pdxlcolstats = GPOS_NEW(memory_pool) CDXLColStats
					(
					memory_pool,
					mdid_col_stats,
					mdname,
					width,
					CHistogram::DDefaultNullFreq,
					CHistogram::DDefaultNDVRemain,
					CHistogram::DDefaultNDVFreqRemain,
					a_pdrgpdxlbucket.Value(),
					true /* is_col_stats_missing */
					);
	a_pdrgpdxlbucket.Reset();
	return a_pdxlcolstats.Reset();
}

// EOF

