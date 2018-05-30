//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerMDRequest.h
//
//	@doc:
//		SAX parse handler class for metadata requests
//---------------------------------------------------------------------------

#ifndef GPDXL_CParseHandlerMDRequest_H
#define GPDXL_CParseHandlerMDRequest_H

#include "gpos/base.h"

#include "naucrates/dxl/parser/CParseHandlerBase.h"

#include "naucrates/md/CMDRequest.h"

namespace gpdxl
{
	using namespace gpos;
	using namespace gpmd;

	XERCES_CPP_NAMESPACE_USE
	
	//---------------------------------------------------------------------------
	//	@class:
	//		CParseHandlerMDRequest
	//
	//	@doc:
	//		Parse handler for relation metadata
	//
	//---------------------------------------------------------------------------
	class CParseHandlerMDRequest : public CParseHandlerBase
	{
		private:
		
			// array of metadata ids
			DrgPmdid *m_pdrgpmdid;
			
			// array of type requests
			CMDRequest::DrgPtr *m_pdrgptr;

			// private copy ctor
			CParseHandlerMDRequest(const CParseHandlerMDRequest &);
			
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
			
		public:
			// ctor
			CParseHandlerMDRequest(IMemoryPool *pmp, CParseHandlerManager *parse_handler_mgr, CParseHandlerBase *pph);
			
			// dtor
			virtual
			~CParseHandlerMDRequest();
			
			// parse handler type
			virtual 
			EDxlParseHandlerType Edxlphtype() const;

			// parsed mdids
			DrgPmdid *Pdrgpmdid() const;	
			
			// parsed type requests
			CMDRequest::DrgPtr *Pdrgptr() const;	
	};
}

#endif // !GPDXL_CParseHandlerMDRequest_H

// EOF
