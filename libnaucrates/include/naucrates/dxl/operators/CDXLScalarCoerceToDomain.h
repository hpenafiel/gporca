//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CDXLScalarCoerceToDomain.h
//
//	@doc:
//		Class for representing DXL CoerceToDomain operation,
//		the operator captures coercing a value to a domain type,
//
//		at runtime, the precise set of constraints to be checked against
//		value are determined,
//		if the value passes, it is returned as the result, otherwise an error
//		is raised.
//
//	@owner:
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLScalarCoerceToDomain_H
#define GPDXL_CDXLScalarCoerceToDomain_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLScalarCoerceBase.h"
#include "naucrates/md/IMDId.h"

namespace gpdxl
{
	using namespace gpos;
	using namespace gpmd;

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLScalarCoerceToDomain
	//
	//	@doc:
	//		Class for representing DXL casting operator
	//
	//---------------------------------------------------------------------------
	class CDXLScalarCoerceToDomain : public CDXLScalarCoerceBase
	{

		private:
			// private copy ctor
			CDXLScalarCoerceToDomain(const CDXLScalarCoerceToDomain&);

		public:
			// ctor/dtor
			CDXLScalarCoerceToDomain
				(
				IMemoryPool *memory_pool,
				IMDId *mdid_type,
				INT type_modifier,
				EdxlCoercionForm edxlcf,
				INT iLoc
				);

			virtual
			~CDXLScalarCoerceToDomain()
			{
			}

			// ident accessor
			virtual
			Edxlopid Edxlop() const
			{
				return EdxlopScalarCoerceToDomain;
			}

			// name of the DXL operator name
			virtual
			const CWStringConst *PstrOpName() const;

			// conversion function
			static
			CDXLScalarCoerceToDomain *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopScalarCoerceToDomain == dxl_op->Edxlop());

				return dynamic_cast<CDXLScalarCoerceToDomain*>(dxl_op);
			}
	};
}

#endif // !GPDXL_CDXLScalarCoerceToDomain_H

// EOF
