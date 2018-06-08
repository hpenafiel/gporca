//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CMDProviderMemory.cpp
//
//	@doc:
//		Implementation of a memory-based metadata provider, which loads all
//		objects in memory and provides a function for looking them up by id.
//---------------------------------------------------------------------------

#include "gpos/io/COstreamString.h"
#include "gpos/memory/IMemoryPool.h"
#include "gpos/task/CWorker.h"
#include "gpos/common/CAutoP.h"
#include "gpos/common/CAutoRef.h"
#include "gpos/error/CAutoTrace.h"

#include "naucrates/md/CMDProviderMemory.h"
#include "naucrates/md/CMDTypeInt4GPDB.h"
#include "naucrates/md/CMDTypeInt8GPDB.h"
#include "naucrates/md/CMDTypeBoolGPDB.h"
#include "naucrates/md/CDXLRelStats.h"
#include "naucrates/md/CDXLColStats.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/exception.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpdxl;
using namespace gpmd;
using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CMDProviderMemory::CMDProviderMemory
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDProviderMemory::CMDProviderMemory
	(
	IMemoryPool *memory_pool,
	const CHAR *szFileName
	)
	:
	m_pmdmap(NULL)
{
	GPOS_ASSERT(NULL != szFileName);
	
	// read DXL file
	CAutoRg<CHAR> a_szDXL;
	a_szDXL = CDXLUtils::Read(memory_pool, szFileName);

	CAutoRef<DrgPimdobj> a_pdrgpmdobj;
	a_pdrgpmdobj = CDXLUtils::ParseDXLToIMDObjectArray(memory_pool, a_szDXL.Rgt(), NULL /*xsd_file_path*/);
	
#ifdef GPOS_DEBUG
	CWorker::Self()->ResetTimeSlice();
#endif // GPOS_DEBUG

	
	LoadMetadataObjectsFromArray(memory_pool, a_pdrgpmdobj.Value());
}

//---------------------------------------------------------------------------
//	@function:
//		CMDProviderMemory::CMDProviderMemory
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDProviderMemory::CMDProviderMemory
	(
	IMemoryPool *memory_pool,
	DrgPimdobj *pdrgpmdobj
	)
	:
	m_pmdmap(NULL)
{
	LoadMetadataObjectsFromArray(memory_pool, pdrgpmdobj);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDProviderMemory::LoadMetadataObjectsFromArray
//
//	@doc:
//		Loads the metadata objects from the given file
//
//---------------------------------------------------------------------------
void
CMDProviderMemory::LoadMetadataObjectsFromArray
	(
	IMemoryPool *memory_pool,
	DrgPimdobj *pdrgpmdobj
	)
{
	GPOS_ASSERT(NULL != pdrgpmdobj);

	// load metadata objects from the file
	CAutoRef<MDMap> a_pmdmap;
	m_pmdmap = GPOS_NEW(memory_pool) MDMap(memory_pool);
	a_pmdmap = m_pmdmap;

	const ULONG size = pdrgpmdobj->Size();

	// load objects into the hash map
	for (ULONG ul = 0; ul < size; ul++)
	{
		GPOS_CHECK_ABORT;

		IMDCacheObject *pmdobj = (*pdrgpmdobj)[ul];
		IMDId *pmdidKey = pmdobj->MDId();
		pmdidKey->AddRef();
		CAutoRef<IMDId> a_pmdidKey;
		a_pmdidKey = pmdidKey;
		
		CAutoP<CWStringDynamic> a_pstr;
		a_pstr = CDXLUtils::SerializeMDObj(memory_pool, pmdobj, true /*fSerializeHeaders*/, false /*findent*/);
		
		GPOS_CHECK_ABORT;
		BOOL fInserted = m_pmdmap->Insert(pmdidKey, a_pstr.Value());
		if (!fInserted)
		{
			
			GPOS_RAISE(gpdxl::ExmaMD, gpdxl::ExmiMDCacheEntryDuplicate, pmdidKey->GetBuffer());
		}
		(void) a_pmdidKey.Reset();
		(void) a_pstr.Reset();
	}
	
	// safely completed loading
	(void) a_pmdmap.Reset();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDProviderMemory::~CMDProviderMemory
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDProviderMemory::~CMDProviderMemory()
{
	CRefCount::SafeRelease(m_pmdmap);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDProviderMemory::PstrObject
//
//	@doc:
//		Returns the DXL of the requested object in the provided memory pool
//
//---------------------------------------------------------------------------
CWStringBase *
CMDProviderMemory::PstrObject
	(
	IMemoryPool *memory_pool,
	CMDAccessor *, //md_accessor
	IMDId *mdid
	) 
	const
{
	GPOS_ASSERT(NULL != m_pmdmap);

	const CWStringDynamic *pstrObj = m_pmdmap->Find(mdid);
	
	// result string
	CAutoP<CWStringDynamic> a_pstrResult;

	a_pstrResult = NULL;
	
	if (NULL == pstrObj)
	{
		// Relstats and colstats are special as they may not
		// exist in the metadata file. Provider must return dummy objects
		// in this case.
		switch(mdid->Emdidt())
		{
			case IMDId::EmdidRelStats:
			{
				mdid->AddRef();
				CAutoRef<CDXLRelStats> a_pdxlrelstats;
				a_pdxlrelstats = CDXLRelStats::PdxlrelstatsDummy(memory_pool, mdid);
				a_pstrResult = CDXLUtils::SerializeMDObj(memory_pool, a_pdxlrelstats.Value(), true /*fSerializeHeaders*/, false /*findent*/);
				break;
			}
			case IMDId::EmdidColStats:
			{
				CAutoP<CWStringDynamic> a_pstr;
				a_pstr = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool, mdid->GetBuffer());
				CAutoP<CMDName> a_pmdname;
				a_pmdname = GPOS_NEW(memory_pool) CMDName(memory_pool, a_pstr.Value());
				mdid->AddRef();
				CAutoRef<CDXLColStats> a_pdxlcolstats;
				a_pdxlcolstats = CDXLColStats::PdxlcolstatsDummy(memory_pool, mdid, a_pmdname.Value(), CStatistics::DDefaultColumnWidth /* width */);
				a_pmdname.Reset();
				a_pstrResult = CDXLUtils::SerializeMDObj(memory_pool, a_pdxlcolstats.Value(), true /*fSerializeHeaders*/, false /*findent*/);
				break;
			}
			default:
			{
				GPOS_RAISE(gpdxl::ExmaMD, gpdxl::ExmiMDCacheEntryNotFound, mdid->GetBuffer());
			}
		}
	}
	else
	{
		// copy string into result
		a_pstrResult = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool, pstrObj->GetBuffer());
	}
	
	GPOS_ASSERT(NULL != a_pstrResult.Value());
	
	return a_pstrResult.Reset();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDProviderMemory::MDId
//
//	@doc:
//		Returns the mdid for the requested system and type info. 
//		The caller takes ownership over the object.
//
//---------------------------------------------------------------------------
IMDId *
CMDProviderMemory::MDId
	(
	IMemoryPool *memory_pool,
	CSystemId sysid,
	IMDType::ETypeInfo eti
	) 
	const
{
	return PmdidTypeGPDB(memory_pool, sysid, eti);
}

// EOF
