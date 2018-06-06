//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLRelStats.cpp
//
//	@doc:
//		Implementation of the class for representing relation stats in DXL
//---------------------------------------------------------------------------


#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/CDXLRelStats.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "gpos/common/CAutoP.h"
#include "gpos/common/CAutoRef.h"

using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CDXLRelStats::CDXLRelStats
//
//	@doc:
//		Constructs a metadata relation
//
//---------------------------------------------------------------------------
CDXLRelStats::CDXLRelStats
	(
	IMemoryPool *memory_pool,
	CMDIdRelStats *pmdidRelStats,
	CMDName *mdname,
	CDouble dRows,
	BOOL fEmpty
	)
	:
	m_memory_pool(memory_pool),
	m_pmdidRelStats(pmdidRelStats),
	m_mdname(mdname),
	m_dRows(dRows),
	m_fEmpty(fEmpty)
{
	GPOS_ASSERT(pmdidRelStats->IsValid());
	m_pstr = CDXLUtils::SerializeMDObj(m_memory_pool, this, false /*fSerializeHeader*/, false /*indentation*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLRelStats::~CDXLRelStats
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLRelStats::~CDXLRelStats()
{
	GPOS_DELETE(m_mdname);
	GPOS_DELETE(m_pstr);
	m_pmdidRelStats->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLRelStats::MDId
//
//	@doc:
//		Returns the metadata id of this relation stats object
//
//---------------------------------------------------------------------------
IMDId *
CDXLRelStats::MDId() const
{
	return m_pmdidRelStats;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLRelStats::Mdname
//
//	@doc:
//		Returns the name of this relation
//
//---------------------------------------------------------------------------
CMDName
CDXLRelStats::Mdname() const
{
	return *m_mdname;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLRelStats::GetMDName
//
//	@doc:
//		Returns the DXL string for this object
//
//---------------------------------------------------------------------------
const CWStringDynamic *
CDXLRelStats::Pstr() const
{
	return m_pstr;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLRelStats::DRows
//
//	@doc:
//		Returns the number of rows
//
//---------------------------------------------------------------------------
CDouble
CDXLRelStats::DRows() const
{
	return m_dRows;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLRelStats::Serialize
//
//	@doc:
//		Serialize relation stats in DXL format
//
//---------------------------------------------------------------------------
void
CDXLRelStats::Serialize
	(
	CXMLSerializer *xml_serializer
	) const
{
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
						CDXLTokens::GetDXLTokenStr(EdxltokenRelationStats));
	
	m_pmdidRelStats->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMdid));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenName), m_mdname->GetMDName());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenRows), m_dRows);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenEmptyRelation), m_fEmpty);

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
						CDXLTokens::GetDXLTokenStr(EdxltokenRelationStats));

	GPOS_CHECK_ABORT;
}



#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLRelStats::DebugPrint
//
//	@doc:
//		Prints a metadata cache relation to the provided output
//
//---------------------------------------------------------------------------
void
CDXLRelStats::DebugPrint
	(
	IOstream &os
	)
	const
{
	os << "Relation id: ";
	MDId()->OsPrint(os);
	os << std::endl;
	
	os << "Relation name: " << (Mdname()).GetMDName()->GetBuffer() << std::endl;
	
	os << "Rows: " << DRows() << std::endl;

	os << "Empty: " << IsEmpty() << std::endl;
}

#endif // GPOS_DEBUG

//---------------------------------------------------------------------------
//	@function:
//		CDXLRelStats::PdxlrelstatsDummy
//
//	@doc:
//		Dummy relation stats
//
//---------------------------------------------------------------------------
CDXLRelStats *
CDXLRelStats::PdxlrelstatsDummy
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid
	)
{
	CMDIdRelStats *pmdidRelStats = CMDIdRelStats::PmdidConvert(pmdid);
	CAutoP<CWStringDynamic> a_pstr;
	a_pstr = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool, pmdidRelStats->GetBuffer());
	CAutoP<CMDName> a_pmdname;
	a_pmdname = GPOS_NEW(memory_pool) CMDName(memory_pool, a_pstr.Value());
	CAutoRef<CDXLRelStats> a_pdxlrelstats;
	a_pdxlrelstats = GPOS_NEW(memory_pool) CDXLRelStats(memory_pool, pmdidRelStats, a_pmdname.Value(), CStatistics::DDefaultColumnWidth, false /* fEmpty */);
	a_pmdname.Reset();
	return a_pdxlrelstats.Reset();
}

// EOF

