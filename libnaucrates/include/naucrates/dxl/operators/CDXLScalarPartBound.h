//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CDXLScalarPartBound.h
//
//	@doc:
//		Class for representing DXL Part boundary expressions
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLScalarPartBound_H
#define GPDXL_CDXLScalarPartBound_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLScalar.h"

namespace gpdxl
{

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLScalarPartBound
	//
	//	@doc:
	//		Class for representing DXL Part boundary expressions
	//		These expressions are created and consumed by the PartitionSelector operator
	//
	//---------------------------------------------------------------------------
	class CDXLScalarPartBound : public CDXLScalar
	{
		private:

			// partitioning level
			ULONG m_ulLevel;

			// boundary type
			IMDId *m_mdid_type;

			// whether this represents a lower or upper bound
			BOOL m_fLower;

			// private copy ctor
			CDXLScalarPartBound(const CDXLScalarPartBound&);

		public:
			// ctor
			CDXLScalarPartBound(IMemoryPool *memory_pool, ULONG ulLevel, IMDId *mdid_type, BOOL fLower);

			// dtor
			virtual
			~CDXLScalarPartBound();

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

			// boundary type
			IMDId *MDIdType() const
			{
				return m_mdid_type;
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
			BOOL FBoolean(CMDAccessor *pmda) const;

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			virtual
			void AssertValid(const CDXLNode *pdxln, BOOL validate_children) const;
#endif // GPOS_DEBUG

			// conversion function
			static
			CDXLScalarPartBound *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopScalarPartBound == dxl_op->GetDXLOperator());

				return dynamic_cast<CDXLScalarPartBound*>(dxl_op);
			}
	};
}

#endif // !GPDXL_CDXLScalarPartBound_H

// EOF
