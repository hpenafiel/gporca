//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLLogicalCTEAnchor.h
//
//	@doc:
//		Class for representing DXL logical CTE anchors
//---------------------------------------------------------------------------
#ifndef GPDXL_CDXLLogicalCTEAnchor_H
#define GPDXL_CDXLLogicalCTEAnchor_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLLogical.h"

namespace gpdxl
{

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLLogicalCTEAnchor
	//
	//	@doc:
	//		Class for representing DXL logical CTE producers
	//
	//---------------------------------------------------------------------------
	class CDXLLogicalCTEAnchor : public CDXLLogical
	{
		private:

			// cte id
			ULONG m_ulId;
			
			// private copy ctor
			CDXLLogicalCTEAnchor(CDXLLogicalCTEAnchor&);

		public:
			// ctor
			CDXLLogicalCTEAnchor(IMemoryPool *memory_pool, ULONG ulId);
			
			// operator type
			Edxlopid Edxlop() const;

			// operator name
			const CWStringConst *PstrOpName() const;

			// cte identifier
			ULONG UlId() const
			{
				return m_ulId;
			}
			
			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;


#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *, BOOL validate_children) const;
#endif // GPOS_DEBUG

			// conversion function
			static
			CDXLLogicalCTEAnchor *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopLogicalCTEAnchor == dxl_op->Edxlop());
				return dynamic_cast<CDXLLogicalCTEAnchor*>(dxl_op);
			}
	};
}
#endif // !GPDXL_CDXLLogicalCTEAnchor_H

// EOF
