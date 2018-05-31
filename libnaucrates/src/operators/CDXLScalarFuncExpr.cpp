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
	m_pmdidFunc(pmdidFunc),
	m_pmdidRetType(pmdidRetType),
	m_iRetTypeModifier(iRetTypeModifier),
	m_fReturnSet(fRetSet)
{
	GPOS_ASSERT(m_pmdidFunc->IsValid());
	GPOS_ASSERT(m_pmdidRetType->IsValid());
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
	m_pmdidFunc->Release();
	m_pmdidRetType->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarFuncExpr::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarFuncExpr::Edxlop() const
{
	return EdxlopScalarFuncExpr;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarFuncExpr::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarFuncExpr::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarFuncExpr);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarFuncExpr::PmdidFunc
//
//	@doc:
//		Returns function id
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarFuncExpr::PmdidFunc() const
{
	return m_pmdidFunc;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarFuncExpr::PmdidRetType
//
//	@doc:
//		Return type
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarFuncExpr::PmdidRetType() const
{
	return m_pmdidRetType;
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
	const CWStringConst *element_name = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	m_pmdidFunc->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenFuncId));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenFuncRetSet), m_fReturnSet);
	m_pmdidRetType->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenTypeId));

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
	IMDId *pmdid = pmda->Pmdfunc(m_pmdidFunc)->PmdidTypeResult();
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
	BOOL fValidateChildren
	) 
	const
{
	for (ULONG ul = 0; ul < pdxln->Arity(); ++ul)
	{
		CDXLNode *pdxlnArg = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeScalar == pdxlnArg->Pdxlop()->Edxloperatortype());
		
		if (fValidateChildren)
		{
			pdxlnArg->Pdxlop()->AssertValid(pdxlnArg, fValidateChildren);
		}
	}
}
#endif // GPOS_DEBUG


// EOF
