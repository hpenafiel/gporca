//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerPhysicalDML.h
//
//	@doc:
//		Parse handler for parsing a physical DML operator
//		
//---------------------------------------------------------------------------
#ifndef GPDXL_CParseHandlerPhysicalDML_H
#define GPDXL_CParseHandlerPhysicalDML_H

#include "gpos/base.h"
#include "naucrates/dxl/parser/CParseHandlerPhysicalOp.h"


namespace gpdxl
{
	using namespace gpos;

	XERCES_CPP_NAMESPACE_USE

	//---------------------------------------------------------------------------
	//	@class:
	//		CParseHandlerPhysicalDML
	//
	//	@doc:
	//		Parse handler for parsing a physical DML operator
	//
	//---------------------------------------------------------------------------
	class CParseHandlerPhysicalDML : public CParseHandlerPhysicalOp
	{
		private:
			
			// operator type
			EdxlDmlType m_edxldmltype;

			// source col ids
			ULongPtrArray *m_pdrgpul;
		
			// action column id
			ULONG m_ulAction;

			// oid column id
			ULONG m_ulOid;

			// ctid column id
			ULONG m_ctid_colid;

			// segmentId column id
			ULONG m_segid_colid;
			
			// does update preserve oids
			BOOL m_fPreserveOids;
			
			// tuple oid column id
			ULONG m_ulTupleOidColId;

			// needs data to be sorted
			BOOL m_fInputSorted;

			// private copy ctor
			CParseHandlerPhysicalDML(const CParseHandlerPhysicalDML &);

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

			// parse the dml type from the attribute value
			static
			EdxlDmlType EdxlDmlOpType(const XMLCh *xmlszDmlType);

		public:
			// ctor
			CParseHandlerPhysicalDML
				(
				IMemoryPool *memory_pool,
				CParseHandlerManager *parse_handler_mgr,
				CParseHandlerBase *parse_handler_root
				);
	};
}

#endif // !GPDXL_CParseHandlerPhysicalDML_H

// EOF
