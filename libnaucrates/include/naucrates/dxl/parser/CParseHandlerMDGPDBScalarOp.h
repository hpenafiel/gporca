//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CParseHandlerMDGPDBScalarOp.h
//
//	@doc:
//		SAX parse handler class for GPDB scalar operator metadata
//---------------------------------------------------------------------------

#ifndef GPDXL_CParseHandlerMDGPDBScalarOp_H
#define GPDXL_CParseHandlerMDGPDBScalarOp_H

#include "gpos/base.h"
#include "naucrates/dxl/parser/CParseHandlerMetadataObject.h"

namespace gpdxl
{
	using namespace gpos;

	XERCES_CPP_NAMESPACE_USE
	
	//---------------------------------------------------------------------------
	//	@class:
	//		CParseHandlerMDGPDBScalarOp
	//
	//	@doc:
	//		Parse handler for GPDB scalar operator metadata
	//
	//---------------------------------------------------------------------------
	class CParseHandlerMDGPDBScalarOp : public CParseHandlerMetadataObject
	{
		private:
			// id and version 
			IMDId *m_mdid;
			
			// name
			CMDName *m_mdname;
			
			// type of left operand
			IMDId *m_pmdidTypeLeft;
			
			// type of right operand
			IMDId *m_pmdidTypeRight;

			// type of result operand
			IMDId *m_pmdidTypeResult;
			
			// id of function which implements the operator
			IMDId *m_func_mdid;
			
			// id of commute operator
			IMDId *m_pmdidOpCommute;
			
			// id of inverse operator
			IMDId *m_pmdidOpInverse;
			
			// comparison type
			IMDType::ECmpType m_ecmpt;
			
			// does operator return NULL on NULL input?
			BOOL m_fReturnsNullOnNullInput;

			// private copy ctor
			CParseHandlerMDGPDBScalarOp(const CParseHandlerMDGPDBScalarOp &);
			
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

			// is this a supported child elem of the scalar op
			BOOL FSupportedChildElem(const XMLCh* const xmlsz);
						
		public:
			// ctor
			CParseHandlerMDGPDBScalarOp
				(
				IMemoryPool *memory_pool,
				CParseHandlerManager *parse_handler_mgr,
				CParseHandlerBase *parse_handler_root
				);			
	};
}

#endif // !GPDXL_CParseHandlerMDGPDBScalarOp_H

// EOF
