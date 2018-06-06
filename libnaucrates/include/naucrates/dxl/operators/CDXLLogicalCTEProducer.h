//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLLogicalCTEProducer.h
//
//	@doc:
//		Class for representing DXL logical CTE producer operators
//---------------------------------------------------------------------------
#ifndef GPDXL_CDXLLogicalCTEProducer_H
#define GPDXL_CDXLLogicalCTEProducer_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLLogical.h"

namespace gpdxl
{

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLLogicalCTEProducer
	//
	//	@doc:
	//		Class for representing DXL logical CTE producers
	//
	//---------------------------------------------------------------------------
	class CDXLLogicalCTEProducer : public CDXLLogical
	{
		private:

			// cte id
			ULONG m_ulId;

			// output column ids
			ULongPtrArray *m_pdrgpulColIds;
			
			// private copy ctor
			CDXLLogicalCTEProducer(CDXLLogicalCTEProducer&);

		public:
			// ctor
			CDXLLogicalCTEProducer(IMemoryPool *memory_pool, ULONG ulId, ULongPtrArray *pdrgpulColIds);
			
			// dtor
			virtual
			~CDXLLogicalCTEProducer();

			// operator type
			virtual
			Edxlopid GetDXLOperator() const;

			// operator name
			virtual
			const CWStringConst *GetOpNameStr() const;

			// cte identifier
			ULONG UlId() const
			{
				return m_ulId;
			}
			
			ULongPtrArray *PdrgpulColIds() const
			{
				return m_pdrgpulColIds;
			}

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *dxlnode) const;

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *, BOOL validate_children) const;
#endif // GPOS_DEBUG

			// conversion function
			static
			CDXLLogicalCTEProducer *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopLogicalCTEProducer == dxl_op->GetDXLOperator());
				return dynamic_cast<CDXLLogicalCTEProducer*>(dxl_op);
			}
	};
}
#endif // !GPDXL_CDXLLogicalCTEProducer_H

// EOF
