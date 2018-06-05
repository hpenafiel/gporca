//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalRoutedDistributeMotion.h
//
//	@doc:
//		Class for representing DXL routed redistribute motion operators.
//---------------------------------------------------------------------------



#ifndef GPDXL_CDXLPhysicalRoutedDistributeMotion_H
#define GPDXL_CDXLPhysicalRoutedDistributeMotion_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLPhysicalMotion.h"

namespace gpdxl
{
	
	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLPhysicalRoutedDistributeMotion
	//
	//	@doc:
	//		Class for representing DXL routed redistribute motion operators
	//
	//---------------------------------------------------------------------------
	class CDXLPhysicalRoutedDistributeMotion : public CDXLPhysicalMotion
	{
		private:

			// indexes of the routed motion elements in the children array
			enum Edxlroutedm
			{
				EdxlroutedmIndexProjList = 0,
				EdxlroutedmIndexFilter,
				EdxlroutedmIndexSortColList,
				EdxlroutedmIndexChild,
				EdxlroutedmIndexSentinel
			};
		
			// segment id column
			ULONG m_ulSegmentIdCol;
		
			// private copy ctor
			CDXLPhysicalRoutedDistributeMotion(const CDXLPhysicalRoutedDistributeMotion&);
			
		public:
			// ctor
			CDXLPhysicalRoutedDistributeMotion(IMemoryPool *memory_pool, ULONG ulSegmentIdCol);
			
			// operator type
			Edxlopid GetDXLOperator() const;
			
			// operator name
			const CWStringConst *GetOpNameStr() const;
			
			// segment id column
			ULONG UlSegmentIdCol() const
			{
				return m_ulSegmentIdCol;
			}
			
			// index of relational child node in the children array
			virtual 
			ULONG GetRelationChildIdx() const
			{
				return EdxlroutedmIndexChild;
			}
			
			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// conversion function
			static
			CDXLPhysicalRoutedDistributeMotion *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopPhysicalMotionRoutedDistribute == dxl_op->GetDXLOperator());

				return dynamic_cast<CDXLPhysicalRoutedDistributeMotion*>(dxl_op);
			}

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *pdxln, BOOL validate_children) const;
#endif // GPOS_DEBUG
			
	};
}
#endif // !GPDXL_CDXLPhysicalRoutedDistributeMotion_H

// EOF

