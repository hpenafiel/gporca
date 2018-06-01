//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarIfStmt.h
//
//	@doc:
//		
//		Class for representing DXL If Statement (Case Statement is represented as a nested if statement)
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLScalarIfStmt_H
#define GPDXL_CDXLScalarIfStmt_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLScalar.h"
#include "naucrates/md/IMDId.h"

namespace gpdxl
{
	using namespace gpos;
	using namespace gpmd;

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLScalarIfStmt
	//
	//	@doc:
	//		Class for representing DXL IF statement
	//
	//---------------------------------------------------------------------------
	class CDXLScalarIfStmt : public CDXLScalar
	{
		private:
			// catalog MDId of the return type
			IMDId *m_pmdidResultType;

			// private copy ctor
			CDXLScalarIfStmt(const CDXLScalarIfStmt&);

		public:

			// ctor
			CDXLScalarIfStmt
				(
				IMemoryPool *memory_pool,
				IMDId *mdid_type
				);

			//dtor
			virtual
			~CDXLScalarIfStmt();

			// name of the operator
			const CWStringConst *PstrOpName() const;

			IMDId *PmdidResultType() const;

			// DXL Operator ID
			Edxlopid Edxlop() const;

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// conversion function
			static
			CDXLScalarIfStmt *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopScalarIfStmt == dxl_op->Edxlop());

				return dynamic_cast<CDXLScalarIfStmt*>(dxl_op);
			}

			// does the operator return a boolean result
			virtual
			BOOL FBoolean(CMDAccessor *pmda) const;

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *pdxln, BOOL validate_children) const;
#endif // GPOS_DEBUG
	};
}
#endif // !GPDXL_CDXLScalarIfStmt_H

// EOF
