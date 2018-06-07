//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerStatsDerivedColumn.h
//
//	@doc:
//		Parse handler for derived column statistics
//---------------------------------------------------------------------------

#ifndef GPDXL_CParseHandlerStatsDerivedColumn_H
#define GPDXL_CParseHandlerStatsDerivedColumn_H

#include "gpos/base.h"
#include "naucrates/dxl/parser/CParseHandlerBase.h"
#include "naucrates/md/CDXLStatsDerivedColumn.h"

namespace gpdxl
{
	using namespace gpos;
	using namespace gpmd;

	XERCES_CPP_NAMESPACE_USE

	//---------------------------------------------------------------------------
	//	@class:
	//		CParseHandlerStatsDerivedColumn
	//
	//	@doc:
	//		Parse handler for derived column statistics
	//
	//---------------------------------------------------------------------------
	class CParseHandlerStatsDerivedColumn : public CParseHandlerBase
	{
		private:

			// column id
			ULONG m_colid;

			// width
			CDouble m_dWidth;

			// null fraction
			CDouble m_dNullFreq;

			// ndistinct of remaining tuples
			CDouble m_dDistinctRemain;

			// frequency of remaining tuples
			CDouble m_dFreqRemain;

			// derived column stats
			CDXLStatsDerivedColumn *m_pstatsdercol;

			// private copy ctor
			CParseHandlerStatsDerivedColumn(const CParseHandlerStatsDerivedColumn &);

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
			CParseHandlerStatsDerivedColumn
				(
				IMemoryPool *memory_pool,
				CParseHandlerManager *parse_handler_mgr,
				CParseHandlerBase *parse_handler_root
				);

			//dtor
			~CParseHandlerStatsDerivedColumn();

			// derived column stats
			CDXLStatsDerivedColumn *Pstatsdercol() const
			{
				return m_pstatsdercol;
			}
	};
}

#endif // !GPDXL_CParseHandlerStatsDerivedColumn_H

// EOF
