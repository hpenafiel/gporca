//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CDXLPhysicalCTAS.h
//
//	@doc:
//		Class for representing DXL physical CTAS operators
//---------------------------------------------------------------------------
#ifndef GPDXL_CDXLPhysicalCTAS_H
#define GPDXL_CDXLPhysicalCTAS_H

#include "gpos/base.h"
#include "naucrates/md/IMDRelation.h"
#include "naucrates/dxl/operators/CDXLColDescr.h"

#include "naucrates/dxl/operators/CDXLPhysical.h"

namespace gpdxl
{

	// fwd decl
	class CDXLCtasStorageOptions;
	
	using namespace gpmd;

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLPhysicalCTAS
	//
	//	@doc:
	//		Class for representing DXL physical CTAS operators
	//
	//---------------------------------------------------------------------------
	class CDXLPhysicalCTAS : public CDXLPhysical
	{
		private:

			// schema name
			CMDName *m_pmdnameSchema;
		
			// table name
			CMDName *m_pmdnameRel;
			
			// list of columns
			ColumnDescrDXLArray *m_col_descr_array;
			
			// storage options
			CDXLCtasStorageOptions *m_pdxlctasopt;
			
			// distribution policy
			IMDRelation::Ereldistrpolicy m_ereldistrpolicy;
	
			// list of distribution column positions		
			ULongPtrArray *m_pdrgpulDistr;
			
			// is this a temporary table
			BOOL m_fTemporary;
			
			// does table have oids
			BOOL m_fHasOids;
			
			// storage type
			IMDRelation::Erelstoragetype m_erelstorage;
			
			// list of source column ids		
			ULongPtrArray *m_pdrgpulSource;

			// list of vartypmod
			IntPtrArray *m_pdrgpiVarTypeMod;

			// private copy ctor
			CDXLPhysicalCTAS(CDXLPhysicalCTAS&);

		public:
			// ctor
			CDXLPhysicalCTAS
				(
				IMemoryPool *memory_pool, 
				CMDName *pmdnameSchema, 
				CMDName *pmdnameRel, 
				ColumnDescrDXLArray *pdrgpdxcd,
				CDXLCtasStorageOptions *pdxlctasopt,
				IMDRelation::Ereldistrpolicy ereldistrpolicy,
				ULongPtrArray *pdrgpulDistr, 
				BOOL fTemporary, 
				BOOL fHasOids, 
				IMDRelation::Erelstoragetype erelstorage,
				ULongPtrArray *pdrgpulSource,
				IntPtrArray *pdrgpiVarTypeMod
				);

			// dtor
			virtual
			~CDXLPhysicalCTAS();

			// operator type
			Edxlopid GetDXLOperator() const;

			// operator name
			const CWStringConst *GetOpNameStr() const;

			// column descriptors
			ColumnDescrDXLArray *GetColumnDescrDXLArray() const
			{
				return m_col_descr_array;
			}
			
			// distribution type
			IMDRelation::Ereldistrpolicy Ereldistrpolicy() const
			{
				return m_ereldistrpolicy;
			}
			
			// distribution column positions
			ULongPtrArray *PdrgpulDistr() const
			{
				return m_pdrgpulDistr;
			}
		
			// source column ids
			ULongPtrArray *PdrgpulSource() const
			{
				return m_pdrgpulSource;
			} 
			
			// list of vartypmod for target expressions
			IntPtrArray *PdrgpiVarTypeMod() const
			{
				return m_pdrgpiVarTypeMod;
			}

			// table name
			CMDName *PmdnameSchema() const
			{
				return m_pmdnameSchema;
			}
			
			// table name
			CMDName *MdName() const
			{
				return m_pmdnameRel;
			}
			
			// is temporary
			BOOL FTemporary() const
			{
				return m_fTemporary;
			}

			// CTAS storage options
			CDXLCtasStorageOptions *Pdxlctasopt() const
			{
				return m_pdxlctasopt;
			}
			
			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *pdxln, BOOL validate_children) const;
#endif // GPOS_DEBUG

			// conversion function
			static
			CDXLPhysicalCTAS *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopPhysicalCTAS == dxl_op->GetDXLOperator());
				return dynamic_cast<CDXLPhysicalCTAS*>(dxl_op);
			}
	};
}
#endif // !GPDXL_CDXLPhysicalCTAS_H

// EOF
