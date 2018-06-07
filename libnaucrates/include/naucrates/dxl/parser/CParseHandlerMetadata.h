//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerMetadata.h
//
//	@doc:
//		SAX parse handler class for parsing metadata from a DXL document.
//---------------------------------------------------------------------------

#ifndef GPDXL_CParseHandlerMetadata_H
#define GPDXL_CParseHandlerMetadata_H

#include "gpos/base.h"
#include "naucrates/dxl/parser/CParseHandlerBase.h"

#include "naucrates/md/IMDCacheObject.h"

#include "naucrates/dxl/xml/dxltokens.h"

namespace gpdxl
{
	using namespace gpos;
	using namespace gpmd;
	using namespace gpnaucrates;

	XERCES_CPP_NAMESPACE_USE
	
	
	//---------------------------------------------------------------------------
	//	@class:
	//		CParseHandlerMetadata
	//
	//	@doc:
	//		Parse handler for metadata.
	//
	//---------------------------------------------------------------------------
	class CParseHandlerMetadata : public CParseHandlerBase
	{
		private:
			
			// list of parsed metadata objects
			DrgPimdobj *m_mdid_cached_obj_array;
			
			// list of parsed mdids
			DrgPmdid *m_mdid_array;

			// list of parsed metatadata source system ids
			DrgPsysid *m_system_id_array;

			// private copy ctor
			CParseHandlerMetadata(const CParseHandlerMetadata&);
			
			// process the start of an element
			void StartElement
				(
					const XMLCh* const element_uri, 		// URI of element's namespace
 					const XMLCh* const element_local_name,	// local part of element's name
					const XMLCh* const element_qname,		// element's qname
					const Attributes& attr				// element's attributes
				);

			// process the end of an element
			void EndElement
				(
					const XMLCh* const element_uri, 		// URI of element's namespace
					const XMLCh* const element_local_name,	// local part of element's name
					const XMLCh* const element_qname		// element's qname
				);
			
			// parse an array of system ids from the XML attributes
			DrgPsysid *GetSrcSysIdArray
						(	
						const Attributes &attr,
						Edxltoken edxltokenAttr,
						Edxltoken edxltokenElement
						);

			
		public:
			// ctor
			CParseHandlerMetadata(IMemoryPool *memory_pool, CParseHandlerManager *parse_handler_mgr, CParseHandlerBase *parse_handler_root);
			
			// dtor
			virtual
			~CParseHandlerMetadata();
			
			// parse hander type
			virtual
			EDxlParseHandlerType GetParseHandlerType() const;
			
			// return the list of parsed metadata objects
			DrgPimdobj *GetMdIdCachedObjArray();
			
			// return the list of parsed mdids
			DrgPmdid *GetMdIdArray();
			
			// return the list of parsed system ids
			DrgPsysid *GetSystemIdArray();

	};
}

#endif // !GPDXL_CParseHandlerMetadata_H

// EOF
