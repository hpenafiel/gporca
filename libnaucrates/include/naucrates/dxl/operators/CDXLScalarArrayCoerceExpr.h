//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 Pivotal Inc.
//
//	@filename:
//		CDXLScalarArrayCoerceExpr.h
//
//	@doc:
//		Class for representing DXL ArrayCoerceExpr operation,
//		the operator will apply type casting for each element in this array
//		using the given element coercion function.
//	@owner:
//
//	@test:
//
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLScalarArrayCoerceExpr_H
#define GPDXL_CDXLScalarArrayCoerceExpr_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLScalarCoerceBase.h"
#include "naucrates/md/IMDId.h"

namespace gpdxl
{
	using namespace gpos;
	using namespace gpmd;

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLScalarArrayCoerceExpr
	//
	//	@doc:
	//		Class for representing DXL array coerce operator
	//---------------------------------------------------------------------------
	class CDXLScalarArrayCoerceExpr : public CDXLScalarCoerceBase
	{
		private:
			// catalog MDId of element coerce function
			IMDId *m_pmdidElementFunc;

			// conversion semantics flag to pass to func
			BOOL m_fIsExplicit;

			// private copy ctor
			CDXLScalarArrayCoerceExpr(const CDXLScalarArrayCoerceExpr&);

		public:
			CDXLScalarArrayCoerceExpr
				(
				IMemoryPool *memory_pool,
				IMDId *pmdidElementFunc,
				IMDId *pmdidResultType,
				INT type_modifier,
				BOOL fIsExplicit,
				EdxlCoercionForm edxlcf,
				INT iLoc
				);

			virtual
			~CDXLScalarArrayCoerceExpr()
			{
				m_pmdidElementFunc->Release();
			}

			// ident accessor
			virtual
			Edxlopid Edxlop() const
			{
				return EdxlopScalarArrayCoerceExpr;
			}

			// return metadata id of element coerce function
			IMDId *PmdidElementFunc() const
			{
				return m_pmdidElementFunc;
			}

			BOOL FIsExplicit() const
			{
				return m_fIsExplicit;
			}

			// name of the DXL operator name
			virtual
			const CWStringConst *PstrOpName() const;

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// conversion function
			static
			CDXLScalarArrayCoerceExpr *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopScalarArrayCoerceExpr == dxl_op->Edxlop());

				return dynamic_cast<CDXLScalarArrayCoerceExpr*>(dxl_op);
			}
	};
}

#endif // !GPDXL_CDXLScalarArrayCoerceExpr_H

// EOF
