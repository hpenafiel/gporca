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
	IMDId *mdid,
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
	)
	:
	m_memory_pool(memory_pool),
	m_mdid(mdid),
	m_mdname(mdname),
	m_mdid_type_left(pmdidTypeLeft),
	m_mdid_type_right(pmdidTypeRight),
	m_mdid_type_result(pmdidTypeResult),
	m_func_mdid(mdid_func),
	m_mdid_commute_opr(pmdidOpCommute),
	m_mdid_inverse_opr(pmdidOpInverse),
	m_comparision_type(ecmpt),
	m_returns_null_on_null_input(fReturnsNullOnNullInput),
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
	m_mdid_type_result->Release();
	m_func_mdid->Release();	

	CRefCount::SafeRelease(m_mdid_type_left);
	CRefCount::SafeRelease(m_mdid_type_right);
	CRefCount::SafeRelease(m_mdid_commute_opr);
	CRefCount::SafeRelease(m_mdid_inverse_opr);
	
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
	return m_mdid_type_left;
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
	return m_mdid_type_right;
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
	return m_mdid_type_result;
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
	return m_mdid_commute_opr;
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
	return m_mdid_inverse_opr;
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
	return IMDType::EcmptEq == m_comparision_type;
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
	return m_returns_null_on_null_input;
}


//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::ParseCmpType
//
//	@doc:
//		Comparison type
//
//---------------------------------------------------------------------------
IMDType::ECmpType
CMDScalarOpGPDB::ParseCmpType() const
{
	return m_comparision_type;
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
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
						CDXLTokens::GetDXLTokenStr(EdxltokenGPDBScalarOp));
	
	m_mdid->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMdid));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenName), m_mdname->GetMDName());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenGPDBScalarOpCmpType), IMDType::PstrCmpType(m_comparision_type));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenReturnsNullOnNullInput), m_returns_null_on_null_input);

	Edxltoken rgEdxltoken[6] = {
							EdxltokenGPDBScalarOpLeftTypeId, EdxltokenGPDBScalarOpRightTypeId, 
							EdxltokenGPDBScalarOpResultTypeId, EdxltokenGPDBScalarOpFuncId, 
							EdxltokenGPDBScalarOpCommOpId, EdxltokenGPDBScalarOpInverseOpId
							};
	
	IMDId *rgMdid[6] = {m_mdid_type_left, m_mdid_type_right, m_mdid_type_result, 
						m_func_mdid, m_mdid_commute_opr, m_mdid_inverse_opr};
	
	for (ULONG ul = 0; ul < GPOS_ARRAY_SIZE(rgEdxltoken); ul++)
	{
		SerializeMDIdAsElem(xml_serializer, CDXLTokens::GetDXLTokenStr(rgEdxltoken[ul]), rgMdid[ul]);

		GPOS_CHECK_ABORT;
	}	
	
	// serialize operator class information
	if (0 < m_pdrgpmdidOpClasses->Size())
	{
		SerializeMDIdList(xml_serializer, m_pdrgpmdidOpClasses, 
						CDXLTokens::GetDXLTokenStr(EdxltokenOpClasses), 
						CDXLTokens::GetDXLTokenStr(EdxltokenOpClass));
	}
	
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
						CDXLTokens::GetDXLTokenStr(EdxltokenGPDBScalarOp));
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
	
	os << "Operator name: " << (Mdname()).GetMDName()->GetBuffer() << std::endl;
	
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
