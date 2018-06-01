//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarComp.cpp
//
//	@doc:
//		Implementation of DXL comparison operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLScalarComp.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarComp::CDXLScalarComp
//
//	@doc:
//		Constructs a scalar comparison node
//
//---------------------------------------------------------------------------
CDXLScalarComp::CDXLScalarComp
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidOp,
	const CWStringConst *pstrCompOpName
	)
	:
	CDXLScalar(memory_pool),
	m_mdid(pmdidOp),
	m_pstrCompOpName(pstrCompOpName)
{
	GPOS_ASSERT(m_mdid->IsValid());
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarComp::~CDXLScalarComp
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLScalarComp::~CDXLScalarComp()
{
	m_mdid->Release();
	GPOS_DELETE(m_pstrCompOpName);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarComp::PstrCmpOpName
//
//	@doc:
//		Comparison operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarComp::PstrCmpOpName() const
{
	return m_pstrCompOpName;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarComp::MDId
//
//	@doc:
//		Comparison operator id
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarComp::MDId() const
{
	return m_mdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarComp::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarComp::Edxlop() const
{
	return EdxlopScalarCmp;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarComp::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarComp::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarComp);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarComp::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarComp::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	GPOS_CHECK_ABORT;

	const CWStringConst *element_name = PstrOpName();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenComparisonOp), PstrCmpOpName());

	m_mdid->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenOpNo));
	
	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	GPOS_CHECK_ABORT;
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarComp::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarComp::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	const ULONG ulArity = pdxln->Arity();
	GPOS_ASSERT(2 == ulArity);

	for (ULONG ul = 0; ul < ulArity; ++ul)
	{
		CDXLNode *pdxlnArg = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeScalar == pdxlnArg->GetOperator()->Edxloperatortype() ||
					EdxloptypeLogical == pdxlnArg->GetOperator()->Edxloperatortype());
		
		if (validate_children)
		{
			pdxlnArg->GetOperator()->AssertValid(pdxlnArg, validate_children);
		}
	}
}
#endif // GPOS_DEBUG





// EOF
