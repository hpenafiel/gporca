//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp..
//
//	@filename:
//		CParseHandlerSearchStage.h
//
//	@doc:
//		Parse handler for search stage.
//---------------------------------------------------------------------------

#ifndef GPDXL_CParseHandlerSearchStage_H
#define GPDXL_CParseHandlerSearchStage_H

#include "gpos/base.h"
#include "naucrates/dxl/parser/CParseHandlerBase.h"

#include "gpopt/cost/CCost.h"
#include "gpopt/xforms/CXform.h"

namespace gpdxl
{
	using namespace gpos;

	//---------------------------------------------------------------------------
	//	@class:
	//		CParseHandlerSearchStage
	//
	//	@doc:
	//		Parse handler for search stage
	//
	//---------------------------------------------------------------------------
	class CParseHandlerSearchStage : public CParseHandlerBase
	{

		private:

			// set of search stage xforms
			CXformSet *m_pxfs;

			// cost threshold
			CCost m_costThreshold;

			// time threshold in milliseconds
			ULONG m_ulTimeThreshold;

			// private ctor
			CParseHandlerSearchStage(const CParseHandlerSearchStage&);

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
			CParseHandlerSearchStage
				(
				IMemoryPool *memory_pool,
				CParseHandlerManager *parse_handler_mgr,
				CParseHandlerBase *parse_handler_root
				);

			// dtor
			virtual
			~CParseHandlerSearchStage();

			// returns stage xforms
			CXformSet *Pxfs() const
			{
				return m_pxfs;
			}

			// returns stage cost threshold
			CCost CostThreshold() const
			{
				return m_costThreshold;
			}

			// returns time threshold
			ULONG UlTimeThreshold() const
			{
				return m_ulTimeThreshold;
			}

			EDxlParseHandlerType GetParseHandlerType() const
			{
				return EdxlphSearchStrategy;
			}

	};
}

#endif // !GPDXL_CParseHandlerSearchStage_H

// EOF
