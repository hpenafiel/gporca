//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarFuncExpr.cpp
//
//	@doc:
//		Implementation of DXL Scalar FuncExpr
//		
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarFuncExpr.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpopt;
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarFuncExpr::CDXLScalarFuncExpr
//
//	@doc:
//		Constructs a scalar FuncExpr node
//
//---------------------------------------------------------------------------
CDXLScalarFuncExpr::CDXLScalarFuncExpr
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidFunc,
	IMDId *pmdidRetType,
	INT iRetTypeModifier,
	BOOL fRetSet
	)
	:
	CDXLScalar(memory_pool),
	m_func_mdid(pmdidFunc),
	m_return_type_mdid(pmdidRetType),
	m_iRetTypeModifier(iRetTypeModifier),
	m_fReturnSet(fRetSet)
{
	GPOS_ASSERT(m_func_mdid->IsValid());
	GPOS_ASSERT(m_return_type_mdid->IsValid());
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarFuncExpr::~CDXLScalarFuncExpr
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLScalarFuncExpr::~CDXLScalarFuncExpr()
{
	m_func_mdid->Release();
	m_return_type_mdid->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarFuncExpr::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarFuncExpr::GetDXLOperator() const
{
	return EdxlopScalarFuncExpr;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarFuncExpr::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarFuncExpr::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarFuncExpr);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarFuncExpr::FuncMdId
//
//	@doc:
//		Returns function id
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarFuncExpr::FuncMdId() const
{
	return m_func_mdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarFuncExpr::ReturnTypeMdId
//
//	@doc:
//		Return type
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarFuncExpr::ReturnTypeMdId() const
{
	return m_return_type_mdid;
}

INT
CDXLScalarFuncExpr::TypeModifier() const
{
	return m_iRetTypeModifier;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarFuncExpr::FReturnSet
//
//	@doc:
//		Returns whether the function returns a set
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarFuncExpr::FReturnSet() const
{
	return m_fReturnSet;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarFuncExpr::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarFuncExpr::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	m_func_mdid->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenFuncId));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenFuncRetSet), m_fReturnSet);
	m_return_type_mdid->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenTypeId));

	if (IDefaultTypeModifier != TypeModifier())
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenTypeMod), TypeModifier());
	}

	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarFuncExpr::FBoolean
//
//	@doc:
//		Does the operator return boolean result
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarFuncExpr::FBoolean
	(
	CMDAccessor *pmda
	)
	const
{
	IMDId *pmdid = pmda->Pmdfunc(m_func_mdid)->PmdidTypeResult();
	return (IMDType::EtiBool == pmda->Pmdtype(pmdid)->Eti());
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarFuncExpr::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarFuncExpr::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	for (ULONG ul = 0; ul < pdxln->Arity(); ++ul)
	{
		CDXLNode *pdxlnArg = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeScalar == pdxlnArg->GetOperator()->GetDXLOperatorType());
		
		if (validate_children)
		{
			pdxlnArg->GetOperator()->AssertValid(pdxlnArg, validate_children);
		}
	}
}
#endif // GPOS_DEBUG


// EOF
