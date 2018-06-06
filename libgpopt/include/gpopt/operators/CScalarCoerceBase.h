//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 Pivotal Inc.
//
//	@filename:
//		CScalarCoerceBase.h
//
//	@doc:
//		Scalar coerce operator base class
//
//	@owner:
//
//	@test:
//
//
//---------------------------------------------------------------------------
#ifndef GPOPT_CScalarCoerceBase_H
#define GPOPT_CScalarCoerceBase_H

#include "gpos/base.h"
#include "gpopt/operators/CScalar.h"

namespace gpopt
{
	using namespace gpos;

	//---------------------------------------------------------------------------
	//	@class:
	//		CScalarCoerceBase
	//
	//	@doc:
	//		Scalar coerce operator base class
	//
	//---------------------------------------------------------------------------
	class CScalarCoerceBase : public CScalar
	{

		private:

			// catalog MDId of the result type
			IMDId *m_result_type_mdid;

			// output type modifier
			INT m_type_modifier;

			// coercion form
			ECoercionForm m_ecf;

			// location of token to be coerced
			INT m_iLoc;

			// private copy ctor
			CScalarCoerceBase(const CScalarCoerceBase &);

		public:

			// ctor
			CScalarCoerceBase
				(
				IMemoryPool *memory_pool,
				IMDId *mdid_type,
				INT type_modifier,
				ECoercionForm edxlcf,
				INT iLoc
				);

			// dtor
			virtual
			~CScalarCoerceBase();

			// the type of the scalar expression
			virtual
			IMDId *MDIdType() const;

			// return type modifier
			INT TypeModifier() const;

			// return coercion form
			ECoercionForm Ecf() const;

			// return token location
			INT ILoc() const;

			// return a copy of the operator with remapped columns
			virtual
			COperator *PopCopyWithRemappedColumns
				(
				IMemoryPool *memory_pool,
				HMUlCr *phmulcr,
				BOOL fMustExist
				);

	}; // class CScalarCoerceBase

}


#endif // !GPOPT_CScalarCoerceBase_H

// EOF
