//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLPhysicalMotion.h
//
//	@doc:
//		Base class for representing DXL motion operators.
//---------------------------------------------------------------------------



#ifndef GPDXL_CDXLPhysicalMotion_H
#define GPDXL_CDXLPhysicalMotion_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"

#include "naucrates/dxl/operators/CDXLPhysical.h"
#include "naucrates/dxl/operators/CDXLNode.h"

namespace gpdxl
{
	using namespace gpos;
	
	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLPhysicalMotion
	//
	//	@doc:
	//		Base class for representing DXL motion operators
	//
	//---------------------------------------------------------------------------
	class CDXLPhysicalMotion : public CDXLPhysical
	{
		private:
			// private copy ctor
			CDXLPhysicalMotion(CDXLPhysicalMotion&);
			
			// serialize the given list of segment ids into a comma-separated string
			CWStringDynamic *PstrSegIds(const IntPtrArray *pdrgpi) const;

			// serialize input and output segment ids into a comma-separated string
			CWStringDynamic *PstrInputSegIds() const;
			CWStringDynamic *PstrOutputSegIds() const;
			
		protected:
			// list of input segment ids
			IntPtrArray *m_pdrgpiInputSegIds;
			
			// list of output segment ids
			IntPtrArray *m_pdrgpiOutputSegIds;

			void SerializeSegmentInfoToDXL(CXMLSerializer *xml_serializer) const;

			
		public:
			// ctor/dtor
			explicit
			CDXLPhysicalMotion(IMemoryPool *memory_pool);

			virtual
			~CDXLPhysicalMotion();
			
			// accessors
			const IntPtrArray *PdrgpiInputSegIds() const;
			const IntPtrArray *PdrgpiOutputSegIds() const;
			
			// setters
			void SetInputSegIds(IntPtrArray *pdrgpi);
			void SetOutputSegIds(IntPtrArray *pdrgpi);
			void SetSegmentInfo(IntPtrArray *pdrgpiInputSegIds, IntPtrArray *pdrgpiOutputSegIds);

			// index of relational child node in the children array
			virtual 
			ULONG UlChildIndex() const = 0;
			
			// conversion function
			static
			CDXLPhysicalMotion *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopPhysicalMotionGather == dxl_op->Edxlop()
						|| EdxlopPhysicalMotionBroadcast == dxl_op->Edxlop()
						|| EdxlopPhysicalMotionRedistribute == dxl_op->Edxlop()
						|| EdxlopPhysicalMotionRoutedDistribute == dxl_op->Edxlop()
						|| EdxlopPhysicalMotionRandom == dxl_op->Edxlop());

				return dynamic_cast<CDXLPhysicalMotion*>(dxl_op);
			}

	};
}
#endif // !GPDXL_CDXLPhysicalMotion_H

// EOF

