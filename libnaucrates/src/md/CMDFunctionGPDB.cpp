//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CMDFunctionGPDB.cpp
//
//	@doc:
//		Implementation of the class for representing GPDB-specific functions
//		in the MD cache
//---------------------------------------------------------------------------


#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/CMDFunctionGPDB.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpmd;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CMDFunctionGPDB::CMDFunctionGPDB
//
//	@doc:
//		Constructs a metadata func
//
//---------------------------------------------------------------------------
CMDFunctionGPDB::CMDFunctionGPDB
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid,
	CMDName *mdname,
	IMDId *pmdidTypeResult,
	DrgPmdid *pdrgpmdidTypes,
	BOOL fReturnsSet,
	EFuncStbl efsStability,
	EFuncDataAcc efdaDataAccess,
	BOOL fStrict
	)
	:
	m_memory_pool(memory_pool),
	m_mdid(pmdid),
	m_mdname(mdname),
	m_pmdidTypeResult(pmdidTypeResult),
	m_pdrgpmdidTypes(pdrgpmdidTypes),
	m_fReturnsSet(fReturnsSet),
	m_efsStability(efsStability),
	m_efdaDataAccess(efdaDataAccess),
	m_fStrict(fStrict)
{
	GPOS_ASSERT(m_mdid->IsValid());
	GPOS_ASSERT(EfsSentinel > efsStability);
	GPOS_ASSERT(EfdaSentinel > efdaDataAccess);

	InitDXLTokenArrays();
	m_pstr = CDXLUtils::SerializeMDObj(m_memory_pool, this, false /*fSerializeHeader*/, false /*indentation*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDFunctionGPDB::~CMDFunctionGPDB
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CMDFunctionGPDB::~CMDFunctionGPDB()
{
	m_mdid->Release();
	m_pmdidTypeResult->Release();
	CRefCount::SafeRelease(m_pdrgpmdidTypes);
	GPOS_DELETE(m_mdname);
	GPOS_DELETE(m_pstr);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDFunctionGPDB::InitDXLTokenArrays
//
//	@doc:
//		Initialize DXL token arrays
//
//---------------------------------------------------------------------------
void
CMDFunctionGPDB::InitDXLTokenArrays()
{
	// stability
	m_drgDxlStability[EfsImmutable] = EdxltokenGPDBFuncImmutable;
	m_drgDxlStability[EfsStable] = EdxltokenGPDBFuncStable;
	m_drgDxlStability[EfsVolatile] = EdxltokenGPDBFuncVolatile;

	// data access
	m_drgDxlDataAccess[EfdaNoSQL] = EdxltokenGPDBFuncNoSQL;
	m_drgDxlDataAccess[EfdaContainsSQL] = EdxltokenGPDBFuncContainsSQL;
	m_drgDxlDataAccess[EfdaReadsSQLData] = EdxltokenGPDBFuncReadsSQLData;
	m_drgDxlDataAccess[EfdaModifiesSQLData] = EdxltokenGPDBFuncModifiesSQLData;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDFunctionGPDB::MDId
//
//	@doc:
//		Func id
//
//---------------------------------------------------------------------------
IMDId *
CMDFunctionGPDB::MDId() const
{
	return m_mdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDFunctionGPDB::Mdname
//
//	@doc:
//		Func name
//
//---------------------------------------------------------------------------
CMDName
CMDFunctionGPDB::Mdname() const
{
	return *m_mdname;
}


//---------------------------------------------------------------------------
//	@function:
//		CMDFunctionGPDB::PmdidTypeResult
//
//	@doc:
//		Type id of result
//
//---------------------------------------------------------------------------
IMDId *
CMDFunctionGPDB::PmdidTypeResult() const
{
	return m_pmdidTypeResult;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDFunctionGPDB::PdrgpmdidOutputArgTypes
//
//	@doc:
//		Output argument types
//
//---------------------------------------------------------------------------
DrgPmdid *
CMDFunctionGPDB::PdrgpmdidOutputArgTypes() const
{
	return m_pdrgpmdidTypes;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDFunctionGPDB::FReturnsSet
//
//	@doc:
//		Returns whether function result is a set
//
//---------------------------------------------------------------------------
BOOL
CMDFunctionGPDB::FReturnsSet() const
{
	return m_fReturnsSet;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDFunctionGPDB::PstrOutArgTypes
//
//	@doc:
//		Serialize the array of output argument types into a comma-separated string
//
//---------------------------------------------------------------------------
CWStringDynamic *
CMDFunctionGPDB::PstrOutArgTypes() const
{
	GPOS_ASSERT(NULL != m_pdrgpmdidTypes);
	CWStringDynamic *pstr = GPOS_NEW(m_memory_pool) CWStringDynamic(m_memory_pool);

	const ULONG ulLen = m_pdrgpmdidTypes->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		IMDId *pmdid = (*m_pdrgpmdidTypes)[ul];
		if (ul == ulLen - 1)
		{
			// last element: do not print a comma
			pstr->AppendFormat(GPOS_WSZ_LIT("%ls"), pmdid->GetBuffer());
		}
		else
		{
			pstr->AppendFormat(GPOS_WSZ_LIT("%ls%ls"), pmdid->GetBuffer(), CDXLTokens::GetDXLTokenStr(EdxltokenComma)->GetBuffer());
		}
	}

	return pstr;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDFunctionGPDB::Serialize
//
//	@doc:
//		Serialize function metadata in DXL format
//
//---------------------------------------------------------------------------
void
CMDFunctionGPDB::Serialize
	(
	CXMLSerializer *xml_serializer
	) 
	const
{
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
						CDXLTokens::GetDXLTokenStr(EdxltokenGPDBFunc));
	
	m_mdid->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMdid));

	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenName), m_mdname->GetMDName());
	
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenGPDBFuncReturnsSet), m_fReturnsSet);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenGPDBFuncStability), PstrStability());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenGPDBFuncDataAccess), PstrDataAccess());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenGPDBFuncStrict), m_fStrict);

	SerializeMDIdAsElem(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenGPDBFuncResultTypeId), m_pmdidTypeResult);

	if (NULL != m_pdrgpmdidTypes)
	{
		xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
							CDXLTokens::GetDXLTokenStr(EdxltokenOutputCols));

		CWStringDynamic *pstrOutArgTypes = PstrOutArgTypes();
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenTypeIds), pstrOutArgTypes);
		GPOS_DELETE(pstrOutArgTypes);

		xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
							CDXLTokens::GetDXLTokenStr(EdxltokenOutputCols));

	}
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
						CDXLTokens::GetDXLTokenStr(EdxltokenGPDBFunc));
}

//---------------------------------------------------------------------------
//	@function:
//		CMDFunctionGPDB::PstrStability
//
//	@doc:
//		String representation of function stability
//
//---------------------------------------------------------------------------
const CWStringConst *
CMDFunctionGPDB::PstrStability() const
{
	if (EfsSentinel > m_efsStability)
	{
		return CDXLTokens::GetDXLTokenStr(m_drgDxlStability[m_efsStability]);
	}

	GPOS_ASSERT(!"Unrecognized function stability setting");
	return NULL;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDFunctionGPDB::PstrDataAccess
//
//	@doc:
//		String representation of function data access
//
//---------------------------------------------------------------------------
const CWStringConst *
CMDFunctionGPDB::PstrDataAccess() const
{
	if (EfdaSentinel > m_efdaDataAccess)
	{
		return CDXLTokens::GetDXLTokenStr(m_drgDxlDataAccess[m_efdaDataAccess]);
	}

	GPOS_ASSERT(!"Unrecognized function data access setting");
	return NULL;
}

#ifdef GPOS_DEBUG

//---------------------------------------------------------------------------
//	@function:
//		CMDFunctionGPDB::DebugPrint
//
//	@doc:
//		Prints a metadata cache relation to the provided output
//
//---------------------------------------------------------------------------
void
CMDFunctionGPDB::DebugPrint
	(
	IOstream &os
	)
	const
{
	os << "Function id: ";
	MDId()->OsPrint(os);
	os << std::endl;
	
	os << "Function name: " << (Mdname()).GetMDName()->GetBuffer() << std::endl;
	
	os << "Result type id: ";
	PmdidTypeResult()->OsPrint(os);
	os << std::endl;
	
	const CWStringConst *pstrReturnsSet = FReturnsSet() ? 
			CDXLTokens::GetDXLTokenStr(EdxltokenTrue): 
			CDXLTokens::GetDXLTokenStr(EdxltokenFalse); 

	os << "Returns set: " << pstrReturnsSet->GetBuffer() << std::endl;

	os << "Function is " << PstrStability()->GetBuffer() << std::endl;
	
	os << "Data access: " << PstrDataAccess()->GetBuffer() << std::endl;

	const CWStringConst *pstrIsStrict = FStrict() ? 
			CDXLTokens::GetDXLTokenStr(EdxltokenTrue): 
			CDXLTokens::GetDXLTokenStr(EdxltokenFalse); 

	os << "Is strict: " << pstrIsStrict->GetBuffer() << std::endl;
	
	os << std::endl;	
}

#endif // GPOS_DEBUG

// EOF
