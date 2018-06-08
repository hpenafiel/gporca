//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CScalarArray.h
//
//	@doc:
//		Class for scalar arrays
//---------------------------------------------------------------------------
#ifndef GPOPT_CScalarArray_H
#define GPOPT_CScalarArray_H

#include "gpos/base.h"
#include "naucrates/md/IMDId.h"

#include "gpopt/operators/CScalar.h"
#include "gpopt/operators/CScalarConst.h"

namespace gpopt
{

	using namespace gpos;
	using namespace gpmd;

	typedef CDynamicPtrArray<CScalarConst, CleanupRelease> DrgPconst;

	//---------------------------------------------------------------------------
	//	@class:
	//		CScalarArray
	//
	//	@doc:
	//		Scalar array
	//
	//---------------------------------------------------------------------------
	class CScalarArray : public CScalar
	{
		private:
	
			// element type id
			IMDId *m_pmdidElem;
			
			// array type id
			IMDId *m_pmdidArray;
			
			// is array multidimensional
			BOOL m_fMultiDimensional;

			// const values
			DrgPconst *m_pdrgPconst;

			// private copy ctor
			CScalarArray(const CScalarArray &);
		
		public:
		
			// ctor
			CScalarArray(IMemoryPool *memory_pool, IMDId *elem_type_mdid, IMDId *array_type_mdid, BOOL is_multidimenstional);

			// ctor
			CScalarArray(IMemoryPool *memory_pool, IMDId *elem_type_mdid, IMDId *array_type_mdid, BOOL is_multidimenstional, DrgPconst *pdrgPconst);

			// dtor
			virtual 
			~CScalarArray();

			// ident accessors
			virtual 
			EOperatorId Eopid() const
			{
				return EopScalarArray;
			}
			
			// return a string for aggregate function
			virtual 
			const CHAR *SzId() const
			{
				return "CScalarArray";
			}


			// operator specific hash function
			ULONG HashValue() const;
			
			// match function
			BOOL FMatch(COperator *pop) const;
			
			// sensitivity to order of inputs
			BOOL FInputOrderSensitive() const
			{
				return true;
			}
			
			// return a copy of the operator with remapped columns
			virtual
			COperator *PopCopyWithRemappedColumns
						(
						IMemoryPool *, //memory_pool,
						HMUlCr *, //phmulcr,
						BOOL //fMustExist
						)
			{
				return PopCopyDefault();
			}

			// conversion function
			static
			CScalarArray *PopConvert
				(
				COperator *pop
				)
			{
				GPOS_ASSERT(NULL != pop);
				GPOS_ASSERT(EopScalarArray == pop->Eopid());
				
				return reinterpret_cast<CScalarArray*>(pop);
			}
			
			// element type id
			IMDId *PmdidElem() const;
			
			// array type id
			IMDId *PmdidArray() const;

			// is array multi-dimensional
			BOOL FMultiDimensional() const;

			// type of expression's result
			virtual 
			IMDId *MDIdType() const;

			// CScalarConst array
			DrgPconst *PdrgPconst() const;

			// print
			IOstream &OsPrint(IOstream &os) const;

	}; // class CScalarArray

}


#endif // !GPOPT_CScalarArray_H

// EOF
