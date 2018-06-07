//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CMDScalarOpGPDB.h
//
//	@doc:
//		Class for representing GPDB-specific scalar operators in the MD cache
//---------------------------------------------------------------------------



#ifndef GPMD_CMDScalarOpGPDB_H
#define GPMD_CMDScalarOpGPDB_H

#include "gpos/base.h"

#include "naucrates/md/IMDScalarOp.h"

namespace gpmd
{
	using namespace gpos;

	
	//---------------------------------------------------------------------------
	//	@class:
	//		CMDScalarOpGPDB
	//
	//	@doc:
	//		Class for representing GPDB-specific scalar operators in the MD cache
	//
	//---------------------------------------------------------------------------
	class CMDScalarOpGPDB : public IMDScalarOp
	{		
		
		private:
		
			// memory pool
			IMemoryPool *m_memory_pool;
			
			// DXL for object
			const CWStringDynamic *m_pstr;
			
			// operator id
			IMDId *m_mdid;
			
			// operator name
			CMDName *m_mdname;
			
			// type of left operand
			IMDId *m_mdid_type_left;
			
			// type of right operand
			IMDId *m_pmdidTypeRight;

			// type of result operand
			IMDId *m_mdid_type_result;
			
			// id of function which implements the operator
			IMDId *m_func_mdid;
			
			// id of commute operator
			IMDId *m_mdid_commute_opr;
			
			// id of inverse operator
			IMDId *m_mdid_inverse_opr;
			
			// comparison type for comparison operators
			IMDType::ECmpType m_comparision_type;
			
			// does operator return NULL when all inputs are NULL?
			BOOL m_returns_null_on_null_input;
			
			// operator classes this operator belongs to
			DrgPmdid *m_pdrgpmdidOpClasses;

			CMDScalarOpGPDB(const CMDScalarOpGPDB &);
			
		public:
			
			// ctor/dtor
			CMDScalarOpGPDB
				(
				IMemoryPool *memory_pool,
				IMDId *pmdid,
				CMDName *mdname,
				IMDId *pmdidTypeLeft,
				IMDId *pmdidTypeRight,
				IMDId *pmdidTypeResult,
				IMDId *mdid_func,
				IMDId *pmdidOpCommute,
				IMDId *pmdidOpInverse,
				IMDType::ECmpType ecmpt,
				BOOL fReturnsNullOnNullInput,
				DrgPmdid *pdrgpmdidOpClasses
				);
			
			~CMDScalarOpGPDB();
			
			// accessors
			virtual 
			const CWStringDynamic *Pstr() const
			{
				return m_pstr;
			}
			
			// operator id
			virtual
			IMDId *MDId() const;
			
			// operator name
			virtual
			CMDName Mdname() const;

			// left operand type id
			virtual
			IMDId *PmdidTypeLeft() const;
			
			// right operand type id
			virtual
			IMDId *PmdidTypeRight() const;

			// resulttype id
			virtual
			IMDId *PmdidTypeResult() const;

			// implementer function id
			virtual
			IMDId *FuncMdId() const;

			// commutor id
			virtual
			IMDId *PmdidOpCommute() const;

			// inverse operator id
			virtual
			IMDId *PmdidOpInverse() const;

			// is this an equality operator
			virtual
			BOOL FEquality() const;

			// does operator return NULL when all inputs are NULL?
			// STRICT implies NULL-returning, but the opposite is not always true,
			// the implementation in GPDB returns what STRICT property states
			virtual
			BOOL FReturnsNullOnNullInput() const;

			// comparison type
			virtual
			IMDType::ECmpType ParseCmpType() const;
			
			// serialize object in DXL format
			virtual
			void Serialize(gpdxl::CXMLSerializer *xml_serializer) const;
			
			// number of classes this operator belongs to
			virtual
			ULONG UlOpCLasses() const;
			
			// operator class at given position
			virtual
			IMDId *PmdidOpClass(ULONG ulPos) const;
			
#ifdef GPOS_DEBUG
			// debug print of the type in the provided stream
			virtual
			void DebugPrint(IOstream &os) const;
#endif
			
	};
}

#endif // !GPMD_CMDScalarOpGPDB_H

// EOF
