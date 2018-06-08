//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CMDRelationCtasGPDB.h
//
//	@doc:
//		Class representing MD CTAS relations
//---------------------------------------------------------------------------

#ifndef GPMD_CMDRelationCTASGPDB_H
#define GPMD_CMDRelationCTASGPDB_H

#include "gpos/base.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/IMDRelationCtas.h"
#include "naucrates/md/IMDColumn.h"
#include "naucrates/md/CMDColumn.h"
#include "naucrates/md/CMDName.h"

namespace gpdxl
{
	// fwd decl
	class CXMLSerializer;
	class CDXLCtasStorageOptions;
}

namespace gpmd
{
	using namespace gpos;
	using namespace gpdxl;


	//---------------------------------------------------------------------------
	//	@class:
	//		CMDRelationCtasGPDB
	//
	//	@doc:
	//		Class representing MD CTAS relations
	//
	//---------------------------------------------------------------------------
	class CMDRelationCtasGPDB : public IMDRelationCtas
	{
		private:
			// memory pool
			IMemoryPool *m_memory_pool;

			// DXL for object
			const CWStringDynamic *m_pstr;

			// relation mdid
			IMDId *m_mdid;

			// schema name
			CMDName *m_mdname_schema;
			
			// table name
			CMDName *m_mdname;
			
			// is this a temporary relation
			BOOL m_is_temp_table;
			
			// does this table have oids
			BOOL m_has_oids;
			
			// storage type
			Erelstoragetype m_rel_storage_type;

			// distribution policy
			GetRelDistrPolicy m_rel_distr_policy;

			// columns
			DrgPmdcol *m_md_col_array;

			// indices of distribution columns
			ULongPtrArray *m_distr_col_array;
			
			// array of key sets
			ULongPtrArray2D *m_pdrgpdrgpulKeys;

			// number of system columns
			ULONG m_ulSystemColumns;
			
			// mapping of attribute number in the system catalog to the positions of
			// the non dropped column in the metadata object
			HMIUl *m_phmiulAttno2Pos;

			// the original positions of all the non-dropped columns
			ULongPtrArray *m_pdrgpulNonDroppedCols;
			
			// storage options
			CDXLCtasStorageOptions *m_dxl_ctas_storage_option;

			// vartypemod list
			IntPtrArray *m_vartypemod_array;

			// array of column widths
			DrgPdouble *m_pdrgpdoubleColWidths;

			// private copy ctor
			CMDRelationCtasGPDB(const CMDRelationCtasGPDB &);

		public:

			// ctor
			CMDRelationCtasGPDB
				(
				IMemoryPool *memory_pool,
				IMDId *mdid,
				CMDName *mdname_schema,
				CMDName *mdname,
				BOOL fTemporary,
				BOOL fHasOids,
				Erelstoragetype rel_storage_type,
				GetRelDistrPolicy rel_distr_policy,
				DrgPmdcol *pdrgpmdcol,
				ULongPtrArray *pdrgpulDistrColumns,
				ULongPtrArray2D *pdrgpdrgpulKeys,
				CDXLCtasStorageOptions *dxl_ctas_storage_options,
				IntPtrArray *pdrgpiVarTypeMod
				);

			// dtor
			virtual
			~CMDRelationCtasGPDB();

			// accessors
			virtual
			const CWStringDynamic *Pstr() const
			{
				return m_pstr;
			}

			// the metadata id
			virtual
			IMDId *MDId() const;

			// schema name
			virtual
			CMDName *GetMdNameSchema() const;
			
			// relation name
			virtual
			CMDName Mdname() const;

			// distribution policy (none, hash, random)
			virtual
			GetRelDistrPolicy Ereldistribution() const;

			// does this table have oids
			virtual
			BOOL HasOids() const
			{
				return m_has_oids;
			}
			
			// is this a temp relation
			virtual 
			BOOL IsTemporary() const
			{
				return m_is_temp_table;
			}
			
			// storage type
			virtual 
			Erelstoragetype GetRelStorageType() const
			{
				return m_rel_storage_type;
			}
			
			// CTAS storage options
			virtual 
			CDXLCtasStorageOptions *GetDxlCtasStorageOption() const
			{
				return m_dxl_ctas_storage_option;
			}
			
			// number of columns
			virtual
			ULONG UlColumns() const;

			// width of a column with regards to the position
			virtual
			DOUBLE DColWidth(ULONG ulPos) const;

			// does relation have dropped columns
			virtual
			BOOL FHasDroppedColumns() const
			{
				return false;
			}

			// number of non-dropped columns
			virtual 
			ULONG UlNonDroppedCols() const
			{
				return UlColumns();
			}
			
			// return the original positions of all the non-dropped columns
			virtual
			ULongPtrArray *PdrgpulNonDroppedCols() const
			{
				return m_pdrgpulNonDroppedCols;
			}

			// number of system columns
			virtual
			ULONG UlSystemColumns() const;

			// retrieve the column at the given position
			virtual
			const IMDColumn *GetMdCol(ULONG ulPos) const;

			// number of distribution columns
			virtual
			ULONG UlDistrColumns() const;

			// retrieve the column at the given position in the distribution columns list for the relation
			virtual
			const IMDColumn *PmdcolDistrColumn(ULONG ulPos) const;

			// number of indices
			virtual
			ULONG UlIndices() const
			{
				return 0;
			}

			// number of triggers
			virtual
			ULONG UlTriggers() const
			{
				return 0;
			}
			
			// return the absolute position of the given attribute position excluding dropped columns
			virtual 
			ULONG UlPosNonDropped(ULONG ulPos) const
			{
				return ulPos;
			}
			
			 // return the position of a column in the metadata object given the attribute number in the system catalog
			virtual
			ULONG UlPosFromAttno(INT attno) const;

			// retrieve the id of the metadata cache index at the given position
			virtual
			IMDId *PmdidIndex
				(
				ULONG // ulPos
				) 
				const
			{
				GPOS_ASSERT("CTAS tables have no indexes"); 
				return 0;
			}

			// retrieve the id of the metadata cache trigger at the given position
			virtual
			IMDId *PmdidTrigger
				(
				ULONG // ulPos
				) 
				const
			{
				GPOS_ASSERT("CTAS tables have no triggers"); 
				return 0;
			}

			// serialize metadata relation in DXL format given a serializer object
			virtual
			void Serialize(gpdxl::CXMLSerializer *) const;

			// number of check constraints
			virtual
			ULONG UlCheckConstraints() const
			{
				return 0;
			}

			// retrieve the id of the check constraint cache at the given position
			virtual
			IMDId *PmdidCheckConstraint
				(
				ULONG // ulPos
				) 
				const
			{
				GPOS_ASSERT("CTAS tables have no constraints"); 
				return 0;
			}

			// list of vartypmod for target expressions
			IntPtrArray *GetVarTypeModArray() const
			{
				return m_vartypemod_array;
			}

#ifdef GPOS_DEBUG
			// debug print of the metadata relation
			virtual
			void DebugPrint(IOstream &os) const;
#endif
	};
}

#endif // !GPMD_CMDRelationCTASGPDB_H

// EOF
