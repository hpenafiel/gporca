//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarConstValue.cpp
//
//	@doc:
//		Implementation of DXL scalar const
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarConstValue.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpopt;
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarConstValue::CDXLScalarConstValue
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLScalarConstValue::CDXLScalarConstValue
	(
	IMemoryPool *memory_pool,
	CDXLDatum *datum_dxl
	)
	:
	CDXLScalar(memory_pool),
	m_pdxldatum(datum_dxl)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarConstValue::~CDXLScalarConstValue
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLScalarConstValue::~CDXLScalarConstValue()
{
	m_pdxldatum->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarConstValue::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarConstValue::Edxlop() const
{
	return EdxlopScalarConstValue;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarConstValue::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarConstValue::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarConstValue);;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarConstValue::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarConstValue::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *//pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();
	m_pdxldatum->Serialize(xml_serializer, element_name);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarConstValue::FBoolean
//
//	@doc:
//		Does the operator return boolean result
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarConstValue::FBoolean
	(
	CMDAccessor *pmda
	)
	const
{
	return (IMDType::EtiBool == pmda->Pmdtype(m_pdxldatum->MDId())->Eti());
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarConstValue::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarConstValue::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL // validate_children 
	) 
	const
{
	GPOS_ASSERT(0 == pdxln->Arity());
	GPOS_ASSERT(m_pdxldatum->MDId()->IsValid());
}
#endif // GPOS_DEBUG

// EOF
