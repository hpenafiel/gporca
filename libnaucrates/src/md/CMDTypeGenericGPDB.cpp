//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CMDTypeGenericGPDB.cpp
//
//	@doc:
//		Implementation of the class for representing GPDB generic types
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/CMDTypeGenericGPDB.h"
#include "naucrates/md/CGPDBTypeHelper.h"

#include "naucrates/base/CDatumGenericGPDB.h"

#include "naucrates/dxl/operators/CDXLScalarConstValue.h"
#include "naucrates/dxl/operators/CDXLDatumStatsDoubleMappable.h"
#include "naucrates/dxl/operators/CDXLDatumStatsLintMappable.h"
#include "naucrates/dxl/operators/CDXLDatumGeneric.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpnaucrates;
using namespace gpdxl;
using namespace gpmd;

#define GPDB_ANYELEMENT_OID OID(2283) // oid of GPDB ANYELEMENT type

#define GPDB_ANYARRAY_OID OID(2277) // oid of GPDB ANYARRAY type

#define GPDB_ANYNONARRAY_OID OID(2776) // oid of GPDB ANYNONARRAY type

#define GPDB_ANYENUM_OID OID(3500) // oid of GPDB ANYENUM type

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeGenericGPDB::CMDTypeGenericGPDB
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDTypeGenericGPDB::CMDTypeGenericGPDB
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid,
	CMDName *pmdname,
	BOOL fRedistributable,
	BOOL fFixedLength,
	ULONG length, 
	BOOL fByValue,
	IMDId *pmdidOpEq,
	IMDId *pmdidOpNeq,
	IMDId *pmdidOpLT,
	IMDId *pmdidOpLEq,
	IMDId *pmdidOpGT,
	IMDId *pmdidOpGEq,
	IMDId *pmdidOpComp,
	IMDId *pmdidMin,
	IMDId *pmdidMax,
	IMDId *pmdidAvg,
	IMDId *pmdidSum,
	IMDId *pmdidCount,
	BOOL fHashable,
	BOOL fComposite,
	IMDId *pmdidBaseRelation,
	IMDId *pmdidTypeArray,
	INT iLength
	)
	:
	m_memory_pool(memory_pool),
	m_pmdid(pmdid),
	m_pmdname(pmdname),
	m_fRedistributable(fRedistributable),
	m_fFixedLength(fFixedLength),
	m_length(length),
	m_fByValue(fByValue),
	m_pmdidOpEq(pmdidOpEq),
	m_pmdidOpNeq(pmdidOpNeq),
	m_pmdidOpLT(pmdidOpLT),
	m_pmdidOpLEq(pmdidOpLEq),
	m_pmdidOpGT(pmdidOpGT),
	m_pmdidOpGEq(pmdidOpGEq),
	m_pmdidOpComp(pmdidOpComp),
	m_pmdidMin(pmdidMin),
	m_pmdidMax(pmdidMax),
	m_pmdidAvg(pmdidAvg),
	m_pmdidSum(pmdidSum),
	m_pmdidCount(pmdidCount),
	m_fHashable(fHashable),
	m_fComposite(fComposite),
	m_pmdidBaseRelation(pmdidBaseRelation),
	m_pmdidTypeArray(pmdidTypeArray),
	m_iLength(iLength),
	m_pdatumNull(NULL)
{
	GPOS_ASSERT_IMP(m_fFixedLength, 0 < m_length);
	GPOS_ASSERT_IMP(!m_fFixedLength, 0 > m_iLength);
	m_pstr = CDXLUtils::SerializeMDObj(m_memory_pool, this, false /*fSerializeHeader*/, false /*indentation*/);

	m_pmdid->AddRef();
	m_pdatumNull = GPOS_NEW(m_memory_pool) CDatumGenericGPDB(m_memory_pool, m_pmdid, IDefaultTypeModifier, NULL /*pba*/, 0 /*length*/, true /*fConstNull*/, 0 /*lValue */, 0 /*dValue */);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeGenericGPDB::~CMDTypeGenericGPDB
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDTypeGenericGPDB::~CMDTypeGenericGPDB()
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
	CRefCount::SafeRelease(m_pmdidBaseRelation);
	GPOS_DELETE(m_pmdname);
	GPOS_DELETE(m_pstr);
	m_pdatumNull->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CMDTypeGenericGPDB::PmdidAgg
//
//	@doc:
//		Return mdid of specified aggregate type
//
//---------------------------------------------------------------------------
IMDId *
CMDTypeGenericGPDB::PmdidAgg
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
//		CMDTypeGenericGPDB::MDId
//
//	@doc:
//		Returns the metadata id of this type
//
//---------------------------------------------------------------------------
IMDId *
CMDTypeGenericGPDB::MDId() const
{
	return m_pmdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeGenericGPDB::Mdname
//
//	@doc:
//		Returns the name of this type
//
//---------------------------------------------------------------------------
CMDName
CMDTypeGenericGPDB::Mdname() const
{
	return *m_pmdname;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeGenericGPDB::PmdidCmp
//
//	@doc:
//		Return mdid of specified comparison operator type
//
//---------------------------------------------------------------------------
IMDId * 
CMDTypeGenericGPDB::PmdidCmp
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
//		CMDTypeGenericGPDB::Serialize
//
//	@doc:
//		Serialize metadata in DXL format
//
//---------------------------------------------------------------------------
void
CMDTypeGenericGPDB::Serialize
	(
	CXMLSerializer *xml_serializer
	) 
	const
{
	CGPDBTypeHelper<CMDTypeGenericGPDB>::Serialize(xml_serializer, this);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeGenericGPDB::Pdatum
//
//	@doc:
//		Factory method for generating generic datum from CDXLScalarConstValue
//
//---------------------------------------------------------------------------
IDatum*
CMDTypeGenericGPDB::Pdatum
	(
	const CDXLScalarConstValue *pdxlop
	)
	const
{
	CDXLDatumGeneric *pdxldatum = CDXLDatumGeneric::Cast(const_cast<CDXLDatum*>(pdxlop->Pdxldatum()));
	GPOS_ASSERT(NULL != pdxlop);

	LINT lValue = 0;
	if (pdxldatum->FHasStatsLINTMapping())
	{
		lValue = pdxldatum->LStatsMapping();
	}

	CDouble dValue = 0;
	if (pdxldatum->FHasStatsDoubleMapping())
	{
		dValue = pdxldatum->DStatsMapping();
	}

	m_pmdid->AddRef();
	return GPOS_NEW(m_memory_pool) CDatumGenericGPDB(m_memory_pool, m_pmdid, pdxldatum->TypeModifier(), pdxldatum->Pba(), pdxldatum->Length(),
											 pdxldatum->IsNull(), lValue, dValue);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeGenericGPDB::Pdatum
//
//	@doc:
//		Construct a datum from a DXL datum
//
//---------------------------------------------------------------------------
IDatum*
CMDTypeGenericGPDB::Pdatum
	(
	IMemoryPool *memory_pool,
	const CDXLDatum *pdxldatum
	)
	const
{
	m_pmdid->AddRef();
	CDXLDatumGeneric *pdxldatumGeneric = CDXLDatumGeneric::Cast(const_cast<CDXLDatum *>(pdxldatum));

	LINT lValue = 0;
	if (pdxldatumGeneric->FHasStatsLINTMapping())
	{
		lValue = pdxldatumGeneric->LStatsMapping();
	}

	CDouble dValue = 0;
	if (pdxldatumGeneric->FHasStatsDoubleMapping())
	{
		dValue = pdxldatumGeneric->DStatsMapping();
	}

	return GPOS_NEW(m_memory_pool) CDatumGenericGPDB
						(
						memory_pool,
						m_pmdid,
						pdxldatumGeneric->TypeModifier(),
						pdxldatumGeneric->Pba(),
						pdxldatumGeneric->Length(),
						pdxldatumGeneric->IsNull(),
						lValue,
						dValue
						);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeGenericGPDB::Pdxldatum
//
//	@doc:
// 		Generate dxl datum
//
//---------------------------------------------------------------------------
CDXLDatum *
CMDTypeGenericGPDB::Pdxldatum
	(
	IMemoryPool *memory_pool,
	IDatum *pdatum
	)
	const
{
	m_pmdid->AddRef();
	CDatumGenericGPDB *pdatumgeneric = dynamic_cast<CDatumGenericGPDB*>(pdatum);
	ULONG length = 0;
	BYTE *pba = NULL;
	if (!pdatumgeneric->IsNull())
	{
		pba = pdatumgeneric->PbaVal(memory_pool, &length);
	}

	LINT lValue = 0;
	if (pdatumgeneric->FHasStatsLINTMapping())
	{
		lValue = pdatumgeneric->LStatsMapping();
	}

	CDouble dValue = 0;
	if (pdatumgeneric->FHasStatsDoubleMapping())
	{
		dValue = pdatumgeneric->DStatsMapping();
	}

	return Pdxldatum(memory_pool, m_pmdid, pdatumgeneric->TypeModifier(), m_fByValue, pdatumgeneric->IsNull(), pba, length, lValue, dValue);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeGenericGPDB::FAmbiguous
//
//	@doc:
// 		Is type an ambiguous one? I.e. a "polymorphic" type in GPDB terms
//
//---------------------------------------------------------------------------
BOOL
CMDTypeGenericGPDB::FAmbiguous() const
{
	OID oid = CMDIdGPDB::PmdidConvert(m_pmdid)->OidObjectId();
	// This should match the IsPolymorphicType() macro in GPDB's pg_type.h
	return (GPDB_ANYELEMENT_OID == oid ||
		GPDB_ANYARRAY_OID == oid ||
		GPDB_ANYNONARRAY_OID == oid ||
		GPDB_ANYENUM_OID == oid);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeGenericGPDB::Pdxldatum
//
//	@doc:
// 		Create a dxl datum
//
//---------------------------------------------------------------------------
CDXLDatum *
CMDTypeGenericGPDB::Pdxldatum
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid,
	INT type_modifier,
	BOOL fByVal,
	BOOL is_null,
	BYTE *pba,
	ULONG length,
	LINT lValue,
	CDouble dValue
	)
{
	GPOS_ASSERT(IMDId::EmdidGPDB == pmdid->Emdidt());

	const CMDIdGPDB * const pmdidGPDB = CMDIdGPDB::PmdidConvert(pmdid);
	switch (pmdidGPDB->OidObjectId())
	{
		// numbers
		case GPDB_NUMERIC:
		case GPDB_FLOAT4:
		case GPDB_FLOAT8:
			return CMDTypeGenericGPDB::PdxldatumStatsDoubleMappable(memory_pool, pmdid, type_modifier, fByVal, is_null, pba,
																	length, lValue, dValue);
		// has lint mapping
		case GPDB_CHAR:
		case GPDB_VARCHAR:
		case GPDB_TEXT:
		case GPDB_CASH:
			return CMDTypeGenericGPDB::PdxldatumStatsLintMappable(memory_pool, pmdid, type_modifier, fByVal, is_null, pba, length, lValue, dValue);
		// time-related types
		case GPDB_DATE:
		case GPDB_TIME:
		case GPDB_TIMETZ:
		case GPDB_TIMESTAMP:
		case GPDB_TIMESTAMPTZ:
		case GPDB_ABSTIME:
		case GPDB_RELTIME:
		case GPDB_INTERVAL:
		case GPDB_TIMEINTERVAL:
			return CMDTypeGenericGPDB::PdxldatumStatsDoubleMappable(memory_pool, pmdid, type_modifier, fByVal, is_null, pba,
																	length, lValue, dValue);
		// network-related types
		case GPDB_INET:
		case GPDB_CIDR:
		case GPDB_MACADDR:
			return CMDTypeGenericGPDB::PdxldatumStatsDoubleMappable(memory_pool, pmdid, type_modifier, fByVal, is_null, pba, length, lValue, dValue);
		default:
			return GPOS_NEW(memory_pool) CDXLDatumGeneric(memory_pool, pmdid, type_modifier, fByVal, is_null, pba, length);
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CMDTypeGenericGPDB::PdxldatumStatsDoubleMappable
//
//	@doc:
// 		Create a dxl datum of types that need double mapping
//
//---------------------------------------------------------------------------
CDXLDatum *
CMDTypeGenericGPDB::PdxldatumStatsDoubleMappable
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid,
	INT type_modifier,
	BOOL fByValue,
	BOOL is_null,
	BYTE *pba,
	ULONG length,
	LINT ,
	CDouble dValue
	)
{
	GPOS_ASSERT(CMDTypeGenericGPDB::FHasByteDoubleMapping(pmdid));
	return GPOS_NEW(memory_pool) CDXLDatumStatsDoubleMappable(memory_pool, pmdid, type_modifier, fByValue, is_null, pba, length, dValue);
}


//---------------------------------------------------------------------------
//	@function:
//		CMDTypeGenericGPDB::PdxldatumStatsLintMappable
//
//	@doc:
// 		Create a dxl datum of types having lint mapping
//
//---------------------------------------------------------------------------
CDXLDatum *
CMDTypeGenericGPDB::PdxldatumStatsLintMappable
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid,
	INT type_modifier,
	BOOL fByValue,
	BOOL is_null,
	BYTE *pba,
	ULONG length,
	LINT lValue,
	CDouble // dValue
	)
{
	GPOS_ASSERT(CMDTypeGenericGPDB::FHasByteLintMapping(pmdid));
	return GPOS_NEW(memory_pool) CDXLDatumStatsLintMappable(memory_pool, pmdid, type_modifier, fByValue, is_null, pba, length, lValue);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeGenericGPDB::PdxlopScConst
//
//	@doc:
// 		Generate a dxl scalar constant from a datum
//
//---------------------------------------------------------------------------
CDXLScalarConstValue *
CMDTypeGenericGPDB::PdxlopScConst
	(
	IMemoryPool *memory_pool,
	IDatum *pdatum
	)
	const
{
	CDXLDatum *pdxldatum = Pdxldatum(memory_pool, pdatum);

	return GPOS_NEW(memory_pool) CDXLScalarConstValue(memory_pool, pdxldatum);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeGenericGPDB::PdxldatumNull
//
//	@doc:
// 		Generate dxl datum
//
//---------------------------------------------------------------------------
CDXLDatum *
CMDTypeGenericGPDB::PdxldatumNull
	(
	IMemoryPool *memory_pool
	)
	const
{
	m_pmdid->AddRef();

	return Pdxldatum(memory_pool, m_pmdid, IDefaultTypeModifier, m_fByValue, true /*fConstNull*/, NULL /*pba*/, 0 /*length*/, 0 /*lValue */, 0 /*dValue */);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeGenericGPDB::FHasByteLintMapping
//
//	@doc:
//		Does the datum of this type need bytea -> Lint mapping for
//		statistics computation
//---------------------------------------------------------------------------
BOOL
CMDTypeGenericGPDB::FHasByteLintMapping
	(
	const IMDId *pmdid
	)
{
	return pmdid->Equals(&CMDIdGPDB::m_mdidBPChar)
			|| pmdid->Equals(&CMDIdGPDB::m_mdidVarChar)
			|| pmdid->Equals(&CMDIdGPDB::m_mdidText)
			|| pmdid->Equals(&CMDIdGPDB::m_mdidCash);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeGenericGPDB::FHasByteDoubleMapping
//
//	@doc:
//		Does the datum of this type need bytea -> double mapping for
//		statistics computation
//---------------------------------------------------------------------------
BOOL
CMDTypeGenericGPDB::FHasByteDoubleMapping
	(
	const IMDId *pmdid
	)
{
	return pmdid->Equals(&CMDIdGPDB::m_mdidNumeric)
			|| pmdid->Equals(&CMDIdGPDB::m_mdidFloat4)
			|| pmdid->Equals(&CMDIdGPDB::m_mdidFloat8)
			|| FTimeRelatedType(pmdid)
			|| FNetworkRelatedType(pmdid);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeGenericGPDB::FTimeRelatedType
//
//	@doc:
//		is this a time-related type
//---------------------------------------------------------------------------
BOOL
CMDTypeGenericGPDB::FTimeRelatedType
	(
	const IMDId *pmdid
	)
{
	return pmdid->Equals(&CMDIdGPDB::m_mdidDate)
			|| pmdid->Equals(&CMDIdGPDB::m_mdidTime)
			|| pmdid->Equals(&CMDIdGPDB::m_mdidTimeTz)
			|| pmdid->Equals(&CMDIdGPDB::m_mdidTimestamp)
			|| pmdid->Equals(&CMDIdGPDB::m_mdidTimestampTz)
			|| pmdid->Equals(&CMDIdGPDB::m_mdidAbsTime)
			|| pmdid->Equals(&CMDIdGPDB::m_mdidRelTime)
			|| pmdid->Equals(&CMDIdGPDB::m_mdidInterval)
			|| pmdid->Equals(&CMDIdGPDB::m_mdidTimeInterval);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeGenericGPDB::FNetworkRelatedType
//
//	@doc:
//		is this a network-related type
//---------------------------------------------------------------------------
BOOL
CMDTypeGenericGPDB::FNetworkRelatedType
	(
	const IMDId *pmdid
	)
{
	return pmdid->Equals(&CMDIdGPDB::m_mdidInet)
			|| pmdid->Equals(&CMDIdGPDB::m_mdidCidr)
			|| pmdid->Equals(&CMDIdGPDB::m_mdidMacaddr);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CMDTypeGenericGPDB::DebugPrint
//
//	@doc:
//		Prints a metadata cache relation to the provided output
//
//---------------------------------------------------------------------------
void
CMDTypeGenericGPDB::DebugPrint
	(
	IOstream &os
	)
	const
{
	CGPDBTypeHelper<CMDTypeGenericGPDB>::DebugPrint(os, this);
}

#endif // GPOS_DEBUG

// EOF

