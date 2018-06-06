//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CMDPartConstraintGPDB.cpp
//
//	@doc:
//		Implementation of part constraints in the MD cache
//---------------------------------------------------------------------------

#include "naucrates/md/CMDPartConstraintGPDB.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

#include "gpopt/translate/CTranslatorDXLToExpr.h"

using namespace gpdxl;
using namespace gpmd;
using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CMDPartConstraintGPDB::CMDPartConstraintGPDB
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDPartConstraintGPDB::CMDPartConstraintGPDB
	(
	IMemoryPool *memory_pool,
	ULongPtrArray *pdrgpulDefaultParts,
	BOOL fUnbounded,
	CDXLNode *dxlnode
	)
	:
	m_memory_pool(memory_pool),
	m_pdrgpulDefaultParts(pdrgpulDefaultParts),
	m_fUnbounded(fUnbounded),
	m_dxl_node(dxlnode)
{
	GPOS_ASSERT(NULL != pdrgpulDefaultParts);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDPartConstraintGPDB::~CMDPartConstraintGPDB
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDPartConstraintGPDB::~CMDPartConstraintGPDB()
{
	if (NULL != m_dxl_node)
		m_dxl_node->Release();
	m_pdrgpulDefaultParts->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDPartConstraintGPDB::Pexpr
//
//	@doc:
//		Scalar expression of the check constraint
//
//---------------------------------------------------------------------------
CExpression *
CMDPartConstraintGPDB::Pexpr
	(
	IMemoryPool *memory_pool,
	CMDAccessor *md_accessor,
	DrgPcr *pdrgpcr
	)
	const
{
	GPOS_ASSERT(NULL != pdrgpcr);

	// translate the DXL representation of the part constraint expression
	CTranslatorDXLToExpr dxltr(memory_pool, md_accessor);
	return dxltr.PexprTranslateScalar(m_dxl_node, pdrgpcr);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDPartConstraintGPDB::PdrgpulDefaultParts
//
//	@doc:
//		Included default partitions
//
//---------------------------------------------------------------------------
ULongPtrArray *
CMDPartConstraintGPDB::PdrgpulDefaultParts() const
{
	return m_pdrgpulDefaultParts;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDPartConstraintGPDB::FUnbounded
//
//	@doc:
//		Is constraint unbounded
//
//---------------------------------------------------------------------------
BOOL
CMDPartConstraintGPDB::FUnbounded() const
{
	return m_fUnbounded;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDPartConstraintGPDB::Serialize
//
//	@doc:
//		Serialize part constraint in DXL format
//
//---------------------------------------------------------------------------
void
CMDPartConstraintGPDB::Serialize
	(
	CXMLSerializer *xml_serializer
	)
	const
{
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
						CDXLTokens::GetDXLTokenStr(EdxltokenPartConstraint));
	
	// serialize default parts
	CWStringDynamic *pstrDefParts = CDXLUtils::Serialize(m_memory_pool, m_pdrgpulDefaultParts);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenDefaultPartition), pstrDefParts);
	GPOS_DELETE(pstrDefParts);

	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenPartConstraintUnbounded), m_fUnbounded);

	// serialize the scalar expression
	if (NULL != m_dxl_node)
		m_dxl_node->SerializeToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
						CDXLTokens::GetDXLTokenStr(EdxltokenPartConstraint));

	GPOS_CHECK_ABORT;
}

// EOF
