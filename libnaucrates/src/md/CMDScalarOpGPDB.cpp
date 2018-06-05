//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CMDScalarOpGPDB.cpp
//
//	@doc:
//		Implementation of the class for representing GPDB-specific scalar ops
//		in the MD cache
//---------------------------------------------------------------------------


#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/CMDScalarOpGPDB.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::CMDScalarOpGPDB
//
//	@doc:
//		Constructs a metadata scalar op
//
//---------------------------------------------------------------------------
CMDScalarOpGPDB::CMDScalarOpGPDB
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid,
	CMDName *mdname,
	IMDId *pmdidTypeLeft,
	IMDId *pmdidTypeRight,
	IMDId *pmdidTypeResult,
	IMDId *pmdidFunc,
	IMDId *pmdidOpCommute,
	IMDId *pmdidOpInverse,
	IMDType::ECmpType ecmpt,
	BOOL fReturnsNullOnNullInput,
	DrgPmdid *pdrgpmdidOpClasses
	)
	:
	m_memory_pool(memory_pool),
	m_mdid(pmdid),
	m_mdname(mdname),
	m_pmdidTypeLeft(pmdidTypeLeft),
	m_pmdidTypeRight(pmdidTypeRight),
	m_pmdidTypeResult(pmdidTypeResult),
	m_func_mdid(pmdidFunc),
	m_pmdidOpCommute(pmdidOpCommute),
	m_pmdidOpInverse(pmdidOpInverse),
	m_ecmpt(ecmpt),
	m_fReturnsNullOnNullInput(fReturnsNullOnNullInput),
	m_pdrgpmdidOpClasses(pdrgpmdidOpClasses)
{
	GPOS_ASSERT(NULL != pdrgpmdidOpClasses);
	m_pstr = CDXLUtils::SerializeMDObj(m_memory_pool, this, false /*fSerializeHeader*/, false /*indentation*/);
}


//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::~CMDScalarOpGPDB
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDScalarOpGPDB::~CMDScalarOpGPDB()
{
	m_mdid->Release();
	m_pmdidTypeResult->Release();
	m_func_mdid->Release();	

	CRefCount::SafeRelease(m_pmdidTypeLeft);
	CRefCount::SafeRelease(m_pmdidTypeRight);
	CRefCount::SafeRelease(m_pmdidOpCommute);
	CRefCount::SafeRelease(m_pmdidOpInverse);
	
	GPOS_DELETE(m_mdname);
	GPOS_DELETE(m_pstr);
	m_pdrgpmdidOpClasses->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::MDId
//
//	@doc:
//		Operator id
//
//---------------------------------------------------------------------------
IMDId *
CMDScalarOpGPDB::MDId() const
{
	return m_mdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::Mdname
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
CMDName
CMDScalarOpGPDB::Mdname() const
{
	return *m_mdname;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::PmdidTypeLeft
//
//	@doc:
//		Type id of left operand
//
//---------------------------------------------------------------------------
IMDId *
CMDScalarOpGPDB::PmdidTypeLeft() const
{
	return m_pmdidTypeLeft;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::PmdidTypeRight
//
//	@doc:
//		Type id of right operand
//
//---------------------------------------------------------------------------
IMDId *
CMDScalarOpGPDB::PmdidTypeRight() const
{
	return m_pmdidTypeRight;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::PmdidTypeResult
//
//	@doc:
//		Type id of result
//
//---------------------------------------------------------------------------
IMDId *
CMDScalarOpGPDB::PmdidTypeResult() const
{
	return m_pmdidTypeResult;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::FuncMdId
//
//	@doc:
//		Id of function which implements the operator
//
//---------------------------------------------------------------------------
IMDId *
CMDScalarOpGPDB::FuncMdId() const
{
	return m_func_mdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::PmdidOpCommute
//
//	@doc:
//		Id of commute operator
//
//---------------------------------------------------------------------------
IMDId *
CMDScalarOpGPDB::PmdidOpCommute() const
{
	return m_pmdidOpCommute;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::PmdidOpInverse
//
//	@doc:
//		Id of inverse operator
//
//---------------------------------------------------------------------------
IMDId *
CMDScalarOpGPDB::PmdidOpInverse() const
{
	return m_pmdidOpInverse;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::FEquality
//
//	@doc:
//		Is this an equality operator
//
//---------------------------------------------------------------------------
BOOL
CMDScalarOpGPDB::FEquality() const
{
	return IMDType::EcmptEq == m_ecmpt;
}


//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::FReturnsNullOnNullInput
//
//	@doc:
//		Does operator return NULL when all inputs are NULL?
//		STRICT implies NULL-returning, but the opposite is not always true,
//		the implementation in GPDB returns what STRICT property states
//
//---------------------------------------------------------------------------
BOOL
CMDScalarOpGPDB::FReturnsNullOnNullInput() const
{
	return m_fReturnsNullOnNullInput;
}


//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::Ecmpt
//
//	@doc:
//		Comparison type
//
//---------------------------------------------------------------------------
IMDType::ECmpType
CMDScalarOpGPDB::Ecmpt() const
{
	return m_ecmpt;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::Serialize
//
//	@doc:
//		Serialize scalar op metadata in DXL format
//
//---------------------------------------------------------------------------
void
CMDScalarOpGPDB::Serialize
	(
	CXMLSerializer *xml_serializer
	) 
	const
{
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), 
						CDXLTokens::PstrToken(EdxltokenGPDBScalarOp));
	
	m_mdid->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenMdid));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenName), m_mdname->Pstr());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenGPDBScalarOpCmpType), IMDType::PstrCmpType(m_ecmpt));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenReturnsNullOnNullInput), m_fReturnsNullOnNullInput);

	Edxltoken rgEdxltoken[6] = {
							EdxltokenGPDBScalarOpLeftTypeId, EdxltokenGPDBScalarOpRightTypeId, 
							EdxltokenGPDBScalarOpResultTypeId, EdxltokenGPDBScalarOpFuncId, 
							EdxltokenGPDBScalarOpCommOpId, EdxltokenGPDBScalarOpInverseOpId
							};
	
	IMDId *rgMdid[6] = {m_pmdidTypeLeft, m_pmdidTypeRight, m_pmdidTypeResult, 
						m_func_mdid, m_pmdidOpCommute, m_pmdidOpInverse};
	
	for (ULONG ul = 0; ul < GPOS_ARRAY_SIZE(rgEdxltoken); ul++)
	{
		SerializeMDIdAsElem(xml_serializer, CDXLTokens::PstrToken(rgEdxltoken[ul]), rgMdid[ul]);

		GPOS_CHECK_ABORT;
	}	
	
	// serialize operator class information
	if (0 < m_pdrgpmdidOpClasses->Size())
	{
		SerializeMDIdList(xml_serializer, m_pdrgpmdidOpClasses, 
						CDXLTokens::PstrToken(EdxltokenOpClasses), 
						CDXLTokens::PstrToken(EdxltokenOpClass));
	}
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), 
						CDXLTokens::PstrToken(EdxltokenGPDBScalarOp));
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::UlOpCLasses
//
//	@doc:
//		Number of classes this operator belongs to
//
//---------------------------------------------------------------------------
ULONG
CMDScalarOpGPDB::UlOpCLasses() const
{
	return m_pdrgpmdidOpClasses->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::PmdidOpClass
//
//	@doc:
//		Operator class at given position
//
//---------------------------------------------------------------------------
IMDId *
CMDScalarOpGPDB::PmdidOpClass
	(
	ULONG ulPos
	) 
	const
{
	GPOS_ASSERT(ulPos < m_pdrgpmdidOpClasses->Size());
	
	return (*m_pdrgpmdidOpClasses)[ulPos];
}


#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::DebugPrint
//
//	@doc:
//		Prints a metadata cache relation to the provided output
//
//---------------------------------------------------------------------------
void
CMDScalarOpGPDB::DebugPrint
	(
	IOstream &os
	)
	const
{
	os << "Operator id: ";
	MDId()->OsPrint(os);
	os << std::endl;
	
	os << "Operator name: " << (Mdname()).Pstr()->GetBuffer() << std::endl;
	
	os << "Left operand type id: ";
	PmdidTypeLeft()->OsPrint(os);
	os << std::endl;
		
	os << "Right operand type id: ";
	PmdidTypeRight()->OsPrint(os);
	os << std::endl;

	os << "Result type id: ";
	PmdidTypeResult()->OsPrint(os);
	os << std::endl;

	os << "Operator func id: ";
	FuncMdId()->OsPrint(os);
	os << std::endl;

	os << "Commute operator id: ";
	PmdidOpCommute()->OsPrint(os);
	os << std::endl;

	os << "Inverse operator id: ";
	PmdidOpInverse()->OsPrint(os);
	os << std::endl;

	os << std::endl;	
}

#endif // GPOS_DEBUG


// EOF
