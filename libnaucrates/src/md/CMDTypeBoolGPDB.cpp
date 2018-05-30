//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CMDTypeBoolGPDB.cpp
//
//	@doc:
//		Implementation of the class for representing the GPDB bool types in the
//		MD cache
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/CMDTypeBoolGPDB.h"
#include "naucrates/md/CGPDBTypeHelper.h"

#include "naucrates/dxl/operators/CDXLScalarConstValue.h"
#include "naucrates/dxl/operators/CDXLDatumBool.h"
#include "naucrates/dxl/CDXLUtils.h"

#include "naucrates/base/CDatumBoolGPDB.h"

#include "naucrates/md/CMDIdGPDB.h"

using namespace gpdxl;
using namespace gpmd;
using namespace gpnaucrates;

// static member initialization 
CWStringConst
CMDTypeBoolGPDB::m_str = CWStringConst(GPOS_WSZ_LIT("bool"));
CMDName
CMDTypeBoolGPDB::m_mdname(&m_str);

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeBoolGPDB::CMDTypeBoolGPDB
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDTypeBoolGPDB::CMDTypeBoolGPDB
	(
	IMemoryPool *memory_pool
	)
	:
	m_memory_pool(memory_pool)
{
	m_pmdid = GPOS_NEW(memory_pool) CMDIdGPDB(GPDB_BOOL_OID);
	m_pmdidOpEq = GPOS_NEW(memory_pool) CMDIdGPDB(GPDB_BOOL_EQ_OP);
	m_pmdidOpNeq = GPOS_NEW(memory_pool) CMDIdGPDB(GPDB_BOOL_NEQ_OP);
	m_pmdidOpLT = GPOS_NEW(memory_pool) CMDIdGPDB(GPDB_BOOL_LT_OP);
	m_pmdidOpLEq = GPOS_NEW(memory_pool) CMDIdGPDB(GPDB_BOOL_LEQ_OP);
	m_pmdidOpGT = GPOS_NEW(memory_pool) CMDIdGPDB(GPDB_BOOL_GT_OP);
	m_pmdidOpGEq = GPOS_NEW(memory_pool) CMDIdGPDB(GPDB_BOOL_GEQ_OP);
	m_pmdidOpComp = GPOS_NEW(memory_pool) CMDIdGPDB(GPDB_BOOL_COMP_OP);
	m_pmdidTypeArray = GPOS_NEW(memory_pool) CMDIdGPDB(GPDB_BOOL_ARRAY_TYPE);
	
	m_pmdidMin = GPOS_NEW(memory_pool) CMDIdGPDB(GPDB_BOOL_AGG_MIN);
	m_pmdidMax = GPOS_NEW(memory_pool) CMDIdGPDB(GPDB_BOOL_AGG_MAX);
	m_pmdidAvg = GPOS_NEW(memory_pool) CMDIdGPDB(GPDB_BOOL_AGG_AVG);
	m_pmdidSum = GPOS_NEW(memory_pool) CMDIdGPDB(GPDB_BOOL_AGG_SUM);
	m_pmdidCount = GPOS_NEW(memory_pool) CMDIdGPDB(GPDB_BOOL_AGG_COUNT);

	m_pstr = CDXLUtils::SerializeMDObj(m_memory_pool, this, false /*fSerializeHeader*/, false /*indentation*/);

	m_pmdid->AddRef();

	GPOS_ASSERT(GPDB_BOOL_OID == CMDIdGPDB::PmdidConvert(m_pmdid)->OidObjectId());
	m_pdatumNull = GPOS_NEW(memory_pool) CDatumBoolGPDB(m_pmdid, false /* fVal */, true /* is_null */);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeBoolGPDB::~CMDTypeBoolGPDB
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDTypeBoolGPDB::~CMDTypeBoolGPDB()
{
	m_pmdid->Release();
	m_pmdidOpEq->Release();
	m_pmdidOpNeq->Release();
	m_pmdidOpLT->Release();
	m_pmdidOpLEq->Release();
	m_pmdidOpGT->Release();
	m_pmdidOpGEq->Release();
	m_pmdidOpComp->Release();
	m_pmdidTypeArray->Release();
	
	m_pmdidMin->Release();
	m_pmdidMax->Release();
	m_pmdidAvg->Release();
	m_pmdidSum->Release();
	m_pmdidCount->Release();
	m_pdatumNull->Release();
	GPOS_DELETE(m_pstr);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeBoolGPDB::PmdidCmp
//
//	@doc:
//		Return mdid of specified comparison operator type
//
//---------------------------------------------------------------------------
IMDId *
CMDTypeBoolGPDB::PmdidCmp
	(
	ECmpType ecmpt
	) 
	const
{
	switch (ecmpt)
	{
		case EcmptEq:
			return m_pmdidOpEq;
		case EcmptNEq:
			return m_pmdidOpNeq;
		case EcmptL:
			return m_pmdidOpLT;
		case EcmptLEq: 
			return m_pmdidOpLEq;
		case EcmptG:
			return m_pmdidOpGT;
		case EcmptGEq:
			return m_pmdidOpGEq;
		default:
			GPOS_ASSERT(!"Invalid operator type");
			return NULL;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeBoolGPDB::PmdidAgg
//
//	@doc:
//		Return mdid of specified aggregate type
//
//---------------------------------------------------------------------------
IMDId *
CMDTypeBoolGPDB::PmdidAgg
	(
	EAggType eagg
	) 
	const
{
	switch (eagg)
	{
		case EaggMin:
			return m_pmdidMin;
		case EaggMax:
			return m_pmdidMax;
		case EaggAvg:
			return m_pmdidAvg;
		case EaggSum:
			return m_pmdidSum;
		case EaggCount:
			return m_pmdidCount;
		default:
			GPOS_ASSERT(!"Invalid aggregate type");
			return NULL;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeBoolGPDB::Pdatum
//
//	@doc:
//		Factory function for creating BOOL datums
//
//---------------------------------------------------------------------------
IDatumBool *
CMDTypeBoolGPDB::PdatumBool
	(
	IMemoryPool *memory_pool, 
	BOOL fValue, 
	BOOL fNULL
	)
	const
{
	return GPOS_NEW(memory_pool) CDatumBoolGPDB(m_pmdid->Sysid(), fValue, fNULL);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeBoolGPDB::MDId
//
//	@doc:
//		Returns the metadata id of this type
//
//---------------------------------------------------------------------------
IMDId *
CMDTypeBoolGPDB::MDId() const
{
	return m_pmdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeBoolGPDB::Mdname
//
//	@doc:
//		Returns the name of this type
//
//---------------------------------------------------------------------------
CMDName
CMDTypeBoolGPDB::Mdname() const
{
	return CMDTypeBoolGPDB::m_mdname;;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeBoolGPDB::Serialize
//
//	@doc:
//		Serialize relation metadata in DXL format
//
//---------------------------------------------------------------------------
void
CMDTypeBoolGPDB::Serialize
	(
	CXMLSerializer *xml_serializer
	) 
	const
{
	CGPDBTypeHelper<CMDTypeBoolGPDB>::Serialize(xml_serializer, this);
}


//---------------------------------------------------------------------------
//	@function:
//		CMDTypeBoolGPDB::Pdatum
//
//	@doc:
//		Transformation function to generate bool datum from CDXLScalarConstValue
//
//---------------------------------------------------------------------------
IDatum *
CMDTypeBoolGPDB::Pdatum
	(
	const CDXLScalarConstValue *pdxlop
	)
	const
{
	CDXLDatumBool *pdxldatum = CDXLDatumBool::PdxldatumConvert(const_cast<CDXLDatum*>(pdxlop->Pdxldatum()));
	GPOS_ASSERT(pdxldatum->IsPassedByValue());

	return GPOS_NEW(m_memory_pool) CDatumBoolGPDB(m_pmdid->Sysid(), pdxldatum->FValue(), pdxldatum->IsNull());
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeBoolGPDB::Pdatum
//
//	@doc:
//		Construct a bool datum from a DXL datum
//
//---------------------------------------------------------------------------
IDatum*
CMDTypeBoolGPDB::Pdatum
	(
	IMemoryPool *memory_pool,
	const CDXLDatum *pdxldatum
	)
	const
{
	CDXLDatumBool *pdxldatumbool = CDXLDatumBool::PdxldatumConvert(const_cast<CDXLDatum *>(pdxldatum));
	GPOS_ASSERT(pdxldatumbool->IsPassedByValue());
	BOOL fVal = pdxldatumbool->FValue();
	BOOL is_null = pdxldatumbool->IsNull();
	
	return GPOS_NEW(memory_pool) CDatumBoolGPDB(m_pmdid->Sysid(), fVal, is_null);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeBoolGPDB::Pdxldatum
//
//	@doc:
// 		Generate dxl datum
//
//---------------------------------------------------------------------------
CDXLDatum *
CMDTypeBoolGPDB::Pdxldatum
	(
	IMemoryPool *memory_pool,
	IDatum *pdatum
	)
	const
{
	CDatumBoolGPDB *pdatumbool = dynamic_cast<CDatumBoolGPDB*>(pdatum);
	m_pmdid->AddRef();
	return GPOS_NEW(memory_pool) CDXLDatumBool(memory_pool, m_pmdid, pdatumbool->IsNull(), pdatumbool->FValue());
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeBoolGPDB::PdxlopScConst
//
//	@doc:
// 		Generate a dxl scalar constant from a datum
//
//---------------------------------------------------------------------------
CDXLScalarConstValue *
CMDTypeBoolGPDB::PdxlopScConst
	(
	IMemoryPool *memory_pool,
	IDatum *pdatum
	)
	const
{
	CDatumBoolGPDB *pdatumboolgpdb = dynamic_cast<CDatumBoolGPDB *>(pdatum);

	m_pmdid->AddRef();
	CDXLDatumBool *pdxldatum = GPOS_NEW(memory_pool) CDXLDatumBool(memory_pool, m_pmdid, pdatumboolgpdb->IsNull(), pdatumboolgpdb->FValue());

	return GPOS_NEW(memory_pool) CDXLScalarConstValue(memory_pool, pdxldatum);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeBoolGPDB::PdxldatumNull
//
//	@doc:
// 		Generate dxl datum representing a null value
//
//---------------------------------------------------------------------------
CDXLDatum *
CMDTypeBoolGPDB::PdxldatumNull
	(
	IMemoryPool *memory_pool
	)
	const
{
	m_pmdid->AddRef();

	return GPOS_NEW(memory_pool) CDXLDatumBool(memory_pool, m_pmdid, true /*is_null*/, false);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CMDTypeBoolGPDB::DebugPrint
//
//	@doc:
//		Prints a metadata cache relation to the provided output
//
//---------------------------------------------------------------------------
void
CMDTypeBoolGPDB::DebugPrint
	(
	IOstream &os
	)
	const
{
	CGPDBTypeHelper<CMDTypeBoolGPDB>::DebugPrint(os, this);
}

#endif // GPOS_DEBUG

// EOF

