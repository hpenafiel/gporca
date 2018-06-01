//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLScalarSwitchCase.cpp
//
//	@doc:
//		Implementation of DXL Switch case
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarSwitchCase.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpopt;
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSwitchCase::CDXLScalarSwitchCase
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarSwitchCase::CDXLScalarSwitchCase
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLScalar(memory_pool)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSwitchCase::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarSwitchCase::Edxlop() const
{
	return EdxlopScalarSwitchCase;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSwitchCase::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarSwitchCase::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarSwitchCase);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSwitchCase::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarSwitchCase::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSwitchCase::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarSwitchCase::AssertValid
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
		GPOS_ASSERT(EdxloptypeScalar == pdxlnArg->GetOperator()->Edxloperatortype());

		if (validate_children)
		{
			pdxlnArg->GetOperator()->AssertValid(pdxlnArg, validate_children);
		}
	}
}
#endif // GPOS_DEBUG

// EOF
