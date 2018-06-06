//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CDXLScalarBitmapBoolOp.cpp
//
//	@doc:
//		Implementation of DXL bitmap bool operator
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarBitmapBoolOp.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/dxltokens.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "naucrates/md/IMDType.h"
#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBitmapBoolOp::CDXLScalarBitmapBoolOp
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarBitmapBoolOp::CDXLScalarBitmapBoolOp
	(
	IMemoryPool *memory_pool,
	IMDId *mdid_type,
	EdxlBitmapBoolOp bitmapboolop
	)
	:
	CDXLScalar(memory_pool),
	m_mdid_type(mdid_type),
	m_bitmapboolop(bitmapboolop)
{
	GPOS_ASSERT(EdxlbitmapSentinel > bitmapboolop);
	GPOS_ASSERT(IMDId::IsValid(mdid_type));
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBitmapBoolOp::~CDXLScalarBitmapBoolOp
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLScalarBitmapBoolOp::~CDXLScalarBitmapBoolOp()
{
	m_mdid_type->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBitmapBoolOp::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarBitmapBoolOp::GetDXLOperator() const
{
	return EdxlopScalarBitmapBoolOp;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBitmapBoolOp::MDIdType
//
//	@doc:
//		Return type
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarBitmapBoolOp::MDIdType() const
{
	return m_mdid_type;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBitmapBoolOp::EdxlBitmapBoolOp
//
//	@doc:
//		Bitmap bool type
//
//---------------------------------------------------------------------------
CDXLScalarBitmapBoolOp::EdxlBitmapBoolOp
CDXLScalarBitmapBoolOp::Edxlbitmapboolop() const
{
	return m_bitmapboolop;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBitmapBoolOp::HasBoolResult
//
//	@doc:
//		Is operator returning a boolean value
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarBitmapBoolOp::HasBoolResult
	(
	CMDAccessor *md_accessor
	) 
	const
{
	return (IMDType::EtiBool == md_accessor->Pmdtype(m_mdid_type)->Eti());
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBitmapBoolOp::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarBitmapBoolOp::GetOpNameStr() const
{
	if (EdxlbitmapAnd == m_bitmapboolop)
	{
		return CDXLTokens::GetDXLTokenStr(EdxltokenScalarBitmapAnd);
	}
	
	return CDXLTokens::GetDXLTokenStr(EdxltokenScalarBitmapOr);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBitmapBoolOp::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarBitmapBoolOp::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	GPOS_CHECK_ABORT;

	const CWStringConst *element_name = GetOpNameStr();

	GPOS_ASSERT(NULL != element_name);
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
	m_mdid_type->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenTypeId));
	
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);

	GPOS_CHECK_ABORT;
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBitmapBoolOp::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarBitmapBoolOp::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	EdxlBitmapBoolOp edxlbitmapboolop = ((CDXLScalarBitmapBoolOp *) pdxln->GetOperator())->Edxlbitmapboolop();

	GPOS_ASSERT( (edxlbitmapboolop == EdxlbitmapAnd) || (edxlbitmapboolop == EdxlbitmapOr));

	ULONG ulArity = pdxln->Arity();
	GPOS_ASSERT(2 == ulArity);
	

	for (ULONG ul = 0; ul < ulArity; ++ul)
	{
		CDXLNode *pdxlnArg = (*pdxln)[ul];
		Edxlopid edxlop = pdxlnArg->GetOperator()->GetDXLOperator();
		
		GPOS_ASSERT(EdxlopScalarBitmapBoolOp == edxlop || EdxlopScalarBitmapIndexProbe == edxlop);
		
		if (validate_children)
		{
			pdxlnArg->GetOperator()->AssertValid(pdxlnArg, validate_children);
		}
	}
}
#endif // GPOS_DEBUG

// EOF
