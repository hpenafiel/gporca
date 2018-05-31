//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CQueryToDXLResult.h
//
//	@doc:
//		Class representing the result of the Query to DXL translation
//		
//---------------------------------------------------------------------------

#ifndef GPDXL_CTranslatorQueryToDXLOutput_H
#define GPDXL_CTranslatorQueryToDXLOutput_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLNode.h"

namespace gpdxl
{

	using namespace gpos;

	//---------------------------------------------------------------------------
	//	@class:
	//		CQueryToDXLResult
	//
	//	@doc:
	//		Class representing the result of the Query to DXL translation
	//
	//---------------------------------------------------------------------------
	class CQueryToDXLResult
	{
		private:

			// DXL representing the Query
			CDXLNode *m_pdxln;

			// array of DXL nodes that represent the query output
			DrgPdxln *m_pdrgpdxlnQueryOutput;
			
			// CTE list
			DrgPdxln *m_cte_producer_dxl_array;

		public:
			// ctor
			CQueryToDXLResult(CDXLNode *pdxlnQuery, DrgPdxln *pdrgpdxlnOutput, DrgPdxln *cte_dxlnode_array);

			// dtor
			~CQueryToDXLResult();

			// return the DXL representation of the query
			const CDXLNode *Pdxln() const;

			// return the array of output columns
			const DrgPdxln *GetOutputColumnsDXLArray() const;
			
			// return the array of CTEs
			const DrgPdxln *GetCTEProducerDXLArray() const;
	};
}

#endif // !GPDXL_CTranslatorQueryToDXLOutput_H

// EOF
