//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC, Corp.
//
//	@filename:
//		CDXLScalarSubqueryQuantified.h
//
//	@doc:
//		Base class for ANY/ALL subqueries
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLScalarSubqueryQuantified_H
#define GPDXL_CDXLScalarSubqueryQuantified_H

#include "gpos/base.h"

#include "naucrates/md/IMDId.h"
#include "naucrates/md/CMDName.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLScalar.h"

namespace gpdxl
{

	using namespace gpmd;

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLScalarSubqueryQuantified
	//
	//	@doc:
	//		Base class for ANY/ALL subqueries
	//
	//---------------------------------------------------------------------------
	class CDXLScalarSubqueryQuantified : public CDXLScalar
	{
		public:

			// indices of the subquery elements in the children array
			enum Edxlsqquantified
			{
				EdxlsqquantifiedIndexScalar,
				EdxlsqquantifiedIndexRelational,
				EdxlsqquantifiedIndexSentinel
			};

		private:
			// id of the scalar comparison operator
			IMDId *m_pmdidScalarOp;

			// name of scalar comparison operator
			CMDName *m_pmdnameScalarOp;

			// colid produced by the relational child of the AnySubquery operator
			ULONG m_ulColId;

			// private copy ctor
			CDXLScalarSubqueryQuantified(CDXLScalarSubqueryQuantified&);

		public:
			// ctor
			CDXLScalarSubqueryQuantified(IMemoryPool *memory_pool, IMDId *pmdidScalarOp, CMDName *mdname, ULONG col_id);

			// dtor
			virtual
			~CDXLScalarSubqueryQuantified();

			// scalar operator id
			IMDId *PmdidScalarOp() const
			{
				return m_pmdidScalarOp;
			}

			// scalar operator name
			const CMDName *PmdnameScalarOp() const
			{
				return m_pmdnameScalarOp;
			}

			// subquery colid
			ULONG UlColId() const
			{
				return m_ulColId;
			}

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *, const CDXLNode *) const;

			// conversion function
			static
			CDXLScalarSubqueryQuantified *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopScalarSubqueryAll == dxl_op->GetDXLOperator() ||
						EdxlopScalarSubqueryAny == dxl_op->GetDXLOperator());

				return dynamic_cast<CDXLScalarSubqueryQuantified*>(dxl_op);
			}

			// does the operator return a boolean result
			virtual
			BOOL HasBoolResult
					(
					CMDAccessor *//md_accessor
					)
					const
			{
				return true;
			}

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *dxlnode, BOOL validate_children) const;
#endif // GPOS_DEBUG

	};
}

#endif // !GPDXL_CDXLScalarSubqueryQuantified_H

// EOF
