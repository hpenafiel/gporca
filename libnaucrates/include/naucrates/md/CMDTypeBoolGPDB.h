//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CMDTypeBoolGPDB.h
//
//	@doc:
//		Class for representing BOOL types in GPDB
//---------------------------------------------------------------------------



#ifndef GPMD_CMDTypeBoolGPDB_H
#define GPMD_CMDTypeBoolGPDB_H

#include "gpos/base.h"

#include "naucrates/md/IMDTypeBool.h"
#include "naucrates/base/IDatumBool.h"

#define GPDB_BOOL_OID OID(16)
#define GPDB_BOOL_LENGTH 1
#define GPDB_BOOL_EQ_OP OID(91)
#define GPDB_BOOL_NEQ_OP OID(85)
#define GPDB_BOOL_LT_OP OID(58)
#define GPDB_BOOL_LEQ_OP OID(1694)
#define GPDB_BOOL_GT_OP OID(59)
#define GPDB_BOOL_GEQ_OP OID(1695)
#define GPDB_BOOL_COMP_OP OID(1693)
#define GPDB_BOOL_ARRAY_TYPE OID(1000)
#define GPDB_BOOL_AGG_MIN OID(0)
#define GPDB_BOOL_AGG_MAX OID(0)
#define GPDB_BOOL_AGG_AVG OID(0)
#define GPDB_BOOL_AGG_SUM OID(0)
#define GPDB_BOOL_AGG_COUNT OID(2147)

// fwd decl
namespace gpdxl
{
	class CXMLSerializer;
}

namespace gpmd
{
	using namespace gpos;
	using namespace gpnaucrates;

	
	//---------------------------------------------------------------------------
	//	@class:
	//		CMDTypeBoolGPDB
	//
	//	@doc:
	//		Class for representing BOOL types in GPDB
	//
	//---------------------------------------------------------------------------
	class CMDTypeBoolGPDB : public IMDTypeBool
	{
	private:
	
		// memory pool
		IMemoryPool *m_memory_pool;
		
		// type id
		IMDId *m_mdid;
		
		// mdids of different operators
		IMDId *m_pmdidOpEq;
		IMDId *m_pmdidOpNeq;
		IMDId *m_pmdidOpLT;
		IMDId *m_pmdidOpLEq;
		IMDId *m_pmdidOpGT;
		IMDId *m_pmdidOpGEq;
		IMDId *m_pmdidOpComp;
		IMDId *m_pmdidTypeArray;
		
		// min aggregate
		IMDId *m_pmdidMin;
		
		// max aggregate
		IMDId *m_pmdidMax;
		
		// avg aggregate
		IMDId *m_pmdidAvg;
		
		// sum aggregate
		IMDId *m_pmdidSum;
		
		// count aggregate
		IMDId *m_pmdidCount;

		// DXL for object
		const CWStringDynamic *m_pstr;
		
		// type name and id
		static CWStringConst m_str;
		static CMDName m_mdname;

		// a null datum of this type (used for statistics comparison)
		IDatum *m_pdatumNull;

		// private copy ctor
		CMDTypeBoolGPDB(const CMDTypeBoolGPDB &);
		
	public:
		// ctor
		explicit 
		CMDTypeBoolGPDB(IMemoryPool *memory_pool);
		
		// dtor
		virtual 
		~CMDTypeBoolGPDB();
		
		// accessors
		virtual 
		const CWStringDynamic *Pstr() const
		{
			return m_pstr;
		}
		
		// type id
		virtual 
		IMDId *MDId() const;
		
		// type name
		virtual 
		CMDName Mdname() const;
		
		// is type redistributable
		virtual
		BOOL FRedistributable() const
		{
			return true;
		}
		
		// is type fixed length
		virtual
		BOOL FFixedLength() const
		{
			return true;
		}
		
		// is type composite
		virtual
		BOOL FComposite() const
		{
			return false;
		}

		// type length
		virtual
		ULONG Length() const
		{
			return GPDB_BOOL_LENGTH;
		}
		
		// is type passed by value
		virtual
		BOOL IsPassedByValue() const
		{
			return true;
		}
		
		// id of specified comparison operator type
		virtual 
		IMDId *PmdidCmp(ECmpType ecmpt) const;
		
		virtual 
		const IMDId *PmdidOpComp() const
		{
			return m_pmdidOpComp;
		}
		
		// id of specified specified aggregate type
		virtual 
		IMDId *PmdidAgg(EAggType eagg) const;
		
		// is type hashable
		virtual 
		BOOL FHashable() const
		{
			return true;
		}
		
		// array type id
		virtual 
		IMDId *PmdidTypeArray() const
		{
			return m_pmdidTypeArray;
		}
		
		// id of the relation corresponding to a composite type
		virtual
		IMDId *PmdidBaseRelation() const
		{
			return NULL;
		}

		// return the null constant for this type
		virtual
		IDatum *PdatumNull() const
		{
			return m_pdatumNull;
		}

		// factory method for creating constants
		virtual
		IDatumBool *PdatumBool(IMemoryPool *memory_pool, BOOL fValue, BOOL fNULL) const;
		
		// create typed datum from DXL datum
		virtual
		IDatum *Pdatum(IMemoryPool *memory_pool, const CDXLDatum *datum_dxl) const;

		// serialize object in DXL format
		virtual 
		void Serialize(gpdxl::CXMLSerializer *xml_serializer) const;

		// transformation function to generate datum from CDXLScalarConstValue
		virtual 
		IDatum* Pdatum(const CDXLScalarConstValue *dxl_op) const;
		
		// generate the DXL datum from IDatum
		virtual
		CDXLDatum* Pdxldatum(IMemoryPool *memory_pool, IDatum *pdatum) const;

		// generate the DXL scalar constant from IDatum
		virtual
		CDXLScalarConstValue* PdxlopScConst(IMemoryPool *memory_pool, IDatum *pdatum) const;

		// generate the DXL datum representing null value
		virtual
		CDXLDatum* PdxldatumNull(IMemoryPool *memory_pool) const;

#ifdef GPOS_DEBUG
		// debug print of the type in the provided stream
		virtual 
		void DebugPrint(IOstream &os) const;
#endif

	};
}

#endif // !GPMD_CMDTypeBoolGPDB_H

// EOF
