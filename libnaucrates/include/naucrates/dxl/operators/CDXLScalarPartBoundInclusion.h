//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CDXLScalarPartBoundInclusion.h
//
//	@doc:
//		Class for representing DXL Part boundary inclusion expressions
//		These expressions indicate whether a particular boundary of a part
//		constraint is closed (inclusive) or open (exclusive)
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLScalarPartBoundInclusion_H
#define GPDXL_CDXLScalarPartBoundInclusion_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLScalar.h"

namespace gpdxl
{

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLScalarPartBoundInclusion
	//
	//	@doc:
	//		Class for representing DXL Part boundary inclusion expressions
	//		These expressions are created and consumed by the PartitionSelector operator
	//
	//---------------------------------------------------------------------------
	class CDXLScalarPartBoundInclusion : public CDXLScalar
	{
		private:

			// partitioning level
			ULONG m_ulLevel;

			// whether this represents a lower or upper bound
			BOOL m_fLower;

			// private copy ctor
			CDXLScalarPartBoundInclusion(const CDXLScalarPartBoundInclusion&);

		public:
			// ctor
			CDXLScalarPartBoundInclusion(IMemoryPool *memory_pool, ULONG ulLevel, BOOL fLower);

			// operator type
			virtual
			Edxlopid GetDXLOperator() const;

			// operator name
			virtual
			const CWStringConst *GetOpNameStr() const;

			// partitioning level
			ULONG UlLevel() const
			{
				return m_ulLevel;
			}

			// is this a lower (or upper) bound
			BOOL FLower() const
			{
				return m_fLower;
			}

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// does the operator return a boolean result
			virtual
			BOOL FBoolean
					(
					CMDAccessor * //pmda
					)
					const
			{
				return true;
			}

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			virtual
			void AssertValid(const CDXLNode *pdxln, BOOL validate_children) const;
#endif // GPOS_DEBUG

			// conversion function
			static
			CDXLScalarPartBoundInclusion *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopScalarPartBoundInclusion == dxl_op->GetDXLOperator());

				return dynamic_cast<CDXLScalarPartBoundInclusion*>(dxl_op);
			}
	};
}

#endif // !GPDXL_CDXLScalarPartBoundInclusion_H

// EOF
