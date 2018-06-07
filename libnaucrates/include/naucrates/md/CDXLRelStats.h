//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLRelStats.h
//
//	@doc:
//		Class representing relation stats
//---------------------------------------------------------------------------



#ifndef GPMD_CDXLRelStats_H
#define GPMD_CDXLRelStats_H

#include "gpos/base.h"
#include "gpos/common/CDouble.h"

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/IMDRelStats.h"
#include "naucrates/md/CMDIdRelStats.h"

namespace gpdxl
{
	class CXMLSerializer;
}

namespace gpmd
{
	using namespace gpos;
	using namespace gpdxl;

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLRelStats
	//
	//	@doc:
	//		Class representing relation stats
	//
	//---------------------------------------------------------------------------
	class CDXLRelStats : public IMDRelStats
	{		
		private:
		
			// memory pool
			IMemoryPool *m_memory_pool;

			// metadata id of the object
			CMDIdRelStats *m_pmdidRelStats;
			
			// table name
			CMDName *m_mdname;
			
			// number of rows
			CDouble m_rows;
			
			// flag to indicate if input relation is empty
			BOOL m_empty;

			// DXL string for object
			CWStringDynamic *m_pstr;
			
			// private copy ctor
			CDXLRelStats(const CDXLRelStats &);
		
		public:
			
			CDXLRelStats
				(
				IMemoryPool *memory_pool,
				CMDIdRelStats *pmdidRelStats,
				CMDName *mdname,
				CDouble rows,
				BOOL fEmpty
				);
			
			virtual
			~CDXLRelStats();
			
			// the metadata id
			virtual 
			IMDId *MDId() const;
			
			// relation name
			virtual 
			CMDName Mdname() const;
			
			// DXL string representation of cache object 
			virtual 
			const CWStringDynamic *Pstr() const;
			
			// number of rows
			virtual
			CDouble Rows() const;
			
			// is statistics on an empty input
			virtual
			BOOL IsEmpty() const
			{
				return m_empty;
			}

			// serialize relation stats in DXL format given a serializer object
			virtual 
			void Serialize(gpdxl::CXMLSerializer *) const;

#ifdef GPOS_DEBUG
			// debug print of the metadata relation
			virtual 
			void DebugPrint(IOstream &os) const;
#endif

			// dummy relstats
			static
			CDXLRelStats *PdxlrelstatsDummy
								(
								IMemoryPool *memory_pool,
								IMDId *pmdid
								);
	};

}



#endif // !GPMD_CDXLRelStats_H

// EOF
