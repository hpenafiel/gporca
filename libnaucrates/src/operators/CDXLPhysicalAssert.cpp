//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalAssert.cpp
//
//	@doc:
//		Implementation of DXL physical assert operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLPhysicalAssert.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAssert::CDXLPhysicalAssert
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLPhysicalAssert::CDXLPhysicalAssert
	(
	IMemoryPool *memory_pool,
	const CHAR *szSQLState
	)
	:
	CDXLPhysical(memory_pool)
{
	GPOS_ASSERT(NULL != szSQLState);
	GPOS_ASSERT(GPOS_SQLSTATE_LENGTH == clib::StrLen(szSQLState));
	clib::StrNCpy(m_szSQLState, szSQLState, GPOS_SQLSTATE_LENGTH);
	m_szSQLState[GPOS_SQLSTATE_LENGTH] = '\0';
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAssert::~CDXLPhysicalAssert
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLPhysicalAssert::~CDXLPhysicalAssert()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAssert::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalAssert::Edxlop() const
{
	return EdxlopPhysicalAssert;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAssert::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalAssert::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalAssert);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAssert::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalAssert::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *pstrElemName = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenErrorCode), m_szSQLState);
	
	pdxln->SerializePropertiesToDXL(xml_serializer);
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAssert::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLPhysicalAssert::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL fValidateChildren
	) 
	const
{

	GPOS_ASSERT(3 == pdxln->Arity());
	
	CDXLNode *pdxlnProjList = (*pdxln)[EdxlassertIndexProjList];
	GPOS_ASSERT(EdxlopScalarProjectList == pdxlnProjList->Pdxlop()->Edxlop());

	CDXLNode *pdxlnPredicate = (*pdxln)[EdxlassertIndexFilter];
	GPOS_ASSERT(EdxlopScalarAssertConstraintList == pdxlnPredicate->Pdxlop()->Edxlop());

	CDXLNode *pdxlnPhysicalChild = (*pdxln)[EdxlassertIndexChild];
	GPOS_ASSERT(EdxloptypePhysical == pdxlnPhysicalChild->Pdxlop()->Edxloperatortype());

	
	if (fValidateChildren)
	{
		for (ULONG ul = 0; ul < 3; ul++)
		{
			CDXLNode *pdxlnChild = (*pdxln)[ul];
			pdxlnChild->Pdxlop()->AssertValid(pdxlnChild, fValidateChildren);
		}
	}
}
#endif // GPOS_DEBUG

// EOF
