//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CMDCheckConstraintGPDB.cpp
//
//	@doc:
//		Implementation of the class representing a GPDB check constraint
//		in a metadata cache relation
//---------------------------------------------------------------------------

#include "naucrates/md/CMDCheckConstraintGPDB.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

#include "gpopt/translate/CTranslatorDXLToExpr.h"

using namespace gpdxl;
using namespace gpmd;
using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CMDCheckConstraintGPDB::CMDCheckConstraintGPDB
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDCheckConstraintGPDB::CMDCheckConstraintGPDB
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid,
	CMDName *mdname,
	IMDId *pmdidRel,
	CDXLNode *dxlnode
	)
	:
	m_memory_pool(memory_pool),
	m_mdid(pmdid),
	m_mdname(mdname),
	m_rel_mdid(pmdidRel),
	m_dxl_node(dxlnode)
{
	GPOS_ASSERT(pmdid->IsValid());
	GPOS_ASSERT(pmdidRel->IsValid());
	GPOS_ASSERT(NULL != mdname);
	GPOS_ASSERT(NULL != dxlnode);

	m_pstr = CDXLUtils::SerializeMDObj(m_memory_pool, this, false /*fSerializeHeader*/, false /*indentation*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDCheckConstraintGPDB::~CMDCheckConstraintGPDB
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDCheckConstraintGPDB::~CMDCheckConstraintGPDB()
{
	GPOS_DELETE(m_mdname);
	GPOS_DELETE(m_pstr);
	m_mdid->Release();
	m_rel_mdid->Release();
	m_dxl_node->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDCheckConstraintGPDB::Pexpr
//
//	@doc:
//		Scalar expression of the check constraint
//
//---------------------------------------------------------------------------
CExpression *
CMDCheckConstraintGPDB::Pexpr
	(
	IMemoryPool *memory_pool,
	CMDAccessor *md_accessor,
	DrgPcr *pdrgpcr
	)
	const
{
	GPOS_ASSERT(NULL != pdrgpcr);

	const IMDRelation *pmdrel = md_accessor->Pmdrel(m_rel_mdid);
#ifdef GPOS_DEBUG
	const ULONG ulLen = pdrgpcr->Size();
	GPOS_ASSERT(ulLen > 0);

	const ULONG arity = pmdrel->UlNonDroppedCols() - pmdrel->UlSystemColumns();
	GPOS_ASSERT(arity == ulLen);
#endif // GPOS_DEBUG

	// translate the DXL representation of the check constraint expression
	CTranslatorDXLToExpr dxltr(memory_pool, md_accessor);
	return dxltr.PexprTranslateScalar(m_dxl_node, pdrgpcr, pmdrel->PdrgpulNonDroppedCols());
}

//---------------------------------------------------------------------------
//	@function:
//		CMDCheckConstraintGPDB::Serialize
//
//	@doc:
//		Serialize check constraint in DXL format
//
//---------------------------------------------------------------------------
void
CMDCheckConstraintGPDB::Serialize
	(
	CXMLSerializer *xml_serializer
	)
	const
{
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
						CDXLTokens::GetDXLTokenStr(EdxltokenCheckConstraint));

	m_mdid->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMdid));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenName), m_mdname->GetMDName());
	m_rel_mdid->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenRelationMdid));

	// serialize the scalar expression
	m_dxl_node->SerializeToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
						CDXLTokens::GetDXLTokenStr(EdxltokenCheckConstraint));
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CMDCheckConstraintGPDB::DebugPrint
//
//	@doc:
//		Prints a MD constraint to the provided output
//
//---------------------------------------------------------------------------
void
CMDCheckConstraintGPDB::DebugPrint
	(
	IOstream &os
	)
	const
{
	os << "Constraint Id: ";
	MDId()->OsPrint(os);
	os << std::endl;

	os << "Constraint Name: " << (Mdname()).GetMDName()->GetBuffer() << std::endl;

	os << "Relation id: ";
	PmdidRel()->OsPrint(os);
	os << std::endl;
}

#endif // GPOS_DEBUG

// EOF
