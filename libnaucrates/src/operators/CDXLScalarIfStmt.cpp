//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarIfStmt.cpp
//
//	@doc:
//		Implementation of DXL If Statement
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarIfStmt.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpopt;
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarIfStmt::CDXLScalarIfStmt
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLScalarIfStmt::CDXLScalarIfStmt
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidResultType
	)
	:
	CDXLScalar(memory_pool),
	m_pmdidResultType(pmdidResultType)
{
	GPOS_ASSERT(m_pmdidResultType->IsValid());
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarIfStmt::~CDXLScalarIfStmt
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLScalarIfStmt::~CDXLScalarIfStmt()
{
	m_pmdidResultType->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarIfStmt::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarIfStmt::Edxlop() const
{
	return EdxlopScalarIfStmt;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarIfStmt::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarIfStmt::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarIfStmt);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarIfStmt::PmdidResultType
//
//	@doc:
//		Return type id
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarIfStmt::PmdidResultType() const
{
	return m_pmdidResultType;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarIfStmt::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarIfStmt::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	m_pmdidResultType->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenTypeId));
	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarIfStmt::FBoolean
//
//	@doc:
//		Does the operator return a boolean result
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarIfStmt::FBoolean
	(
	CMDAccessor *pmda
	)
	const
{
	return (IMDType::EtiBool == pmda->Pmdtype(m_pmdidResultType)->Eti());
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarIfStmt::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarIfStmt::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	const ULONG ulArity = pdxln->Arity();
	GPOS_ASSERT(3 == ulArity);

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
