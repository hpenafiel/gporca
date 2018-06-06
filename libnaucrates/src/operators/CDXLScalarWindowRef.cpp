//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLScalarWindowRef.cpp
//
//	@doc:
//		Implementation of DXL WindowRef
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarWindowRef.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"
#include "naucrates/md/IMDAggregate.h"

using namespace gpopt;
using namespace gpmd;
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarWindowRef::CDXLScalarWindowRef
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarWindowRef::CDXLScalarWindowRef
	(
	IMemoryPool *memory_pool,
	IMDId *mdid_func,
	IMDId *mdid_return_type,
	BOOL fDistinct,
	BOOL fStarArg,
	BOOL fSimpleAgg,
	EdxlWinStage edxlwinstage,
	ULONG ulWinspecPosition
	)
	:
	CDXLScalar(memory_pool),
	m_func_mdid(mdid_func),
	m_return_type_mdid(mdid_return_type),
	m_fDistinct(fDistinct),
	m_fStarArg(fStarArg),
	m_fSimpleAgg(fSimpleAgg),
	m_edxlwinstage(edxlwinstage),
	m_ulWinspecPos(ulWinspecPosition)
{
	GPOS_ASSERT(m_func_mdid->IsValid());
	GPOS_ASSERT(m_return_type_mdid->IsValid());
	GPOS_ASSERT(EdxlwinstageSentinel != m_edxlwinstage);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarWindowRef::~CDXLScalarWindowRef
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLScalarWindowRef::~CDXLScalarWindowRef()
{
	m_func_mdid->Release();
	m_return_type_mdid->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarWindowRef::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarWindowRef::GetDXLOperator() const
{
	return EdxlopScalarWindowRef;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarWindowRef::PstrWinStage
//
//	@doc:
//		Return window stage
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarWindowRef::PstrWinStage() const
{
	GPOS_ASSERT(EdxlwinstageSentinel > m_edxlwinstage);
	ULONG rgrgulMapping[][2] =
					{
					{EdxlwinstageImmediate, EdxltokenWindowrefStageImmediate},
					{EdxlwinstagePreliminary, EdxltokenWindowrefStagePreliminary},
					{EdxlwinstageRowKey, EdxltokenWindowrefStageRowKey}
					};

	const ULONG ulArity = GPOS_ARRAY_SIZE(rgrgulMapping);
	for (ULONG ul = 0; ul < ulArity; ul++)
	{
		ULONG *pulElem = rgrgulMapping[ul];
		if ((ULONG) m_edxlwinstage == pulElem[0])
		{
			Edxltoken edxltk = (Edxltoken) pulElem[1];
			return CDXLTokens::GetDXLTokenStr(edxltk);
			break;
		}
	}

	GPOS_ASSERT(!"Unrecognized window stage");
	return NULL;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarWindowRef::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarWindowRef::GetOpNameStr() const
{
	return CDXLTokens::GetDXLTokenStr(EdxltokenScalarWindowref);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarWindowRef::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarWindowRef::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
	m_func_mdid->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenWindowrefOid));
	m_return_type_mdid->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenTypeId));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenWindowrefDistinct),m_fDistinct);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenWindowrefStarArg),m_fStarArg);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenWindowrefSimpleAgg),m_fSimpleAgg);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenWindowrefStrategy), PstrWinStage());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenWindowrefWinSpecPos), m_ulWinspecPos);

	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarWindowRef::HasBoolResult
//
//	@doc:
//		Does the operator return a boolean result
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarWindowRef::HasBoolResult
	(
	CMDAccessor *md_accessor
	)
	const
{
	IMDId *pmdid = md_accessor->Pmdfunc(m_func_mdid)->PmdidTypeResult();
	return (IMDType::EtiBool == md_accessor->Pmdtype(pmdid)->Eti());
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarWindowRef::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarWindowRef::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	EdxlWinStage edxlwinrefstage = ((CDXLScalarWindowRef*) pdxln->GetOperator())->Edxlwinstage();

	GPOS_ASSERT((EdxlwinstageSentinel >= edxlwinrefstage));

	const ULONG ulArity = pdxln->Arity();
	for (ULONG ul = 0; ul < ulArity; ++ul)
	{
		CDXLNode *pdxlnWinrefArg = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeScalar == pdxlnWinrefArg->GetOperator()->GetDXLOperatorType());

		if (validate_children)
		{
			pdxlnWinrefArg->GetOperator()->AssertValid(pdxlnWinrefArg, validate_children);
		}
	}
}
#endif // GPOS_DEBUG

// EOF
