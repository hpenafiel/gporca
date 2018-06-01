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
	IMDId *pmdidFunc,
	IMDId *pmdidRetType,
	BOOL fDistinct,
	BOOL fStarArg,
	BOOL fSimpleAgg,
	EdxlWinStage edxlwinstage,
	ULONG ulWinspecPosition
	)
	:
	CDXLScalar(memory_pool),
	m_pmdidFunc(pmdidFunc),
	m_pmdidRetType(pmdidRetType),
	m_fDistinct(fDistinct),
	m_fStarArg(fStarArg),
	m_fSimpleAgg(fSimpleAgg),
	m_edxlwinstage(edxlwinstage),
	m_ulWinspecPos(ulWinspecPosition)
{
	GPOS_ASSERT(m_pmdidFunc->IsValid());
	GPOS_ASSERT(m_pmdidRetType->IsValid());
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
	m_pmdidFunc->Release();
	m_pmdidRetType->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarWindowRef::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarWindowRef::Edxlop() const
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
			return CDXLTokens::PstrToken(edxltk);
			break;
		}
	}

	GPOS_ASSERT(!"Unrecognized window stage");
	return NULL;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarWindowRef::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarWindowRef::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarWindowref);
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
	const CWStringConst *element_name = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	m_pmdidFunc->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenWindowrefOid));
	m_pmdidRetType->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenTypeId));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenWindowrefDistinct),m_fDistinct);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenWindowrefStarArg),m_fStarArg);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenWindowrefSimpleAgg),m_fSimpleAgg);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenWindowrefStrategy), PstrWinStage());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenWindowrefWinSpecPos), m_ulWinspecPos);

	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarWindowRef::FBoolean
//
//	@doc:
//		Does the operator return a boolean result
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarWindowRef::FBoolean
	(
	CMDAccessor *pmda
	)
	const
{
	IMDId *pmdid = pmda->Pmdfunc(m_pmdidFunc)->PmdidTypeResult();
	return (IMDType::EtiBool == pmda->Pmdtype(pmdid)->Eti());
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
		GPOS_ASSERT(EdxloptypeScalar == pdxlnWinrefArg->GetOperator()->Edxloperatortype());

		if (validate_children)
		{
			pdxlnWinrefArg->GetOperator()->AssertValid(pdxlnWinrefArg, validate_children);
		}
	}
}
#endif // GPOS_DEBUG

// EOF
