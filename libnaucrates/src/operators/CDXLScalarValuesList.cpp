//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 Pivotal Software, Inc.
//
//	@filename:
//		CDXLScalarValuesList.cpp
//
//	@doc:
//		Implementation of DXL value list operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLScalarValuesList.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;


// constructs a value list node
CDXLScalarValuesList::CDXLScalarValuesList
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLScalar(memory_pool)
{
}

// destructor
CDXLScalarValuesList::~CDXLScalarValuesList()
{
}

// operator type
Edxlopid
CDXLScalarValuesList::Edxlop() const
{
	return EdxlopScalarValuesList;
}

// operator name
const CWStringConst *
CDXLScalarValuesList::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarValuesList);
}

// serialize operator in DXL format
void
CDXLScalarValuesList::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	GPOS_CHECK_ABORT;

	const CWStringConst *element_name = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	GPOS_CHECK_ABORT;
}

// conversion function
CDXLScalarValuesList *
CDXLScalarValuesList::PdxlopConvert
	(
	CDXLOperator *dxl_op
	)
{
	GPOS_ASSERT(NULL != dxl_op);
	GPOS_ASSERT(EdxlopScalarValuesList == dxl_op->Edxlop());

	return dynamic_cast<CDXLScalarValuesList*>(dxl_op);
}

// does the operator return a boolean result
BOOL
CDXLScalarValuesList::FBoolean
	(
	CMDAccessor * //pmda
	)
	const
{
	return false;
}

#ifdef GPOS_DEBUG

// checks whether operator node is well-structured
void
CDXLScalarValuesList::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	const ULONG ulArity = pdxln->Arity();

	for (ULONG ul = 0; ul < ulArity; ++ul)
	{
		CDXLNode *pdxlnConstVal = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeScalar == pdxlnConstVal->GetOperator()->Edxloperatortype());

		if (validate_children)
		{
			pdxlnConstVal->GetOperator()->AssertValid(pdxlnConstVal, validate_children);
		}
	}
}
#endif // GPOS_DEBUG

// EOF
