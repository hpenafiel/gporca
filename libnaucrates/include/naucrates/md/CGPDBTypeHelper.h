//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CGPDBTypeHelper.h
//
//	@doc:
//		Helper class that provides implementation for common functions across
//		different GPDB types (CMDTypeInt4GPDB, CMDTypeBoolGPDB, and CMDTypeGenericGPDB)
//---------------------------------------------------------------------------
#ifndef GPMD_CGPDBHelper_H
#define GPMD_CGPDBHelper_H

#include "gpos/base.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"


namespace gpmd
{
	using namespace gpos;
	using namespace gpdxl;

	template<class T>
	class CGPDBTypeHelper
	{

		public:

			// serialize object in DXL format
			static void Serialize(CXMLSerializer *xml_serializer, const T *pt)
            {
                xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
                                     CDXLTokens::GetDXLTokenStr(EdxltokenMDType));

                pt->MDId()->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMdid));

                xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenName), pt->Mdname().GetMDName());
                xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeRedistributable), pt->FRedistributable());
                xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeHashable), pt->FHashable());
                xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeComposite), pt->FComposite());

                if (pt->FComposite())
                {
                    pt->PmdidBaseRelation()->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeRelid));
                }

                xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeFixedLength), pt->FFixedLength());

                if (pt->FFixedLength())
                {
                    xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeLength), pt->Length());
                }

                xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeByValue), pt->IsPassedByValue());

                pt->SerializeMDIdAsElem(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeEqOp), pt->PmdidCmp(IMDType::EcmptEq));
                pt->SerializeMDIdAsElem(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeNEqOp), pt->PmdidCmp(IMDType::EcmptNEq));
                pt->SerializeMDIdAsElem(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeLTOp), pt->PmdidCmp(IMDType::EcmptL));
                pt->SerializeMDIdAsElem(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeLEqOp), pt->PmdidCmp(IMDType::EcmptLEq));
                pt->SerializeMDIdAsElem(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeGTOp), pt->PmdidCmp(IMDType::EcmptG));
                pt->SerializeMDIdAsElem(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeGEqOp), pt->PmdidCmp(IMDType::EcmptGEq));
                pt->SerializeMDIdAsElem(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeCompOp), pt->PmdidOpComp());
                pt->SerializeMDIdAsElem(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeArray), pt->PmdidTypeArray());

                pt->SerializeMDIdAsElem(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeAggMin), pt->PmdidAgg(IMDType::EaggMin));
                pt->SerializeMDIdAsElem(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeAggMax), pt->PmdidAgg(IMDType::EaggMax));
                pt->SerializeMDIdAsElem(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeAggAvg), pt->PmdidAgg(IMDType::EaggAvg));
                pt->SerializeMDIdAsElem(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeAggSum), pt->PmdidAgg(IMDType::EaggSum));
                pt->SerializeMDIdAsElem(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeAggCount), pt->PmdidAgg(IMDType::EaggCount));

                xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
                                      CDXLTokens::GetDXLTokenStr(EdxltokenMDType));

                GPOS_CHECK_ABORT;
            }

#ifdef GPOS_DEBUG
			// debug print of the type in the provided stream
			static void DebugPrint(IOstream &os, const T *pt)
            {
                os << "Type id: ";
                pt->MDId()->OsPrint(os);
                os << std::endl;

                os << "Type name: " << pt->Mdname().GetMDName()->GetBuffer() << std::endl;

                const CWStringConst *pstrRedistributable = pt->FRedistributable() ?
                CDXLTokens::GetDXLTokenStr(EdxltokenTrue):
                CDXLTokens::GetDXLTokenStr(EdxltokenFalse);

                os << "Redistributable: " << pstrRedistributable->GetBuffer() << std::endl;

                const CWStringConst *pstrFixedLength = pt->FFixedLength() ?
                CDXLTokens::GetDXLTokenStr(EdxltokenTrue):
                CDXLTokens::GetDXLTokenStr(EdxltokenFalse);

                os << "Fixed length: " << pstrFixedLength->GetBuffer() << std::endl;

                if (pt->FFixedLength())
                {
                    os << "Type length: " << pt->Length() << std::endl;
                }

                const CWStringConst *pstrByValue = pt->IsPassedByValue() ?
                CDXLTokens::GetDXLTokenStr(EdxltokenTrue):
                CDXLTokens::GetDXLTokenStr(EdxltokenFalse);

                os << "Pass by value: " << pstrByValue->GetBuffer() << std::endl;

                os << "Equality operator id: ";
                pt->PmdidCmp(IMDType::EcmptEq)->OsPrint(os);
                os << std::endl;

                os << "Less-than operator id: ";
                pt->PmdidCmp(IMDType::EcmptL)->OsPrint(os);
                os << std::endl;

                os << "Greater-than operator id: ";
                pt->PmdidCmp(IMDType::EcmptG)->OsPrint(os);
                os << std::endl;

                os << "Comparison operator id: ";
                pt->PmdidOpComp()->OsPrint(os);
                os << std::endl;

                os << std::endl;
            }
#endif // GPOS_DEBUG
	};
}

#endif // !CGPMD_GPDBHelper_H

// EOF

