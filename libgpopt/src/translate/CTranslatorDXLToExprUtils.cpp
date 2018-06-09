//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CTranslatorDXLToExprUtils.cpp
//
//	@doc:
//		Implementation of the helper methods used during DXL to Expr translation
//		
//---------------------------------------------------------------------------

#include "gpopt/translate/CTranslatorDXLToExprUtils.h"
#include "gpopt/mdcache/CMDAccessorUtils.h"

#include "naucrates/base/IDatumInt4.h"
#include "naucrates/base/IDatumInt8.h"

#include "naucrates/dxl/operators/CDXLScalarFuncExpr.h"
#include "naucrates/dxl/operators/CDXLScalarIdent.h"

#include "naucrates/md/IMDRelation.h"
#include "naucrates/md/IMDTypeInt8.h"
#include "naucrates/md/IMDCast.h"

using namespace gpos;
using namespace gpmd;
using namespace gpdxl;
using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExprUtils::PopConst
//
//	@doc:
// 		Construct const operator from a DXL const value operator
//
//---------------------------------------------------------------------------
CScalarConst *
CTranslatorDXLToExprUtils::PopConst
	(
	IMemoryPool *memory_pool,
	CMDAccessor *md_accessor,
	const CDXLScalarConstValue *dxl_op
	)
{
	IDatum *pdatum = CTranslatorDXLToExprUtils::Pdatum(md_accessor, dxl_op);
	return GPOS_NEW(memory_pool) CScalarConst(memory_pool, pdatum);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExprUtils::Pdatum
//
//	@doc:
// 		Construct a datum from a DXL const value operator
//
//---------------------------------------------------------------------------
IDatum *
CTranslatorDXLToExprUtils::Pdatum
	(
	CMDAccessor *md_accessor,
	const CDXLScalarConstValue *dxl_op
	)
{
	IMDId *mdid = dxl_op->GetDatumVal()->MDId();
	IDatum *pdatum = md_accessor->Pmdtype(mdid)->Pdatum(dxl_op);

	return pdatum;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExprUtils::Pdrgpdatum
//
//	@doc:
// 		Construct a datum array from a DXL datum array
//
//---------------------------------------------------------------------------
DrgPdatum *
CTranslatorDXLToExprUtils::Pdrgpdatum
	(
	IMemoryPool *memory_pool,
	CMDAccessor *md_accessor,
	const DXLDatumArray *pdrgpdxldatum
	)
{
	GPOS_ASSERT(NULL != pdrgpdxldatum);

	DrgPdatum *pdrgdatum = GPOS_NEW(memory_pool) DrgPdatum(memory_pool);
	const ULONG ulLen = pdrgpdxldatum->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		CDXLDatum *datum_dxl = (*pdrgpdxldatum)[ul];
		IMDId *mdid = datum_dxl->MDId();
		IDatum *pdatum = md_accessor->Pmdtype(mdid)->Pdatum(memory_pool, datum_dxl);
		pdrgdatum->Append(pdatum);
	}

	return pdrgdatum;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExprUtils::PexprConstInt8
//
//	@doc:
// 		Construct an expression representing the given value in INT8 format
//
//---------------------------------------------------------------------------
CExpression *
CTranslatorDXLToExprUtils::PexprConstInt8
	(
	IMemoryPool *memory_pool,
	CMDAccessor *md_accessor,
	CSystemId sysid,
	LINT val
	)
{
	IDatumInt8 *pdatum = md_accessor->PtMDType<IMDTypeInt8>(sysid)->PdatumInt8(memory_pool, val, false /* is_null */);
	CExpression *pexprConst = GPOS_NEW(memory_pool) CExpression(memory_pool, GPOS_NEW(memory_pool) CScalarConst(memory_pool, pdatum));

	return pexprConst;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExprUtils::AddKeySets
//
//	@doc:
// 		Add key sets info from the MD relation to the table descriptor
//
//---------------------------------------------------------------------------
void
CTranslatorDXLToExprUtils::AddKeySets
	(
	IMemoryPool *memory_pool,
	CTableDescriptor *ptabdesc,
	const IMDRelation *pmdrel,
	HMUlUl *phmululColMapping
	)
{
	GPOS_ASSERT(NULL != ptabdesc);
	GPOS_ASSERT(NULL != pmdrel);

	const ULONG ulKeySets = pmdrel->UlKeySets();
	for (ULONG ul = 0; ul < ulKeySets; ul++)
	{
		CBitSet *pbs = GPOS_NEW(memory_pool) CBitSet(memory_pool, ptabdesc->UlColumns());
		const ULongPtrArray *pdrgpulKeys = pmdrel->PdrgpulKeyset(ul);
		const ULONG ulKeys = pdrgpulKeys->Size();

		for (ULONG ulKey = 0; ulKey < ulKeys; ulKey++)
		{
			// populate current keyset
			ULONG ulOriginalKey = *((*pdrgpulKeys)[ulKey]);
			ULONG *pulRemappedKey = phmululColMapping->Find(&ulOriginalKey);
			GPOS_ASSERT(NULL != pulRemappedKey);
			
			pbs->ExchangeSet(*pulRemappedKey);
		}

		if (!ptabdesc->FAddKeySet(pbs))
		{
			pbs->Release();
		}
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExprUtils::FScalarBool
//
//	@doc:
//		Check if a dxl node is a scalar bool
//
//---------------------------------------------------------------------------
BOOL
CTranslatorDXLToExprUtils::FScalarBool
	(
	const CDXLNode *dxlnode,
	EdxlBoolExprType edxlboolexprtype
	)
{
	GPOS_ASSERT(NULL != dxlnode);

	CDXLOperator *dxl_op = dxlnode->GetOperator();
	if (EdxlopScalarBoolExpr == dxl_op->GetDXLOperator())
	{
		EdxlBoolExprType edxlboolexprtypeNode =
				CDXLScalarBoolExpr::Cast(dxl_op)->GetDxlBoolTypeStr();
		return edxlboolexprtype == edxlboolexprtypeNode;
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExprUtils::EBoolOperator
//
//	@doc:
// 		returns the equivalent bool expression type in the optimizer for
// 		a given DXL bool expression type

//---------------------------------------------------------------------------
CScalarBoolOp::EBoolOperator
CTranslatorDXLToExprUtils::EBoolOperator
	(
	EdxlBoolExprType edxlbooltype
	)
{
	CScalarBoolOp::EBoolOperator eboolop = CScalarBoolOp::EboolopSentinel;

	switch (edxlbooltype)
	{
		case Edxlnot:
			eboolop = CScalarBoolOp::EboolopNot;
			break;
		case Edxland:
			eboolop = CScalarBoolOp::EboolopAnd;
			break;
		case Edxlor:
			eboolop = CScalarBoolOp::EboolopOr;
			break;
		default:
			GPOS_ASSERT(!"Unrecognized boolean expression type");
	}

	GPOS_ASSERT(CScalarBoolOp::EboolopSentinel > eboolop);

	return eboolop;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExprUtils::Pdrgpcr
//
//	@doc:
// 		Construct a dynamic array of column references corresponding to the
//		given col ids
//
//---------------------------------------------------------------------------
DrgPcr *
CTranslatorDXLToExprUtils::Pdrgpcr
	(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr,
	const ULongPtrArray *pdrgpulColIds
	)
{
	GPOS_ASSERT(NULL != pdrgpulColIds);

	DrgPcr *pdrgpcr = GPOS_NEW(memory_pool) DrgPcr(memory_pool);

	for (ULONG ul = 0; ul < pdrgpulColIds->Size(); ul++)
	{
		ULONG *pulColId = (*pdrgpulColIds)[ul];
		const CColRef *pcr = phmulcr->Find(pulColId);
		GPOS_ASSERT(NULL != pcr);

		pdrgpcr->Append(const_cast<CColRef*>(pcr));
	}

	return pdrgpcr;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorDXLToExprUtils::FCastFunc
//
//	@doc:
//		Is the given expression is a scalar function that is used to cast
//
//---------------------------------------------------------------------------
BOOL
CTranslatorDXLToExprUtils::FCastFunc
	(
	CMDAccessor *md_accessor,
	const CDXLNode *dxlnode,
	IMDId *pmdidInput
	)
{
	GPOS_ASSERT(NULL != dxlnode);

	if (1 != dxlnode->Arity())
	{
		return false;
	}

	if (NULL == pmdidInput)
	{
		return false;
	}

	if (EdxlopScalarFuncExpr != dxlnode->GetOperator()->GetDXLOperator())
	{
		return false;
	}

	CDXLScalarFuncExpr *pdxlopScFunc = CDXLScalarFuncExpr::Cast(dxlnode->GetOperator());

	IMDId *mdid_dest = pdxlopScFunc->ReturnTypeMdId();

	if(!CMDAccessorUtils::FCastExists(md_accessor, pmdidInput, mdid_dest))
	{
		return false;
	}

	const IMDCast *pmdcast = md_accessor->Pmdcast(pmdidInput, mdid_dest);

	return (pmdcast->GetCastFuncMdId()->Equals(pdxlopScFunc->FuncMdId()));
}


// EOF
