//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CMDAccessor.cpp
//
//	@doc:
//		Implementation of the metadata accessor class handling accesses to
//		metadata objects in an optimization session
//---------------------------------------------------------------------------

#include "gpos/common/CAutoP.h"
#include "gpos/common/CAutoRef.h"
#include "gpos/common/CTimerUser.h"
#include "gpos/io/COstreamString.h"
#include "gpos/task/CAutoSuspendAbort.h"
#include "gpos/error/CAutoTrace.h"

#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CColRefTable.h"
#include "gpopt/exception.h"
#include "gpopt/mdcache/CMDAccessor.h"
#include "gpopt/mdcache/CMDAccessorUtils.h"


#include "naucrates/exception.h"
#include "naucrates/traceflags/traceflags.h"

#include "naucrates/dxl/CDXLUtils.h"

#include "naucrates/md/IMDCacheObject.h"
#include "naucrates/md/IMDRelation.h"
#include "naucrates/md/IMDRelationExternal.h"
#include "naucrates/md/IMDType.h"
#include "naucrates/md/IMDScalarOp.h"
#include "naucrates/md/IMDFunction.h"
#include "naucrates/md/IMDAggregate.h"
#include "naucrates/md/IMDIndex.h"
#include "naucrates/md/IMDTrigger.h"
#include "naucrates/md/IMDCheckConstraint.h"
#include "naucrates/md/IMDRelStats.h"
#include "naucrates/md/IMDColStats.h"
#include "naucrates/md/IMDCast.h"
#include "naucrates/md/IMDScCmp.h"

#include "naucrates/md/CMDIdRelStats.h"
#include "naucrates/md/CMDIdColStats.h"
#include "naucrates/md/CMDIdCast.h"
#include "naucrates/md/CMDIdScCmp.h"

#include "naucrates/md/IMDProvider.h"
#include "naucrates/md/CMDProviderGeneric.h"

using namespace gpos;
using namespace gpmd;
using namespace gpopt;
using namespace gpdxl;

// no. of hashtable buckets
#define GPOPT_CACHEACC_HT_NUM_OF_BUCKETS 128

// static member initialization

// invalid mdid pointer
const MdidPtr CMDAccessor::SMDAccessorElem::m_pmdidInvalid = NULL;

// invalid md provider element
const CMDAccessor::SMDProviderElem CMDAccessor::SMDProviderElem::m_mdpelemInvalid (CSystemId(IMDId::EmdidSentinel, NULL, 0), NULL);

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::SMDAccessorElem::SMDAccessorElem
//
//	@doc:
//		Constructs a metadata accessor element for the accessors hashtable
//
//---------------------------------------------------------------------------
CMDAccessor::SMDAccessorElem::SMDAccessorElem
	(
	IMDCacheObject *pimdobj,
	IMDId *mdid
	)
	:
	m_imd_obj(pimdobj),
	m_mdid(mdid)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::SMDAccessorElem::~SMDAccessorElem
//
//	@doc:
//		Destructor for the metadata accessor element
//
//---------------------------------------------------------------------------
CMDAccessor::SMDAccessorElem::~SMDAccessorElem()
{
	// deleting the cache accessor will effectively unpin the cache entry for that object
	m_imd_obj->Release();
	m_mdid->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::SMDAccessorElem::MDId
//
//	@doc:
//		Return the key for this hashtable element
//
//---------------------------------------------------------------------------
IMDId *
CMDAccessor::SMDAccessorElem::MDId()
{
	return m_mdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::SMDAccessorElem::Equals
//
//	@doc:
//		Equality function for cache accessors hash table
//
//---------------------------------------------------------------------------
BOOL 
CMDAccessor::SMDAccessorElem::Equals
	(
	const MdidPtr &pmdidLeft,
	const MdidPtr &pmdidRight
	)
{
	if (pmdidLeft == m_pmdidInvalid || pmdidRight == m_pmdidInvalid)
	{
		return pmdidLeft == m_pmdidInvalid && pmdidRight == m_pmdidInvalid;
	}

	return pmdidLeft->Equals(pmdidRight);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::SMDAccessorElem::HashValue
//
//	@doc:
//		Hash function for cache accessors hash table
//
//---------------------------------------------------------------------------
ULONG 
CMDAccessor::SMDAccessorElem::HashValue
	(
	const MdidPtr& mdid
	)
{
	GPOS_ASSERT(m_pmdidInvalid != mdid);

	return mdid->HashValue();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::SMDProviderElem::SMDProviderElem
//
//	@doc:
//		Constructs an MD provider element
//
//---------------------------------------------------------------------------
CMDAccessor::SMDProviderElem::SMDProviderElem
	(
	CSystemId sysid,
	IMDProvider *pmdp
	)
	:
	m_sysid(sysid),
	m_pmdp(pmdp)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::SMDProviderElem::~SMDProviderElem
//
//	@doc:
//		Destructor for the MD provider element
//
//---------------------------------------------------------------------------
CMDAccessor::SMDProviderElem::~SMDProviderElem()
{
	CRefCount::SafeRelease(m_pmdp);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::SMDProviderElem::Pmdp
//
//	@doc:
//		Returns the MD provider for this hash table element
//
//---------------------------------------------------------------------------
IMDProvider *
CMDAccessor::SMDProviderElem::Pmdp()
{
	return m_pmdp;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::SMDProviderElem::Sysid
//
//	@doc:
//		Returns the system id for this hash table element
//
//---------------------------------------------------------------------------
CSystemId
CMDAccessor::SMDProviderElem::Sysid() const
{
	return m_sysid;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::SMDProviderElem::Equals
//
//	@doc:
//		Equality function for hash tables
//
//---------------------------------------------------------------------------
BOOL 
CMDAccessor::SMDProviderElem::Equals
	(
	const SMDProviderElem &mdpelemLeft,
	const SMDProviderElem &mdpelemRight
	)
{
	return mdpelemLeft.m_sysid.Equals(mdpelemRight.m_sysid);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::SMDProviderElem::HashValue
//
//	@doc:
//		Hash function for cost contexts hash table
//
//---------------------------------------------------------------------------
ULONG 
CMDAccessor::SMDProviderElem::HashValue
	(
	const SMDProviderElem &mdpelem
	)
{
	GPOS_ASSERT(!Equals(mdpelem, m_mdpelemInvalid));

	return mdpelem.m_sysid.HashValue();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::CMDAccessor
//
//	@doc:
//		Constructs a metadata accessor
//
//---------------------------------------------------------------------------
CMDAccessor::CMDAccessor
	(
	IMemoryPool *memory_pool,
	MDCache *pcache
	)
	:
	m_memory_pool(memory_pool),
	m_pcache(pcache),
	m_dLookupTime(0.0),
	m_dFetchTime(0.0)
{
	GPOS_ASSERT(NULL != m_memory_pool);
	GPOS_ASSERT(NULL != m_pcache);
	
	m_pmdpGeneric = GPOS_NEW(memory_pool) CMDProviderGeneric(memory_pool);
	
	InitHashtables(memory_pool);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::CMDAccessor
//
//	@doc:
//		Constructs a metadata accessor and registers an MD provider
//
//---------------------------------------------------------------------------
CMDAccessor::CMDAccessor
	(
	IMemoryPool *memory_pool,
	MDCache *pcache,
	CSystemId sysid,
	IMDProvider *pmdp
	)
	:
	m_memory_pool(memory_pool),
	m_pcache(pcache),
	m_dLookupTime(0.0),
	m_dFetchTime(0.0)
{
	GPOS_ASSERT(NULL != m_memory_pool);
	GPOS_ASSERT(NULL != m_pcache);
	
	m_pmdpGeneric = GPOS_NEW(memory_pool) CMDProviderGeneric(memory_pool);
	
	InitHashtables(memory_pool);
	
	RegisterProvider(sysid, pmdp);
}


//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::CMDAccessor
//
//	@doc:
//		Constructs a metadata accessor and registers MD providers
//
//---------------------------------------------------------------------------
CMDAccessor::CMDAccessor
	(
	IMemoryPool *memory_pool,
	MDCache *pcache,
	const DrgPsysid *pdrgpsysid,
	const DrgPmdp *pdrgpmdp
	)
	:
	m_memory_pool(memory_pool),
	m_pcache(pcache),
	m_dLookupTime(0.0),
	m_dFetchTime(0.0)
{
	GPOS_ASSERT(NULL != m_memory_pool);
	GPOS_ASSERT(NULL != m_pcache);

	m_pmdpGeneric = GPOS_NEW(memory_pool) CMDProviderGeneric(memory_pool);

	InitHashtables(memory_pool);

	RegisterProviders(pdrgpsysid, pdrgpmdp);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::DestroyAccessorElement
//
//	@doc:
//		Destroy accessor element;
//		called only at destruction time
//
//---------------------------------------------------------------------------
void
CMDAccessor::DestroyAccessorElement
	(
	SMDAccessorElem *pmdaccelem
	)
{
	GPOS_ASSERT(NULL != pmdaccelem);

	// remove deletion lock for mdid
	pmdaccelem->MDId()->RemoveDeletionLock();;

	GPOS_DELETE(pmdaccelem);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::DestroyProviderElement
//
//	@doc:
//		Destroy provider element;
//		called only at destruction time
//
//---------------------------------------------------------------------------
void
CMDAccessor::DestroyProviderElement
	(
	SMDProviderElem *pmdpelem
	)
{
	GPOS_DELETE(pmdpelem);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::InitHashtables
//
//	@doc:
//		Initializes the hash tables
//
//---------------------------------------------------------------------------
void
CMDAccessor::InitHashtables
	(
	IMemoryPool *memory_pool
	)
{
	// initialize Cache accessors hash table
	m_shtCacheAccessors.Init
				(
				memory_pool,
				GPOPT_CACHEACC_HT_NUM_OF_BUCKETS,
				GPOS_OFFSET(SMDAccessorElem, m_link),
				GPOS_OFFSET(SMDAccessorElem, m_mdid),
				&(SMDAccessorElem::m_pmdidInvalid),
				SMDAccessorElem::HashValue,
				SMDAccessorElem::Equals
				);
	
	// initialize MD providers hash table
	m_shtProviders.Init
		(
		memory_pool,
		GPOPT_CACHEACC_HT_NUM_OF_BUCKETS,
		GPOS_OFFSET(SMDProviderElem, m_link),
		0, // the HT element is used as key
		&(SMDProviderElem::m_mdpelemInvalid),
		SMDProviderElem::HashValue,
		SMDProviderElem::Equals
		);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::~CMDAccessor
//
//	@doc:
//		Destructor 
//
//---------------------------------------------------------------------------
CMDAccessor::~CMDAccessor()
{
	// release cache accessors and MD providers in hashtables
	m_shtCacheAccessors.DestroyEntries(DestroyAccessorElement);
	m_shtProviders.DestroyEntries(DestroyProviderElement);
	GPOS_DELETE(m_pmdpGeneric);

	if (GPOS_FTRACE(EopttracePrintOptimizationStatistics))
	{
		// print fetch time and lookup time
		CAutoTrace at(m_memory_pool);
		at.Os() << "[OPT]: Total metadata fetch time: " << m_dFetchTime << "ms" << std::endl;
		at.Os() << "[OPT]: Total metadata lookup time (including fetch time): " << m_dLookupTime << "ms" << std::endl;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::RegisterProvider
//
//	@doc:
//		Register a MD provider for the given source system id
//
//---------------------------------------------------------------------------
void
CMDAccessor::RegisterProvider
	(
	CSystemId sysid,
	IMDProvider *pmdp
	)
{	
	CAutoP<SMDProviderElem> a_pmdpelem;
	a_pmdpelem = GPOS_NEW(m_memory_pool) SMDProviderElem(sysid, pmdp);
	
	MDPHTAccessor mdhtacc(m_shtProviders, *(a_pmdpelem.Value()));

	GPOS_ASSERT(NULL == mdhtacc.Find());
	
	// insert provider in the hash table
	mdhtacc.Insert(a_pmdpelem.Value());
	a_pmdpelem.Reset();
}


//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::RegisterProviders
//
//	@doc:
//		Register given MD providers
//
//---------------------------------------------------------------------------
void
CMDAccessor::RegisterProviders
	(
	const DrgPsysid *pdrgpsysid,
	const DrgPmdp *pdrgpmdp
	)
{
	GPOS_ASSERT(NULL != pdrgpmdp);
	GPOS_ASSERT(NULL != pdrgpsysid);
	GPOS_ASSERT(pdrgpmdp->Size() == pdrgpsysid->Size());
	GPOS_ASSERT(0 < pdrgpmdp->Size());

	const ULONG ulProviders = pdrgpmdp->Size();
	for (ULONG ul = 0; ul < ulProviders; ul++)
	{
		IMDProvider *pmdp = (*pdrgpmdp)[ul];
		pmdp->AddRef();
		RegisterProvider(*((*pdrgpsysid)[ul]), pmdp);
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::Pmdp
//
//	@doc:
//		Retrieve the MD provider for the given source system id
//
//---------------------------------------------------------------------------
IMDProvider *
CMDAccessor::Pmdp
	(
	CSystemId sysid
	)
{	
	SMDProviderElem *pmdpelem = NULL;
	
	{
		// scope for HT accessor
		
		SMDProviderElem mdpelem(sysid, NULL /*pmdp*/);
		MDPHTAccessor mdhtacc(m_shtProviders, mdpelem);
		
		pmdpelem = mdhtacc.Find();
	}
	
	GPOS_ASSERT(NULL != pmdpelem && "Could not find MD provider");
	
	return pmdpelem->Pmdp();
}



//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::GetImdObj
//
//	@doc:
//		Retrieves a metadata cache object from the md cache, possibly retrieving
//		it from the external metadata provider and storing it in the cache first.
//		Main workhorse for retrieving the different types of md cache objects.
//
//---------------------------------------------------------------------------
const IMDCacheObject *
CMDAccessor::GetImdObj
	(
	IMDId *mdid
	)
{
	BOOL fPrintOptStats = GPOS_FTRACE(EopttracePrintOptimizationStatistics);
	CTimerUser timerLookup; // timer to measure lookup time
	if (fPrintOptStats)
	{
		timerLookup.Restart();
	}

	const IMDCacheObject *pimdobj = NULL;

	// first, try to locate object in local hashtable
	{
		// scope for ht accessor
		MDHTAccessor mdhtacc(m_shtCacheAccessors, mdid);
		SMDAccessorElem *pmdaccelem = mdhtacc.Find(); 
		if (NULL != pmdaccelem)
		{
			pimdobj = pmdaccelem->GetImdObj();
		}
	}

	if (NULL == pimdobj)
	{
		// object not in local hashtable, try lookup in the MD cache
		
		// construct a key for cache lookup
		IMDProvider *pmdp = Pmdp(mdid->Sysid());
		
		CMDKey mdkey(mdid);
				
		CAutoP<CacheAccessorMD> a_pmdcacc;
		a_pmdcacc = GPOS_NEW(m_memory_pool) CacheAccessorMD(m_pcache);
		a_pmdcacc->Lookup(&mdkey);
		IMDCacheObject *pmdobjNew = a_pmdcacc->Val();
		if (NULL == pmdobjNew)
		{
			// object not found in MD cache: retrieve it from MD provider
			CTimerUser timerFetch;
			if (fPrintOptStats)
			{
				timerFetch.Restart();
			}
			CAutoP<CWStringBase> a_pstr;
			a_pstr = pmdp->PstrObject(m_memory_pool, this, mdid);
			
			GPOS_ASSERT(NULL != a_pstr.Value());
			IMemoryPool *memory_pool = m_memory_pool;
			
			if (IMDId::EmdidGPDBCtas != mdid->Emdidt())
			{
				// create the accessor memory pool
				memory_pool = a_pmdcacc->Pmp();
			}

			pmdobjNew = gpdxl::CDXLUtils::ParseDXLToIMDIdCacheObj(memory_pool, a_pstr.Value(), NULL /* XSD path */);
			GPOS_ASSERT(NULL != pmdobjNew);

			if (fPrintOptStats)
			{
				// add fetch time in msec
				CDouble dFetch(timerFetch.ElapsedUS() / CDouble(GPOS_USEC_IN_MSEC));
				m_dFetchTime = CDouble(m_dFetchTime.Get() + dFetch.Get());
			}

			// For CTAS mdid, we avoid adding the corresponding object to the MD cache
			// since those objects have a fixed id, and if caching is enabled and those
			// objects are cached, then a subsequent CTAS query will attempt to use
			// the cached object, which has a different schema, resulting in a crash.
			// so for such objects, we bypass the MD cache, getting them from the
			// MD provider, directly to the local hash table

			if (IMDId::EmdidGPDBCtas != mdid->Emdidt())
			{
				// add to MD cache
				CAutoP<CMDKey> a_pmdkeyCache;
				// ref count of the new object is set to one and optimizer becomes its owner
				a_pmdkeyCache = GPOS_NEW(memory_pool) CMDKey(pmdobjNew->MDId());

				// object gets pinned independent of whether insertion succeeded or
				// failed because object was already in cache

#ifdef GPOS_DEBUG
				IMDCacheObject *pmdobjInserted =
#endif
				a_pmdcacc->Insert(a_pmdkeyCache.Value(), pmdobjNew);

				GPOS_ASSERT(NULL != pmdobjInserted);

				// safely inserted
				(void) a_pmdkeyCache.Reset();
			}
		}

		{
			// store in local hashtable
			GPOS_ASSERT(NULL != pmdobjNew);
			IMDId *pmdidNew = pmdobjNew->MDId();
			pmdidNew->AddRef();

			CAutoP<SMDAccessorElem> a_pmdaccelem;
			a_pmdaccelem = GPOS_NEW(m_memory_pool) SMDAccessorElem(pmdobjNew, pmdidNew);

			MDHTAccessor mdhtacc(m_shtCacheAccessors, a_pmdaccelem->MDId());

			if (NULL == mdhtacc.Find())
			{
				// object has not been inserted in the meantime
				mdhtacc.Insert(a_pmdaccelem.Value());

				// add deletion lock for mdid
				a_pmdaccelem->MDId()->AddDeletionLock();
				a_pmdaccelem.Reset();
			}
		}
	}
	
	// requested object must be in local hashtable already: retrieve it
	MDHTAccessor mdhtacc(m_shtCacheAccessors, mdid);
	SMDAccessorElem *pmdaccelem = mdhtacc.Find();
	
	GPOS_ASSERT(NULL != pmdaccelem);
	
	pimdobj = pmdaccelem->GetImdObj();
	GPOS_ASSERT(NULL != pimdobj);
	
	if (fPrintOptStats)
	{
		// add lookup time in msec
		CDouble dLookup(timerLookup.ElapsedUS() / CDouble(GPOS_USEC_IN_MSEC));
		m_dLookupTime = CDouble(m_dLookupTime.Get() + dLookup.Get());
	}

	return pimdobj;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::Pmdrel
//
//	@doc:
//		Retrieves a metadata cache relation from the md cache, possibly retrieving
//		it from the external metadata provider and storing it in the cache first.
//
//---------------------------------------------------------------------------
const IMDRelation *
CMDAccessor::Pmdrel
	(
	IMDId *mdid
	)
{
	const IMDCacheObject *pmdobj = GetImdObj(mdid);
	if (IMDCacheObject::EmdtRel != pmdobj->Emdt())
	{
		GPOS_RAISE(gpdxl::ExmaMD, gpdxl::ExmiMDCacheEntryNotFound, mdid->GetBuffer());
	}

	return dynamic_cast<const IMDRelation*>(pmdobj);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::Pmdtype
//
//	@doc:
//		Retrieves the metadata description for a type from the md cache, 
//		possibly retrieving it from the external metadata provider and storing 
//		it in the cache first.
//
//---------------------------------------------------------------------------
const IMDType *
CMDAccessor::Pmdtype
	(
	IMDId *mdid
	)
{	
	const IMDCacheObject *pmdobj = GetImdObj(mdid);
	if (IMDCacheObject::EmdtType != pmdobj->Emdt())
	{
		GPOS_RAISE(gpdxl::ExmaMD, gpdxl::ExmiMDCacheEntryNotFound, mdid->GetBuffer());
	}

	return dynamic_cast<const IMDType*>(pmdobj);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::Pmdtype
//
//	@doc:
//		Retrieves the MD type from the md cache given the type info and source
//		system id,  possibly retrieving it from the external metadata provider 
//		and storing it in the cache first.
//
//---------------------------------------------------------------------------
const IMDType *
CMDAccessor::Pmdtype
	(
	CSystemId sysid,
	IMDType::ETypeInfo eti
	)
{	
	GPOS_ASSERT(IMDType::EtiGeneric != eti);
	IMDProvider *pmdp = Pmdp(sysid);
	CAutoRef<IMDId> a_pmdid;
	a_pmdid = pmdp->MDId(m_memory_pool, sysid, eti);
	const IMDCacheObject *pmdobj = GetImdObj(a_pmdid.Value());
	if (IMDCacheObject::EmdtType != pmdobj->Emdt())
	{
		GPOS_RAISE(gpdxl::ExmaMD, gpdxl::ExmiMDCacheEntryNotFound, a_pmdid.Value()->GetBuffer());
	}

	return dynamic_cast<const IMDType*>(pmdobj);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::Pmdtype
//
//	@doc:
//		Retrieves the generic MD type from the md cache given the
//		type info,  possibly retrieving it from the external metadata provider 
//		and storing it in the cache first.
//
//---------------------------------------------------------------------------
const IMDType *
CMDAccessor::Pmdtype
	(
	IMDType::ETypeInfo eti
	)
{	
	GPOS_ASSERT(IMDType::EtiGeneric != eti);

	IMDId *mdid = m_pmdpGeneric->MDId(eti);
	GPOS_ASSERT(NULL != mdid);
	const IMDCacheObject *pmdobj = GetImdObj(mdid);
	
	if (IMDCacheObject::EmdtType != pmdobj->Emdt())
	{
		GPOS_RAISE(gpdxl::ExmaMD, gpdxl::ExmiMDCacheEntryNotFound, mdid->GetBuffer());
	}

	return dynamic_cast<const IMDType*>(pmdobj);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::Pmdscop
//
//	@doc:
//		Retrieves the metadata description for a scalar operator from the md cache, 
//		possibly retrieving it from the external metadata provider and storing 
//		it in the cache first.
//
//---------------------------------------------------------------------------
const IMDScalarOp *
CMDAccessor::Pmdscop
	(
	IMDId *mdid
	)
{	
	const IMDCacheObject *pmdobj = GetImdObj(mdid);
	if (IMDCacheObject::EmdtOp != pmdobj->Emdt())
	{
		GPOS_RAISE(gpdxl::ExmaMD, gpdxl::ExmiMDCacheEntryNotFound, mdid->GetBuffer());
	}

	return dynamic_cast<const IMDScalarOp*>(pmdobj);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::Pmdfunc
//
//	@doc:
//		Retrieves the metadata description for a function from the md cache, 
//		possibly retrieving it from the external metadata provider and storing 
//		it in the cache first.
//
//---------------------------------------------------------------------------
const IMDFunction *
CMDAccessor::Pmdfunc
	(
	IMDId *mdid
	)
{	
	const IMDCacheObject *pmdobj = GetImdObj(mdid);
	if (IMDCacheObject::EmdtFunc != pmdobj->Emdt())
	{
		GPOS_RAISE(gpdxl::ExmaMD, gpdxl::ExmiMDCacheEntryNotFound, mdid->GetBuffer());
	}

	return dynamic_cast<const IMDFunction*>(pmdobj);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::FaggWindowFunc
//
//	@doc:
//		Check if the retrieved the window function metadata description from 
//		the md cache is an aggregate window function. Internally this function
//		may retrieve it from the external metadata provider and storing
//		it in the cache.
//
//---------------------------------------------------------------------------
BOOL
CMDAccessor::FAggWindowFunc
	(
	IMDId *mdid
	)
{
	const IMDCacheObject *pmdobj = GetImdObj(mdid);

	return (IMDCacheObject::EmdtAgg == pmdobj->Emdt());
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::Pmdagg
//
//	@doc:
//		Retrieves the metadata description for an aggregate from the md cache, 
//		possibly retrieving it from the external metadata provider and storing 
//		it in the cache first.
//
//---------------------------------------------------------------------------
const IMDAggregate *
CMDAccessor::Pmdagg
	(
	IMDId *mdid
	)
{	
	const IMDCacheObject *pmdobj = GetImdObj(mdid);
	if (IMDCacheObject::EmdtAgg != pmdobj->Emdt())
	{
		GPOS_RAISE(gpdxl::ExmaMD, gpdxl::ExmiMDCacheEntryNotFound, mdid->GetBuffer());
	}

	return dynamic_cast<const IMDAggregate*>(pmdobj);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::Pmdtrigger
//
//	@doc:
//		Retrieves the metadata description for a trigger from the md cache,
//		possibly retrieving it from the external metadata provider and storing
//		it in the cache first.
//
//---------------------------------------------------------------------------
const IMDTrigger *
CMDAccessor::Pmdtrigger
	(
	IMDId *mdid
	)
{
	const IMDCacheObject *pmdobj = GetImdObj(mdid);
	if (IMDCacheObject::EmdtTrigger != pmdobj->Emdt())
	{
		GPOS_RAISE(gpdxl::ExmaMD, gpdxl::ExmiMDCacheEntryNotFound, mdid->GetBuffer());
	}

	return dynamic_cast<const IMDTrigger*>(pmdobj);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::Pmdindex
//
//	@doc:
//		Retrieves the metadata description for an index from the md cache, 
//		possibly retrieving it from the external metadata provider and storing 
//		it in the cache first.
//
//---------------------------------------------------------------------------
const IMDIndex *
CMDAccessor::Pmdindex
	(
	IMDId *mdid
	)
{	
	const IMDCacheObject *pmdobj = GetImdObj(mdid);
	if (IMDCacheObject::EmdtInd != pmdobj->Emdt())
	{
		GPOS_RAISE(gpdxl::ExmaMD, gpdxl::ExmiMDCacheEntryNotFound, mdid->GetBuffer());
	}

	return dynamic_cast<const IMDIndex*>(pmdobj);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::Pmdcheckconstraint
//
//	@doc:
//		Retrieves the metadata description for a check constraint from the md cache,
//		possibly retrieving it from the external metadata provider and storing
//		it in the cache first.
//
//---------------------------------------------------------------------------
const IMDCheckConstraint *
CMDAccessor::Pmdcheckconstraint
	(
	IMDId *mdid
	)
{
	const IMDCacheObject *pmdobj = GetImdObj(mdid);
	if (IMDCacheObject::EmdtCheckConstraint != pmdobj->Emdt())
	{
		GPOS_RAISE(gpdxl::ExmaMD, gpdxl::ExmiMDCacheEntryNotFound, mdid->GetBuffer());
	}

	return dynamic_cast<const IMDCheckConstraint*>(pmdobj);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::Pmdcolstats
//
//	@doc:
//		Retrieves column statistics from the md cache, possibly retrieving it  
//		from the external metadata provider and storing it in the cache first.
//
//---------------------------------------------------------------------------
const IMDColStats *
CMDAccessor::Pmdcolstats
	(
	IMDId *mdid
	)
{	
	const IMDCacheObject *pmdobj = GetImdObj(mdid);
	if (IMDCacheObject::EmdtColStats != pmdobj->Emdt())
	{
		GPOS_RAISE(gpdxl::ExmaMD, gpdxl::ExmiMDCacheEntryNotFound, mdid->GetBuffer());
	}

	return dynamic_cast<const IMDColStats*>(pmdobj);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::Pmdrelstats
//
//	@doc:
//		Retrieves relation statistics from the md cache, possibly retrieving it  
//		from the external metadata provider and storing it in the cache first.
//
//---------------------------------------------------------------------------
const IMDRelStats *
CMDAccessor::Pmdrelstats
	(
	IMDId *mdid
	)
{	
	const IMDCacheObject *pmdobj = GetImdObj(mdid);
	if (IMDCacheObject::EmdtRelStats != pmdobj->Emdt())
	{
		GPOS_RAISE(gpdxl::ExmaMD, gpdxl::ExmiMDCacheEntryNotFound, mdid->GetBuffer());
	}

	return dynamic_cast<const IMDRelStats*>(pmdobj);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::Pmdcast
//
//	@doc:
//		Retrieve cast object between given source and destination types
//
//---------------------------------------------------------------------------
const IMDCast *
CMDAccessor::Pmdcast
	(
	IMDId *mdid_src,
	IMDId *mdid_dest
	)
{	
	GPOS_ASSERT(NULL != mdid_src);
	GPOS_ASSERT(NULL != mdid_dest);
	
	mdid_src->AddRef();
	mdid_dest->AddRef();
	
	CAutoP<IMDId> a_pmdidCast;
	a_pmdidCast = GPOS_NEW(m_memory_pool) CMDIdCast(CMDIdGPDB::PmdidConvert(mdid_src), CMDIdGPDB::PmdidConvert(mdid_dest));
	
	const IMDCacheObject *pmdobj = GetImdObj(a_pmdidCast.Value());
		
	if (IMDCacheObject::EmdtCastFunc != pmdobj->Emdt())
	{
		GPOS_RAISE(gpdxl::ExmaMD, gpdxl::ExmiMDCacheEntryNotFound, a_pmdidCast->GetBuffer());
	}
	a_pmdidCast.Reset()->Release();

	return dynamic_cast<const IMDCast*>(pmdobj);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::Pmdsccmp
//
//	@doc:
//		Retrieve scalar comparison object between given types
//
//---------------------------------------------------------------------------
const IMDScCmp *
CMDAccessor::Pmdsccmp
	(
	IMDId *pmdidLeft,
	IMDId *pmdidRight,
	IMDType::ECmpType ecmpt
	)
{	
	GPOS_ASSERT(NULL != pmdidLeft);
	GPOS_ASSERT(NULL != pmdidLeft);
	GPOS_ASSERT(IMDType::EcmptOther > ecmpt);
	
	pmdidLeft->AddRef();
	pmdidRight->AddRef();
	
	CAutoP<IMDId> a_pmdidScCmp;
	a_pmdidScCmp = GPOS_NEW(m_memory_pool) CMDIdScCmp(CMDIdGPDB::PmdidConvert(pmdidLeft), CMDIdGPDB::PmdidConvert(pmdidRight), ecmpt);
	
	const IMDCacheObject *pmdobj = GetImdObj(a_pmdidScCmp.Value());
		
	if (IMDCacheObject::EmdtScCmp != pmdobj->Emdt())
	{
		GPOS_RAISE(gpdxl::ExmaMD, gpdxl::ExmiMDCacheEntryNotFound, a_pmdidScCmp->GetBuffer());
	}
	a_pmdidScCmp.Reset()->Release();

	return dynamic_cast<const IMDScCmp*>(pmdobj);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::ExtractColumnHistWidth
//
//	@doc:
//		Record histogram and width information for a given column of a table
//
//---------------------------------------------------------------------------
void
CMDAccessor::RecordColumnStats
	(
	IMemoryPool *memory_pool,
	IMDId *rel_mdid,
	ULONG col_id,
	ULONG ulPos,
	BOOL fSystemCol,
	BOOL fEmptyTable,
	HMUlHist *phmulhist,
	HMUlDouble *phmuldoubleWidth,
	CStatisticsConfig *pstatsconf
	)
{
	GPOS_ASSERT(NULL != rel_mdid);
	GPOS_ASSERT(NULL != phmulhist);
	GPOS_ASSERT(NULL != phmuldoubleWidth);

	// get the column statistics
	const IMDColStats *pmdcolstats = Pmdcolstats(memory_pool, rel_mdid, ulPos);
	GPOS_ASSERT(NULL != pmdcolstats);

	// fetch the column width and insert it into the hashmap
	CDouble *pdWidth = GPOS_NEW(memory_pool) CDouble(pmdcolstats->Width());
	phmuldoubleWidth->Insert(GPOS_NEW(memory_pool) ULONG(col_id), pdWidth);

	// extract the the histogram and insert it into the hashmap
	const IMDRelation *pmdrel = Pmdrel(rel_mdid);
	IMDId *mdid_type = pmdrel->GetMdCol(ulPos)->MDIdType();
	CHistogram *phist = Phist(memory_pool, mdid_type, pmdcolstats);
	GPOS_ASSERT(NULL != phist);
	phmulhist->Insert(GPOS_NEW(memory_pool) ULONG(col_id), phist);

	BOOL fGuc = GPOS_FTRACE(EopttracePrintColsWithMissingStats);
	BOOL fRecordMissingStats = !fEmptyTable && fGuc && !fSystemCol
								&& (NULL != pstatsconf) && phist->FColStatsMissing();
	if (fRecordMissingStats)
	{
		// record the columns with missing (dummy) statistics information
		rel_mdid->AddRef();
		CMDIdColStats *pmdidCol = GPOS_NEW(memory_pool) CMDIdColStats
												(
												CMDIdGPDB::PmdidConvert(rel_mdid),
												ulPos
												);
		pstatsconf->AddMissingStatsColumn(pmdidCol);
		pmdidCol->Release();
	}
}


// Return the column statistics meta data object for a given column of a table
const IMDColStats *
CMDAccessor::Pmdcolstats
	(
	IMemoryPool *memory_pool,
	IMDId *rel_mdid,
	ULONG ulPos
	)
{
	rel_mdid->AddRef();
	CMDIdColStats *mdid_col_stats = GPOS_NEW(memory_pool) CMDIdColStats(CMDIdGPDB::PmdidConvert(rel_mdid), ulPos);
	const IMDColStats *pmdcolstats = Pmdcolstats(mdid_col_stats);
	mdid_col_stats->Release();

	return pmdcolstats;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::Pstats
//
//	@doc:
//		Construct a statistics object for the columns of the given relation
//
//---------------------------------------------------------------------------
IStatistics *
CMDAccessor::Pstats
	(
	IMemoryPool *memory_pool,
	IMDId *rel_mdid,
	CColRefSet *pcrsHist,
	CColRefSet *pcrsWidth,
	CStatisticsConfig *pstatsconf
	)
{
	GPOS_ASSERT(NULL != rel_mdid);
	GPOS_ASSERT(NULL != pcrsHist);
	GPOS_ASSERT(NULL != pcrsWidth);

	// retrieve MD relation and MD relation stats objects
	rel_mdid->AddRef();
	CMDIdRelStats *pmdidRelStats = GPOS_NEW(memory_pool) CMDIdRelStats(CMDIdGPDB::PmdidConvert(rel_mdid));
	const IMDRelStats *pmdRelStats = Pmdrelstats(pmdidRelStats);
	pmdidRelStats->Release();

	BOOL fEmptyTable = pmdRelStats->IsEmpty();
	const IMDRelation *pmdrel = Pmdrel(rel_mdid);

	HMUlHist *phmulhist = GPOS_NEW(memory_pool) HMUlHist(memory_pool);
	HMUlDouble *phmuldoubleWidth = GPOS_NEW(memory_pool) HMUlDouble(memory_pool);

	CColRefSetIter crsiHist(*pcrsHist);
	while (crsiHist.Advance())
	{
		CColRef *pcrHist = crsiHist.Pcr();

		// colref must be one of the base table
		CColRefTable *pcrtable = CColRefTable::PcrConvert(pcrHist);

		// extract the column identifier, position of the attribute in the system catalog
		ULONG col_id = pcrtable->Id();
		INT attno = pcrtable->AttrNum();
		ULONG ulPos = pmdrel->UlPosFromAttno(attno);

		RecordColumnStats
			(
			memory_pool,
			rel_mdid,
			col_id,
			ulPos,
			pcrtable->FSystemCol(),
			fEmptyTable,
			phmulhist,
			phmuldoubleWidth,
			pstatsconf
			);
	}

	// extract column widths
	CColRefSetIter crsiWidth(*pcrsWidth);

	while (crsiWidth.Advance())
	{
		CColRef *pcrWidth = crsiWidth.Pcr();

		// colref must be one of the base table
		CColRefTable *pcrtable = CColRefTable::PcrConvert(pcrWidth);

		// extract the column identifier, position of the attribute in the system catalog
		ULONG col_id = pcrtable->Id();
		INT attno = pcrtable->AttrNum();
		ULONG ulPos = pmdrel->UlPosFromAttno(attno);

		CDouble *pdWidth = GPOS_NEW(memory_pool) CDouble(pmdrel->DColWidth(ulPos));
		phmuldoubleWidth->Insert(GPOS_NEW(memory_pool) ULONG(col_id), pdWidth);
	}

	CDouble rows = std::max(DOUBLE(1.0), pmdRelStats->Rows().Get());

	return GPOS_NEW(memory_pool) CStatistics
							(
							memory_pool,
							phmulhist,
							phmuldoubleWidth,
							rows,
							fEmptyTable
							);
}


//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::Phist
//
//	@doc:
//		Construct a histogram from the given MD column stats object
//
//---------------------------------------------------------------------------
CHistogram *
CMDAccessor::Phist
	(
	IMemoryPool *memory_pool,
	IMDId *mdid_type,
	const IMDColStats *pmdcolstats
	)
{
	GPOS_ASSERT(NULL != mdid_type);
	GPOS_ASSERT(NULL != pmdcolstats);

	BOOL is_col_stats_missing = pmdcolstats->FColStatsMissing();
	const ULONG num_of_buckets = pmdcolstats->UlBuckets();
	BOOL fBoolType = CMDAccessorUtils::FBoolType(this, mdid_type);
	if (is_col_stats_missing && fBoolType)
	{
		GPOS_ASSERT(0 == num_of_buckets);

		return CHistogram::PhistDefaultBoolColStats(memory_pool);
	}

	DrgPbucket *pdrgpbucket = GPOS_NEW(memory_pool) DrgPbucket(memory_pool);
	for (ULONG ul = 0; ul < num_of_buckets; ul++)
	{
		const CDXLBucket *pdxlbucket = pmdcolstats->GetBucketDXL(ul);
		CBucket *pbucket = Pbucket(memory_pool, mdid_type, pdxlbucket);
		pdrgpbucket->Append(pbucket);
	}

	CDouble null_freq = pmdcolstats->DNullFreq();
	CDouble distinct_remaining = pmdcolstats->DDistinctRemain();
	CDouble freq_remaining = pmdcolstats->DFreqRemain();

	CHistogram *phist = GPOS_NEW(memory_pool) CHistogram
									(
									pdrgpbucket,
									true /*fWellDefined*/,
									null_freq,
									distinct_remaining,
									freq_remaining,
									is_col_stats_missing
									);
	GPOS_ASSERT_IMP(fBoolType, 3 >= phist->DDistinct() - CStatistics::DEpsilon);

	return phist;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::Pbucket
//
//	@doc:
//		Construct a typed bucket from a DXL bucket
//
//---------------------------------------------------------------------------
CBucket *
CMDAccessor::Pbucket
	(
	IMemoryPool *memory_pool,
	IMDId *mdid_type,
	const CDXLBucket *pdxlbucket
	)
{
	IDatum *pdatumLower = Pdatum(memory_pool, mdid_type, pdxlbucket->PdxldatumLower());
	IDatum *pdatumUpper = Pdatum(memory_pool, mdid_type, pdxlbucket->PdxldatumUpper());
	
	CPoint *ppointLower = GPOS_NEW(memory_pool) CPoint(pdatumLower);
	CPoint *ppointUpper = GPOS_NEW(memory_pool) CPoint(pdatumUpper);
	
	return GPOS_NEW(memory_pool) CBucket
						(
						ppointLower,
						ppointUpper,
						pdxlbucket->FLowerClosed(),
						pdxlbucket->FUpperClosed(),
						pdxlbucket->DFrequency(),
						pdxlbucket->DDistinct()
						);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::Pdatum
//
//	@doc:
//		Construct a typed bucket from a DXL bucket
//
//---------------------------------------------------------------------------
IDatum *
CMDAccessor::Pdatum
	(
	IMemoryPool *memory_pool,
	IMDId *mdid_type,
	const CDXLDatum *datum_dxl
	)
{
	const IMDType *pmdtype = Pmdtype(mdid_type);
		
	return pmdtype->Pdatum(memory_pool, datum_dxl);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::Serialize
//
//	@doc:
//		Serialize MD object into provided stream
//
//---------------------------------------------------------------------------
void
CMDAccessor::Serialize
	(
	COstream& oos
	)
{
	ULONG nentries = m_shtCacheAccessors.Size();
	IMDCacheObject **cacheEntries;
	CAutoRg<IMDCacheObject*> aCacheEntries;
	ULONG ul;

	// Iterate the hash table and insert all entries to the array.
	// The iterator holds a lock on the hash table, so we must not
	// do anything non-trivial that might e.g. allocate memory,
	// while iterating.
	cacheEntries = GPOS_NEW_ARRAY(m_memory_pool, IMDCacheObject *, nentries);
	aCacheEntries = cacheEntries;
	{
		MDHTIter mdhtit(m_shtCacheAccessors);
		ul = 0;
		while (mdhtit.Advance())
		{
			MDHTIterAccessor mdhtitacc(mdhtit);
			SMDAccessorElem *pmdaccelem = mdhtitacc.Value();
			GPOS_ASSERT(NULL != pmdaccelem);
			cacheEntries[ul++] = pmdaccelem->GetImdObj();
		}
		GPOS_ASSERT(ul == nentries);
	}

	// Now that we're done iterating and no longer hold the lock,
	// serialize the entries.
	for (ul = 0; ul < nentries; ul++)
		oos << cacheEntries[ul]->Pstr()->GetBuffer();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessor::SerializeSysid
//
//	@doc:
//		Serialize the system ids into provided stream
//
//---------------------------------------------------------------------------
void
CMDAccessor::SerializeSysid
	(
	COstream &oos
	)
{
	ULONG ul = 0;
	MDPHTIter mdhtit(m_shtProviders);

	while (mdhtit.Advance())
	{
		MDPHTIterAccessor mdhtitacc(mdhtit);
		SMDProviderElem *pmdpelem = mdhtitacc.Value();
		CSystemId sysid = pmdpelem->Sysid();
		
		
		WCHAR wszSysId[GPDXL_MDID_LENGTH];
		CWStringStatic str(wszSysId, GPOS_ARRAY_SIZE(wszSysId));
		
		if (0 < ul)
		{
			str.AppendFormat(GPOS_WSZ_LIT("%s"), ",");
		}
		
		str.AppendFormat(GPOS_WSZ_LIT("%d.%ls"), sysid.Emdidt(), sysid.GetBuffer());

		oos << str.GetBuffer();
		ul++;
	}
}


// EOF
