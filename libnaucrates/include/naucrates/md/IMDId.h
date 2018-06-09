//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		IMDId.h
//
//	@doc:
//		Abstract class for representing metadata object ids
//---------------------------------------------------------------------------



#ifndef GPMD_IMDId_H
#define GPMD_IMDId_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"
#include "gpos/common/CHashSet.h"
#include "gpos/common/CHashSetIter.h"
#include "gpos/string/CWStringConst.h"

#include "naucrates/dxl/gpdb_types.h"

#define GPDXL_MDID_LENGTH 100

// fwd decl
namespace gpdxl
{
	class CXMLSerializer;
}

namespace gpmd
{
	using namespace gpos;
	using namespace gpdxl;

	// fwd decl
	class CSystemId;

	static const INT IDefaultTypeModifier = -1;
	
	//---------------------------------------------------------------------------
	//	@class:
	//		IMDId
	//
	//	@doc:
	//		Abstract class for representing metadata objects ids
	//
	//---------------------------------------------------------------------------
	class IMDId : public CRefCount
	{
		
		private:

			// number of deletion locks -- each MDAccessor adds a new deletion lock if it uses
			// an MDId object in its internal hash-table, the deletion lock is released when
			// MDAccessor is destroyed
			volatile ULONG_PTR m_ulpDeletionLocks;

		public:
			//------------------------------------------------------------------
			//	@doc:
			//		Type of md id.
			//		The exact values are important when parsing mdids from DXL and
			//		should not be modified without modifying the parser
			//
			//------------------------------------------------------------------
			enum EMDIdType
			{
				EmdidGPDB = 0,
				EmdidColStats = 1,
				EmdidRelStats = 2,
				EmdidCastFunc = 3,
				EmdidScCmp = 4,
				EmdidGPDBCtas = 5,
				EmdidSentinel
			};
			
			// ctor
			IMDId()
				:
				m_ulpDeletionLocks(0)
			{}

			// dtor
			virtual 
			~IMDId(){};
			
			// type of mdid
			virtual
			EMDIdType Emdidt() const = 0;
			
			// string representation of mdid
			virtual
			const WCHAR *GetBuffer() const = 0;
			
			// system id
			virtual
			CSystemId Sysid() const = 0;
			
			
			// equality check
			virtual 
			BOOL Equals(const IMDId *mdid) const = 0;
					
			// computes the hash value for the metadata id
			virtual
			ULONG HashValue() const = 0;
			
			// return true if calling object's destructor is allowed
			virtual
			BOOL Deletable() const
			{
				return (0 == m_ulpDeletionLocks);
			}

			// increase number of deletion locks
			void AddDeletionLock()
			{
				(void) ExchangeAddUlongPtrWithInt(&m_ulpDeletionLocks, 1);
			}

			// decrease number of deletion locks
			void RemoveDeletionLock()
			{
				GPOS_ASSERT(0 < m_ulpDeletionLocks);

				(void) ExchangeAddUlongPtrWithInt(&m_ulpDeletionLocks, -1);
			}

			// return number of deletion locks
			ULONG_PTR UlpDeletionLocks() const
			{
				return m_ulpDeletionLocks;
			}

			// static hash functions for use in different indexing structures,
			// e.g. hashmaps, MD cache, etc.
			static
			ULONG UlHashMDId
				(
				const IMDId *mdid
				)
			{
				GPOS_ASSERT(NULL != mdid);
				return mdid->HashValue();
			}

			// hash function for using mdids in a cache
			static
			ULONG UlHashMDid
				(
				const VOID_PTR & pv
				)
			{
				GPOS_ASSERT(NULL != pv);
				IMDId *mdid = static_cast<IMDId *> (pv);
				return mdid->HashValue();
			}
			
			// static equality functions for use in different structures, 
			// e.g. hashmaps, MD cache, etc.
			static 
			BOOL FEqualMDId
				(
				const IMDId *left_mdid,
				const IMDId *right_mdid
				)
			{
				GPOS_ASSERT(NULL != left_mdid && NULL != right_mdid);
				return left_mdid->Equals(right_mdid);
			}
			
			// equality function for using mdids in a cache
			static BOOL
			FEqualMDid
				(
				const VOID_PTR &pvLeft,
				const VOID_PTR &pvRight
				)
			{
				if (NULL == pvLeft && NULL == pvRight)
				{
					return true;
				}
				
				if (NULL == pvLeft || NULL == pvRight)
				{
					return false;
				}
				
			
				IMDId *left_mdid = static_cast<IMDId *> (pvLeft);
				IMDId *right_mdid = static_cast<IMDId *> (pvRight);
				return left_mdid->Equals(right_mdid);

			}
			
			// is the mdid valid
			virtual
			BOOL IsValid() const = 0;
			
			// serialize mdid in DXL as the value for the specified attribute 
			virtual
			void Serialize(CXMLSerializer *xml_serializer, const CWStringConst *pstrAttribute) const = 0;

			// debug print of the metadata id
			virtual
			IOstream &OsPrint(IOstream &os) const = 0;
			
			// safe validity function
			static
			BOOL IsValid(const IMDId *mdid)
			{
				return NULL != mdid && mdid->IsValid();
			}
	};
	
	// common structures over metadata id elements
	typedef CDynamicPtrArray<IMDId, CleanupRelease> DrgPmdid;

    // hash set for mdid
    typedef CHashSet<IMDId, IMDId::UlHashMDId, IMDId::FEqualMDId, CleanupRelease<IMDId> > HSMDId;

    // iterator over the hash set for column id information for missing statistics
    typedef CHashSetIter<IMDId, IMDId::UlHashMDId, IMDId::FEqualMDId, CleanupRelease<IMDId> > HSIterMDId;
}



#endif // !GPMD_IMDId_H

// EOF
