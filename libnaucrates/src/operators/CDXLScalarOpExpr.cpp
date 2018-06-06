//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarOpExpr.cpp
//
//	@doc:
//		Implementation of DXL Scalar OpExpr
//---------------------------------------------------------------------------

#include "naucrates/md/IMDScalarOp.h"

#include "naucrates/dxl/operators/CDXLScalarOpExpr.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpos;
using namespace gpopt;
using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarOpExpr::CDXLScalarOpExpr
//
//	@doc:
//		Constructs a scalar OpExpr node
//
//---------------------------------------------------------------------------
CDXLScalarOpExpr::CDXLScalarOpExpr
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidOp,
	IMDId *pmdidReturnType,
	const CWStringConst *pstrOpName
	)
	:
	CDXLScalar(memory_pool),
	m_mdid(pmdidOp),
	m_pmdidReturnType(pmdidReturnType),
	m_pstrOpName(pstrOpName)
{
	GPOS_ASSERT(m_mdid->IsValid());

}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarOpExpr::~CDXLScalarOpExpr
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLScalarOpExpr::~CDXLScalarOpExpr()
{
	m_mdid->Release();
	CRefCount::SafeRelease(m_pmdidReturnType);
	GPOS_DELETE(m_pstrOpName);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarOpExpr::PstrScalarOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarOpExpr::PstrScalarOpName() const
{
	return m_pstrOpName;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarOpExpr::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarOpExpr::GetDXLOperator() const
{
	return EdxlopScalarOpExpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarOpExpr::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarOpExpr::GetOpNameStr() const
{
	return CDXLTokens::GetDXLTokenStr(EdxltokenScalarOpExpr);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarOpExpr::MDId
//
//	@doc:
//		Operator id
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarOpExpr::MDId() const
{
	return m_mdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarOpExpr::PmdidReturnType
//
//	@doc:
//		Operator return type
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarOpExpr::PmdidReturnType() const
{
	return m_pmdidReturnType;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarOpExpr::HasBoolResult
//
//	@doc:
//		Does the operator return boolean result
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarOpExpr::HasBoolResult
	(
	CMDAccessor *md_accessor
	)
	const
{
	const IMDScalarOp *pmdscop = md_accessor->Pmdscop(m_mdid);
	IMDId *pmdid = md_accessor->Pmdfunc(pmdscop->FuncMdId())->PmdidTypeResult();
	return (IMDType::EtiBool == md_accessor->Pmdtype(pmdid)->Eti());
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarOpExpr::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarOpExpr::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *dxlnode
	)
	const
{
	GPOS_CHECK_ABORT;

	const CWStringConst *element_name = GetOpNameStr();
	const CWStringConst *pstrOpName = PstrScalarOpName();

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenOpName), pstrOpName);
	m_mdid->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenOpNo));
	
	if (NULL != m_pmdidReturnType)
	{
		m_pmdidReturnType->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenOpType));
	}
	
	dxlnode->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);

	GPOS_CHECK_ABORT;
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarOpExpr::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarOpExpr::AssertValid
	(
	const CDXLNode *dxlnode,
	BOOL validate_children
	) 
	const
{
	const ULONG arity = dxlnode->Arity();
	GPOS_ASSERT(1 == arity || 2 == arity);

	for (ULONG ul = 0; ul < arity; ++ul)
	{
		CDXLNode *dxlnode_arg = (*dxlnode)[ul];
		GPOS_ASSERT(EdxloptypeScalar == dxlnode_arg->GetOperator()->GetDXLOperatorType());
		
		if (validate_children)
		{
			dxlnode_arg->GetOperator()->AssertValid(dxlnode_arg, validate_children);
		}
	}
}
#endif // GPOS_DEBUG


// EOF
