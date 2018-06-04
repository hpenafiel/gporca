//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerPhysicalSplit.h
//
//	@doc:
//		Parse handler for parsing a physical split operator
//
//---------------------------------------------------------------------------
#ifndef GPDXL_CParseHandlerPhysicalSplit_H
#define GPDXL_CParseHandlerPhysicalSplit_H

#include "gpos/base.h"
#include "naucrates/dxl/parser/CParseHandlerPhysicalOp.h"


namespace gpdxl
{
	using namespace gpos;

	XERCES_CPP_NAMESPACE_USE

	//---------------------------------------------------------------------------
	//	@class:
	//		CParseHandlerPhysicalSplit
	//
	//	@doc:
	//		Parse handler for parsing a physical split operator
	//
	//---------------------------------------------------------------------------
	class CParseHandlerPhysicalSplit : public CParseHandlerPhysicalOp
	{
		private:

			// deletion col ids
			ULongPtrArray *m_deletion_colid_array;

			// insertion col ids
			ULongPtrArray *m_pdrgpulInsert;

			// action column id
			ULONG m_ulAction;

			// ctid column id
			ULONG m_ctid_colid;

			// segmentId column id
			ULONG m_segid_colid;

			// does update preserve oids
			BOOL m_fPreserveOids;
			
			// tuple oid column id
			ULONG m_ulTupleOidColId;
			
			// private copy ctor
			CParseHandlerPhysicalSplit(const CParseHandlerPhysicalSplit &);

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
			CParseHandlerPhysicalSplit
				(
				IMemoryPool *memory_pool,
				CParseHandlerManager *parse_handler_mgr,
				CParseHandlerBase *parse_handler_root
				);
	};
}

#endif // !GPDXL_CParseHandlerPhysicalSplit_H

// EOF
