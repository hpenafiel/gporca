//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLPhysicalMergeJoin.h
//
//	@doc:
//		Class for representing DXL merge join operators.
//---------------------------------------------------------------------------



#ifndef GPDXL_CDXLPhysicalMergeJoin_H
#define GPDXL_CDXLPhysicalMergeJoin_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLPhysicalJoin.h"

namespace gpdxl
{
	// indices of merge join elements in the children array
	enum Edxlmj
	{
		EdxlmjIndexProjList = 0,
		EdxlmjIndexFilter,
		EdxlmjIndexJoinFilter,
		EdxlmjIndexMergeCondList,
		EdxlmjIndexLeftChild,
		EdxlmjIndexRightChild,
		EdxlmjIndexSentinel
	};

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLPhysicalMergeJoin
	//
	//	@doc:
	//		Class for representing DXL merge operators
	//
	//---------------------------------------------------------------------------
	class CDXLPhysicalMergeJoin : public CDXLPhysicalJoin
	{
		private:
			// true if outer relation has unique values for the merge key
			BOOL m_fUniqueOuter;
			
			// private copy ctor
			CDXLPhysicalMergeJoin(const CDXLPhysicalMergeJoin&);

		public:
			// ctor
			CDXLPhysicalMergeJoin(IMemoryPool *memory_pool, EdxlJoinType edxljt, BOOL fUniqueOuter);
			
			// accessors
			Edxlopid Edxlop() const;
			const CWStringConst *PstrOpName() const;
			
			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// conversion function
			static
			CDXLPhysicalMergeJoin *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopPhysicalMergeJoin == dxl_op->Edxlop());

				return dynamic_cast<CDXLPhysicalMergeJoin*>(dxl_op);
			}

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *, BOOL validate_children) const;
#endif // GPOS_DEBUG
			
	};
}
#endif // !GPDXL_CDXLPhysicalMergeJoin_H

// EOF

