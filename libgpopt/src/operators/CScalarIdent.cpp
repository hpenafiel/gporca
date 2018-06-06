//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CScalarIdent.cpp
//
//	@doc:
//		Implementation of scalar identity operator
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefTable.h"
#include "gpopt/operators/CScalarIdent.h"


using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CScalarIdent::HashValue
//
//	@doc:
//		Hash value built from colref and Eop
//
//---------------------------------------------------------------------------
ULONG
CScalarIdent::HashValue() const 
{
	return gpos::CombineHashes(COperator::HashValue(),
							   gpos::HashPtr<CColRef>(m_pcr));
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarIdent::FMatch
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarIdent::FMatch
	(
	COperator *pop
	)
const
{
	if (pop->Eopid() == Eopid())
	{
		CScalarIdent *popIdent = CScalarIdent::PopConvert(pop);
		
		// match if column reference is same
		return Pcr() == popIdent->Pcr();
	}
	
	return false;
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarIdent::FInputOrderSensitive
//
//	@doc:
//		Not called for leaf operators
//
//---------------------------------------------------------------------------
BOOL
CScalarIdent::FInputOrderSensitive() const
{
	GPOS_ASSERT(!"Unexpected call of function FInputOrderSensitive");
	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarIdent::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
COperator *
CScalarIdent::PopCopyWithRemappedColumns
	(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr,
	BOOL fMustExist
	)
{
	ULONG id = m_pcr->Id();
	CColRef *pcr = phmulcr->Find(&id);
	if (NULL == pcr)
	{
		if (fMustExist)
		{
			// not found in hashmap, so create a new colref and add to hashmap
			CColumnFactory *pcf = COptCtxt::PoctxtFromTLS()->Pcf();

			CName name(m_pcr->Name());
			pcr = pcf->PcrCreate(m_pcr->Pmdtype(), m_pcr->TypeModifier(), name);

#ifdef GPOS_DEBUG
			BOOL fResult =
#endif // GPOS_DEBUG
			phmulcr->Insert(GPOS_NEW(memory_pool) ULONG(id), pcr);
			GPOS_ASSERT(fResult);
		}
		else
		{
			pcr = const_cast<CColRef *>(m_pcr);
		}
	}

	return GPOS_NEW(memory_pool) CScalarIdent(memory_pool, pcr);
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarIdent::MDIdType
//
//	@doc:
//		Expression type
//
//---------------------------------------------------------------------------
IMDId*
CScalarIdent::MDIdType() const
{
	return m_pcr->Pmdtype()->MDId();
}

INT
CScalarIdent::TypeModifier() const
{
	return m_pcr->TypeModifier();
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarIdent::FCastedScId
//
//	@doc:
// 		Is the given expression a scalar cast of a scalar identifier
//
//---------------------------------------------------------------------------
BOOL
CScalarIdent::FCastedScId
	(
	CExpression *pexpr
	)
{
	GPOS_ASSERT(NULL != pexpr);

	// cast(col1)
	if (COperator::EopScalarCast == pexpr->Pop()->Eopid())
	{
		if (COperator::EopScalarIdent == (*pexpr)[0]->Pop()->Eopid())
		{
			return true;
		}
	}

	return false;
}

BOOL
CScalarIdent::FCastedScId
	(
	CExpression *pexpr,
	CColRef *pcr
	)
{
	GPOS_ASSERT(NULL != pexpr);
	GPOS_ASSERT(NULL != pcr);

	if (!FCastedScId(pexpr))
	{
		return false;
	}

	CScalarIdent *pScIdent = CScalarIdent::PopConvert((*pexpr)[0]->Pop());

	return pcr == pScIdent->Pcr();
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarIdent::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CScalarIdent::OsPrint
	(
	IOstream &os
	)
	const
{
	os << SzId() << " ";
	m_pcr->OsPrint(os);
	
	return os;
}

// EOF

