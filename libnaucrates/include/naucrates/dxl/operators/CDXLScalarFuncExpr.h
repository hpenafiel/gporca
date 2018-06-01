//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarFuncExpr.h
//
//	@doc:
//		Class for representing DXL scalar FuncExpr
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLScalarFuncExpr_H
#define GPDXL_CDXLScalarFuncExpr_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLScalar.h"
#include "naucrates/md/IMDId.h"

namespace gpdxl
{
	using namespace gpos;
	using namespace gpmd;

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLScalarFuncExpr
	//
	//	@doc:
	//		Class for representing DXL scalar FuncExpr
	//
	//---------------------------------------------------------------------------
	class CDXLScalarFuncExpr : public CDXLScalar
	{
		private:

			// catalog id of the function
			IMDId *m_pmdidFunc;

			// return type
			IMDId *m_pmdidRetType;

			const INT m_iRetTypeModifier;

			// does the func return a set
			BOOL m_fReturnSet;

			// private copy ctor
			CDXLScalarFuncExpr(const CDXLScalarFuncExpr&);

		public:
			// ctor
			CDXLScalarFuncExpr
				(
				IMemoryPool *memory_pool,
				IMDId *pmdidFunc,
				IMDId *pmdidRetType,
				INT iRetTypeModifier,
				BOOL fretset
				);

			//dtor
			virtual
			~CDXLScalarFuncExpr();

			// ident accessors
			Edxlopid Edxlop() const;

			// name of the DXL operator
			const CWStringConst *PstrOpName() const;

			// function id
			IMDId *PmdidFunc() const;

			// return type
			IMDId *PmdidRetType() const;

			INT TypeModifier() const;

			// does function return a set
			BOOL FReturnSet() const;

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// conversion function
			static
			CDXLScalarFuncExpr *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopScalarFuncExpr == dxl_op->Edxlop());

				return dynamic_cast<CDXLScalarFuncExpr*>(dxl_op);
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

#endif // !GPDXL_CDXLScalarFuncExpr_H

// EOF
