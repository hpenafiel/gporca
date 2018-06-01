//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalCTEProducer.h
//
//	@doc:
//		Class for representing DXL physical CTE producer operators
//---------------------------------------------------------------------------
#ifndef GPDXL_CDXLPhysicalCTEProducer_H
#define GPDXL_CDXLPhysicalCTEProducer_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"

#include "naucrates/dxl/operators/CDXLPhysical.h"

namespace gpdxl
{

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLPhysicalCTEProducer
	//
	//	@doc:
	//		Class for representing DXL physical CTE producers
	//
	//---------------------------------------------------------------------------
	class CDXLPhysicalCTEProducer : public CDXLPhysical
	{
		private:

			// cte id
			ULONG m_ulId;

			// output column ids
			ULongPtrArray *m_pdrgpulColIds;

			// private copy ctor
			CDXLPhysicalCTEProducer(CDXLPhysicalCTEProducer&);

		public:
			// ctor
			CDXLPhysicalCTEProducer(IMemoryPool *memory_pool, ULONG ulId, ULongPtrArray *pdrgpulColIds);

			// dtor
			virtual
			~CDXLPhysicalCTEProducer();

			// operator type
			virtual
			Edxlopid Edxlop() const;

			// operator name
			virtual
			const CWStringConst *PstrOpName() const;

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
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *pdxln, BOOL validate_children) const;
#endif // GPOS_DEBUG

			// conversion function
			static
			CDXLPhysicalCTEProducer *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopPhysicalCTEProducer == dxl_op->Edxlop());
				return dynamic_cast<CDXLPhysicalCTEProducer*>(dxl_op);
			}
	};
}
#endif // !GPDXL_CDXLPhysicalCTEProducer_H

// EOF
