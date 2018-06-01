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
//		CDXLScalarOpExpr::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarOpExpr::Edxlop() const
{
	return EdxlopScalarOpExpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarOpExpr::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarOpExpr::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarOpExpr);
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
//		CDXLScalarOpExpr::FBoolean
//
//	@doc:
//		Does the operator return boolean result
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarOpExpr::FBoolean
	(
	CMDAccessor *pmda
	)
	const
{
	const IMDScalarOp *pmdscop = pmda->Pmdscop(m_mdid);
	IMDId *pmdid = pmda->Pmdfunc(pmdscop->PmdidFunc())->PmdidTypeResult();
	return (IMDType::EtiBool == pmda->Pmdtype(pmdid)->Eti());
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
	const CDXLNode *pdxln
	)
	const
{
	GPOS_CHECK_ABORT;

	const CWStringConst *element_name = PstrOpName();
	const CWStringConst *pstrOpName = PstrScalarOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenOpName), pstrOpName);
	m_mdid->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenOpNo));
	
	if (NULL != m_pmdidReturnType)
	{
		m_pmdidReturnType->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenOpType));
	}
	
	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

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
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	const ULONG ulArity = pdxln->Arity();
	GPOS_ASSERT(1 == ulArity || 2 == ulArity);

	for (ULONG ul = 0; ul < ulArity; ++ul)
	{
		CDXLNode *pdxlnArg = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeScalar == pdxlnArg->GetOperator()->Edxloperatortype());
		
		if (validate_children)
		{
			pdxlnArg->GetOperator()->AssertValid(pdxlnArg, validate_children);
		}
	}
}
#endif // GPOS_DEBUG


// EOF
