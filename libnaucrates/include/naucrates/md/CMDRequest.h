//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CMDRequest.h
//
//	@doc:
//		Class for a representing MD requests
//---------------------------------------------------------------------------



#ifndef GPMD_CMDRequest_H
#define GPMD_CMDRequest_H

#include "gpos/base.h"
#include "gpos/common/CRefCount.h"
#include "gpos/common/CDynamicPtrArray.h"

#include "naucrates/md/CSystemId.h"
#include "naucrates/md/IMDType.h"
#include "naucrates/md/IMDFunction.h"

namespace gpmd
{
	using namespace gpos;

	
	//--------------------------------------------------------------------------
	//	@class:
	//		CMDRequest
	//
	//	@doc:
	//		Class for representing MD requests
	//
	//--------------------------------------------------------------------------
	class CMDRequest : public CRefCount
	{
		
		public:
		
			// fwd decl
			struct SMDTypeRequest;
			struct SMDFuncRequest;
			
			// array of type requests
			typedef CDynamicPtrArray<SMDTypeRequest, CleanupDelete> DrgPtr;
			
			//---------------------------------------------------------------------------
			//	@class:
			//		SMDTypeRequest
			//
			//	@doc:
			//		Struct for representing requests for types metadata
			//
			//---------------------------------------------------------------------------
			struct SMDTypeRequest
			{
	
				// system id
				CSystemId m_sysid;
				
				// type info
				IMDType::ETypeInfo m_eti;
				
				// ctor
				SMDTypeRequest
					(
					CSystemId sysid,
					IMDType::ETypeInfo eti
					)
					:
					m_sysid(sysid),
					m_eti(eti)
				{}
	
			};
			
		private:
			
			// memory pool
			IMemoryPool *m_memory_pool;
			
			// array of mdids
			DrgPmdid *m_mdid_array;
			
			// type info requests
			DrgPtr *m_pdrgptr;
			
			// serialize system id
			CWStringDynamic *Pstr(CSystemId sysid);

			// private copy ctor
			CMDRequest(const CMDRequest &);
			
		public:
			
			// ctor
			CMDRequest(IMemoryPool *memory_pool, DrgPmdid *mdid_array, DrgPtr *pdrgptr);
			
			// ctor: type request only
			CMDRequest(IMemoryPool *memory_pool, SMDTypeRequest *pmdtr);
			
			// dtor
			virtual
			~CMDRequest();
			
			// accessors
			
			// array of mdids
			DrgPmdid *GetMdIdArray() const
			{
				return m_mdid_array;
			}
			
			// array of type info requests
			DrgPtr *Pdrgptr() const
			{
				return m_pdrgptr;
			}

			// serialize request in DXL format
			virtual
			void Serialize(gpdxl::CXMLSerializer *xml_serializer);
				
	};
}

#endif // !GPMD_CMDRequest_H

// EOF
