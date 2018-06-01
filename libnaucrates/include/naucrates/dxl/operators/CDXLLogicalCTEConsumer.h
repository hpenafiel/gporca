//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLLogicalCTEConsumer.h
//
//	@doc:
//		Class for representing DXL logical CTE Consumer operators
//---------------------------------------------------------------------------
#ifndef GPDXL_CDXLLogicalCTEConsumer_H
#define GPDXL_CDXLLogicalCTEConsumer_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLLogical.h"

namespace gpdxl
{

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLLogicalCTEConsumer
	//
	//	@doc:
	//		Class for representing DXL logical CTE Consumers
	//
	//---------------------------------------------------------------------------
	class CDXLLogicalCTEConsumer : public CDXLLogical
	{
		private:

			// cte id
			ULONG m_ulId;

			// output column ids
			ULongPtrArray *m_pdrgpulColIds;
			
			// private copy ctor
			CDXLLogicalCTEConsumer(CDXLLogicalCTEConsumer&);

		public:
			// ctor
			CDXLLogicalCTEConsumer(IMemoryPool *memory_pool, ULONG ulId, ULongPtrArray *pdrgpulColIds);
			
			// dtor
			virtual
			~CDXLLogicalCTEConsumer();

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
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// check if given column is defined by operator
			virtual
			BOOL FDefinesColumn(ULONG ulColId) const;

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *, BOOL validate_children) const;
#endif // GPOS_DEBUG
			
			// conversion function
			static
			CDXLLogicalCTEConsumer *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopLogicalCTEConsumer == dxl_op->GetDXLOperator());
				return dynamic_cast<CDXLLogicalCTEConsumer*>(dxl_op);
			}

	};
}
#endif // !GPDXL_CDXLLogicalCTEConsumer_H

// EOF
