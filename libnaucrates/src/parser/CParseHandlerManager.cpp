//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerManager.cpp
//
//	@doc:
//		Implementation of the controller for parse handlers.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/xml/CDXLMemoryManager.h"

using namespace gpdxl;

#define GPDXL_PARSE_CFA_FREQUENCY 50

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerManager::CParseHandlerManager
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerManager::CParseHandlerManager
	(
	CDXLMemoryManager *memory_manager_dxl,
	SAX2XMLReader *sax_2_xml_reader
	)
	:
	m_memory_manager_dxl(memory_manager_dxl),
	m_pxmlreader(sax_2_xml_reader),
	m_pphCurrent(NULL),
	m_ulIterLastCFA(0)
{
	m_pphstack = GPOS_NEW(memory_manager_dxl->Pmp()) PHStack(memory_manager_dxl->Pmp());
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerManager::~CParseHandlerManager
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CParseHandlerManager::~CParseHandlerManager()
{
	GPOS_DELETE(m_pphstack);
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerManager::ActivateParseHandler
//
//	@doc:
//		Activates the given parse handler and saves the current one on the stack
//
//---------------------------------------------------------------------------
void
CParseHandlerManager::ActivateParseHandler(CParseHandlerBase *pph)
{
	CheckForAborts();
	
	if (m_pphCurrent)
	{
		// push current handler on stack
		m_pphstack->Push(m_pphCurrent);
	}
	
	GPOS_ASSERT(NULL != pph);
	
	m_pphCurrent = pph;
	m_pxmlreader->setContentHandler(pph);
	m_pxmlreader->setErrorHandler(pph);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerManager::ReplaceHandler
//
//	@doc:
//		Activates the given parse handler and throws out the current one
//
//---------------------------------------------------------------------------
void
CParseHandlerManager::ReplaceHandler
	(
	CParseHandlerBase *pph,
	CParseHandlerBase *parse_handler_root
	)
{
	CheckForAborts();
	
	GPOS_ASSERT(NULL != m_pphCurrent);
	GPOS_ASSERT(NULL != pph);
	
	if (NULL != parse_handler_root)
	{
		parse_handler_root->ReplaceParseHandler(m_pphCurrent, pph);
	}
	
	m_pphCurrent = pph;
	m_pxmlreader->setContentHandler(pph);	
	m_pxmlreader->setErrorHandler(pph);
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerManager::DeactivateHandler
//
//	@doc:
//		Deactivates the current handler and activates the one on top of the
//		parser stack if one exists
//
//---------------------------------------------------------------------------
void
CParseHandlerManager::DeactivateHandler()
{

	CheckForAborts();
	
	GPOS_ASSERT(NULL != m_pphCurrent);

	if (!m_pphstack->IsEmpty())
	{
		m_pphCurrent = m_pphstack->Pop();
	}
	else
	{
		m_pphCurrent = NULL;
	}
	
	m_pxmlreader->setContentHandler(m_pphCurrent);
	m_pxmlreader->setErrorHandler(m_pphCurrent);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerManager::PphCurrent
//
//	@doc:
//		Returns the current handler
//
//---------------------------------------------------------------------------
const CParseHandlerBase *
CParseHandlerManager::PphCurrent()
{
	return m_pphCurrent;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerManager::CheckForAborts
//
//	@doc:
//		Increment CFA counter and check for aborts if necessary according to the
//		specified CFA frequency
//
//---------------------------------------------------------------------------
void
CParseHandlerManager::CheckForAborts()
{
	m_ulIterLastCFA++;
	
	if (GPDXL_PARSE_CFA_FREQUENCY < m_ulIterLastCFA)
	{
		GPOS_CHECK_ABORT;
		m_ulIterLastCFA = 0;
	}
}

// EOF
