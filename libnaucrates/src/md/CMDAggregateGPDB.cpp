//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CMDAggregateGPDB.cpp
//
//	@doc:
//		Implementation of the class for representing GPDB-specific aggregates
//		in the MD cache
//---------------------------------------------------------------------------


#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/CMDAggregateGPDB.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpmd;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CMDAggregateGPDB::CMDAggregateGPDB
//
//	@doc:
//		Constructs a metadata aggregate
//
//---------------------------------------------------------------------------
CMDAggregateGPDB::CMDAggregateGPDB
	(
	IMemoryPool *memory_pool,
	IMDId *mdid,
	CMDName *mdname,
	IMDId *pmdidTypeResult,
	IMDId *pmdidTypeIntermediate,
	BOOL fOrdered,
	BOOL fSplittable,
	BOOL fHashAggCapable
	)
	:
	m_memory_pool(memory_pool),
	m_mdid(mdid),
	m_mdname(mdname),
	m_mdid_type_result(pmdidTypeResult),
	m_mdid_type_intermediate(pmdidTypeIntermediate),
	m_is_ordered(fOrdered),
	m_is_splittable(fSplittable),
	m_hash_agg_capable(fHashAggCapable)
	{
		GPOS_ASSERT(mdid->IsValid());
		
		m_pstr = CDXLUtils::SerializeMDObj(m_memory_pool, this, false /*fSerializeHeader*/, false /*indentation*/);
	}

//---------------------------------------------------------------------------
//	@function:
//		CMDAggregateGPDB::~CMDAggregateGPDB
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CMDAggregateGPDB::~CMDAggregateGPDB()
{
	m_mdid->Release();
	m_mdid_type_intermediate->Release();
	m_mdid_type_result->Release();
	GPOS_DELETE(m_mdname);
	GPOS_DELETE(m_pstr);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAggregateGPDB::MDId
//
//	@doc:
//		Agg id
//
//---------------------------------------------------------------------------
IMDId *
CMDAggregateGPDB::MDId() const
{
	return m_mdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAggregateGPDB::Mdname
//
//	@doc:
//		Agg name
//
//---------------------------------------------------------------------------
CMDName
CMDAggregateGPDB::Mdname() const
{
	return *m_mdname;
}


//---------------------------------------------------------------------------
//	@function:
//		CMDAggregateGPDB::PmdidTypeResult
//
//	@doc:
//		Type id of result
//
//---------------------------------------------------------------------------
IMDId *
CMDAggregateGPDB::PmdidTypeResult() const
{
	return m_mdid_type_result;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAggregateGPDB::PmdidTypeIntermediate
//
//	@doc:
//		Type id of intermediate result
//
//---------------------------------------------------------------------------
IMDId *
CMDAggregateGPDB::PmdidTypeIntermediate() const
{
	return m_mdid_type_intermediate;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAggregateGPDB::Serialize
//
//	@doc:
//		Serialize function metadata in DXL format
//
//---------------------------------------------------------------------------
void
CMDAggregateGPDB::Serialize
	(
	CXMLSerializer *xml_serializer
	) 
	const
{
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
						CDXLTokens::GetDXLTokenStr(EdxltokenGPDBAgg));
	
	m_mdid->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMdid));

	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenName), m_mdname->GetMDName());
	if (m_is_ordered)
	{
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenGPDBIsAggOrdered), m_is_ordered);
	}
	
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenGPDBAggSplittable), m_is_splittable);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenGPDBAggHashAggCapable), m_hash_agg_capable);
	
	SerializeMDIdAsElem(xml_serializer, 
			CDXLTokens::GetDXLTokenStr(EdxltokenGPDBAggResultTypeId), m_mdid_type_result);
	SerializeMDIdAsElem(xml_serializer, 
			CDXLTokens::GetDXLTokenStr(EdxltokenGPDBAggIntermediateResultTypeId), m_mdid_type_intermediate);

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
						CDXLTokens::GetDXLTokenStr(EdxltokenGPDBAgg));
}


#ifdef GPOS_DEBUG

//---------------------------------------------------------------------------
//	@function:
//		CMDAggregateGPDB::DebugPrint
//
//	@doc:
//		Prints a metadata cache relation to the provided output
//
//---------------------------------------------------------------------------
void
CMDAggregateGPDB::DebugPrint
	(
	IOstream &os
	)
	const
{
	os << "Aggregate id: ";
	MDId()->OsPrint(os);
	os << std::endl;
	
	os << "Aggregate name: " << (Mdname()).GetMDName()->GetBuffer() << std::endl;
	
	os << "Result type id: ";
	PmdidTypeResult()->OsPrint(os);
	os << std::endl;
	
	os << "Intermediate result type id: ";
	PmdidTypeIntermediate()->OsPrint(os);
	os << std::endl;
	
	os << std::endl;	
}

#endif // GPOS_DEBUG

// EOF
