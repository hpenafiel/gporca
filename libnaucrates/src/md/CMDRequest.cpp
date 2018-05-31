//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CMDRequest.cpp
//
//	@doc:
//		Implementation of the class for metadata requests
//---------------------------------------------------------------------------


#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/CMDRequest.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CMDRequest::CMDRequest
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDRequest::CMDRequest
	(
	IMemoryPool *memory_pool,
	DrgPmdid *mdid_array,
	DrgPtr *pdrgptr
	)
	:
	m_memory_pool(memory_pool),
	m_mdid_array(mdid_array),
	m_pdrgptr(pdrgptr)
{
	GPOS_ASSERT(NULL != memory_pool);
	GPOS_ASSERT(NULL != mdid_array);
	GPOS_ASSERT(NULL != pdrgptr);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRequest::CMDRequest
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDRequest::CMDRequest
	(
	IMemoryPool *memory_pool,
	SMDTypeRequest *pmdtr
	)
	:
	m_memory_pool(memory_pool),
	m_mdid_array(NULL),
	m_pdrgptr(NULL)
{
	GPOS_ASSERT(NULL != memory_pool);
	GPOS_ASSERT(NULL != pmdtr);
	
	m_mdid_array = GPOS_NEW(m_memory_pool) DrgPmdid(m_memory_pool);
	m_pdrgptr = GPOS_NEW(m_memory_pool) DrgPtr(m_memory_pool);
	
	m_pdrgptr->Append(pmdtr);	
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRequest::~CMDRequest
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDRequest::~CMDRequest()
{
	m_mdid_array->Release();
	m_pdrgptr->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRequest::Pstr
//
//	@doc:
//		Serialize system id
//
//---------------------------------------------------------------------------
CWStringDynamic *
CMDRequest::Pstr
	(
	CSystemId sysid
	) 
{
	CWStringDynamic *pstr = GPOS_NEW(m_memory_pool) CWStringDynamic(m_memory_pool);
	pstr->AppendFormat(GPOS_WSZ_LIT("%d.%ls"), sysid.Emdidt(), sysid.GetBuffer());
	return pstr;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRequest::Serialize
//
//	@doc:
//		Serialize relation metadata in DXL format
//
//---------------------------------------------------------------------------
void
CMDRequest::Serialize
	(
	CXMLSerializer *xml_serializer
	) 
{
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenMDRequest));

	const ULONG ulMdids = m_mdid_array->Size();
	for (ULONG ul = 0; ul < ulMdids; ul++)
	{
		IMDId *pmdid = (*m_mdid_array)[ul];
		xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), 
										CDXLTokens::PstrToken(EdxltokenMdid));				
		pmdid->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenValue));
		xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), 
						CDXLTokens::PstrToken(EdxltokenMdid));
	}

	const ULONG ulTypeRequests = m_pdrgptr->Size();
	for (ULONG ul = 0; ul < ulTypeRequests; ul++)
	{
		SMDTypeRequest *pmdtr = (*m_pdrgptr)[ul];
		xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), 
										CDXLTokens::PstrToken(EdxltokenMDTypeRequest));				
		
		CWStringDynamic *pstr = Pstr(pmdtr->m_sysid);
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenSysid), pstr);
		GPOS_DELETE(pstr);
		
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenTypeInfo), pmdtr->m_eti);
		
		xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), 
										CDXLTokens::PstrToken(EdxltokenMDTypeRequest));				
	}
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenMDRequest));
}



// EOF

