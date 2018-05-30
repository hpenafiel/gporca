//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerLogicalUpdate.h
//
//	@doc:
//		Parse handler for parsing a logical update operator
//
//---------------------------------------------------------------------------
#ifndef GPDXL_CParseHandlerLogicalUpdate_H
#define GPDXL_CParseHandlerLogicalUpdate_H

#include "gpos/base.h"
#include "naucrates/dxl/parser/CParseHandlerLogicalOp.h"


namespace gpdxl
{
	using namespace gpos;

	XERCES_CPP_NAMESPACE_USE

	//---------------------------------------------------------------------------
	//	@class:
	//		CParseHandlerLogicalUpdate
	//
	//	@doc:
	//		Parse handler for parsing a logical update operator
	//
	//---------------------------------------------------------------------------
	class CParseHandlerLogicalUpdate : public CParseHandlerLogicalOp
	{
		private:

			// ctid column id
			ULONG m_ulCtid;

			// segmentId column id
			ULONG m_ulSegmentId;

			// delete col ids
			ULongPtrArray *m_pdrgpulDelete;

			// insert col ids
			ULongPtrArray *m_pdrgpulInsert;
			
			// does update preserve oids
			BOOL m_fPreserveOids;
			
			// tuple oid column id
			ULONG m_ulTupleOidColId;

			// private copy ctor
			CParseHandlerLogicalUpdate(const CParseHandlerLogicalUpdate &);

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
			CParseHandlerLogicalUpdate
				(
				IMemoryPool *pmp,
				CParseHandlerManager *parse_handler_mgr,
				CParseHandlerBase *pphRoot
				);
	};
}

#endif // !GPDXL_CParseHandlerLogicalUpdate_H

// EOF
