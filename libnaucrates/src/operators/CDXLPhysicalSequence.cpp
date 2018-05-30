//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalSequence.cpp
//
//	@doc:
//		Implementation of DXL physical sequence operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLPhysicalSequence.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSequence::CDXLPhysicalSequence
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLPhysicalSequence::CDXLPhysicalSequence
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLPhysical(memory_pool)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSequence::~CDXLPhysicalSequence
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLPhysicalSequence::~CDXLPhysicalSequence()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSequence::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalSequence::Edxlop() const
{
	return EdxlopPhysicalSequence;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSequence::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalSequence::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalSequence);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSequence::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalSequence::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *pstrElemName = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);
		
	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);

	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSequence::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLPhysicalSequence::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL fValidateChildren
	) 
	const
{

	const ULONG ulArity = pdxln->UlArity();  
	GPOS_ASSERT(1 < ulArity);

	for (ULONG ul = 1; ul < ulArity; ul++)
	{
		CDXLNode *pdxlnChild = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypePhysical == pdxlnChild->Pdxlop()->Edxloperatortype());

		if (fValidateChildren)
		{
			pdxlnChild->Pdxlop()->AssertValid(pdxlnChild, fValidateChildren);
		}
	}

}
#endif // GPOS_DEBUG

// EOF
