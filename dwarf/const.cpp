#include "const.hpp"
#include <cstddef>

namespace dwarf
{
    struct _AttributeNameStrings { AttributeName enum_; const char* string; };

    static const _AttributeNameStrings _attributeNameStrings[] = {
        { AttributeName::None, "" },
        { AttributeName::AbstractOrigin, "AbstractOrigin" },
        { AttributeName::Accessibility, "Accessibility" },
        { AttributeName::AddressClass, "AddressClass" },
        { AttributeName::Allocated, "Allocated" },
        { AttributeName::Artificial, "Artificial" },
        { AttributeName::Associated, "Associated" },
        { AttributeName::BaseTypes, "BaseTypes" },
        { AttributeName::BinaryScale, "BinaryScale" },
        { AttributeName::BitOffset, "BitOffset" },
        { AttributeName::BitSize, "BitSize" },
        { AttributeName::BitStride, "BitStride" },
        { AttributeName::ByteSize, "ByteSize" },
        { AttributeName::ByteStride, "ByteStride" },
        { AttributeName::CallColumn, "CallColumn" },
        { AttributeName::CallFile, "CallFile" },
        { AttributeName::CallLine, "CallLine" },
        { AttributeName::CallingConvention, "CallingConvention" },
        { AttributeName::CommonReference, "CommonReference" },
        { AttributeName::CompDir, "CompDir" },
        { AttributeName::ConstValue, "ConstValue" },
        { AttributeName::ConstExpr, "ConstExpr" },
        { AttributeName::ContainingType, "ContainingType" },
        { AttributeName::Count, "Count" },
        { AttributeName::DataBitOffset, "DataBitOffset" },
        { AttributeName::DataLocation, "DataLocation" },
        { AttributeName::DataMemberLocation, "DataMemberLocation" },
        { AttributeName::DecimalScale, "DecimalScale" },
        { AttributeName::DecimalSign, "DecimalSign" },
        { AttributeName::DeclColumn, "DeclColumn" },
        { AttributeName::DeclFile, "DeclFile" },
        { AttributeName::DeclLine, "DeclLine" },
        { AttributeName::Declaration, "Declaration" },
        { AttributeName::DefaultValue, "DefaultValue" },
        { AttributeName::Description, "Description" },
        { AttributeName::DigitCount, "DigitCount" },
        { AttributeName::Discr, "Discr" },
        { AttributeName::DiscrList, "DiscrList" },
        { AttributeName::DiscrValue, "DiscrValue" },
        { AttributeName::Elemental, "Elemental" },
        { AttributeName::Encoding, "Encoding" },
        { AttributeName::Endianity, "Endianity" },
        { AttributeName::EntryPC, "EntryPC" },
        { AttributeName::EnumClass, "EnumClass" },
        { AttributeName::Explicit, "Explicit" },
        { AttributeName::Extension, "Extension" },
        { AttributeName::External, "External" },
        { AttributeName::FrameBase, "FrameBase" },
        { AttributeName::Friend, "Friend" },
        { AttributeName::HighPC, "HighPC" },
        { AttributeName::IdentifierCase, "IdentifierCase" },
        { AttributeName::Import, "Import" },
        { AttributeName::Inline, "Inline" },
        { AttributeName::IsOptional, "IsOptional" },
        { AttributeName::Language, "Language" },
        { AttributeName::LinkageName, "LinkageName" },
        { AttributeName::Location, "Location" },
        { AttributeName::LowPC, "LowPC" },
        { AttributeName::LowerBound, "LowerBound" },
        { AttributeName::MacroInfo, "MacroInfo" },
        { AttributeName::MainSubprogram, "MainSubprogram" },
        { AttributeName::Mutable, "Mutable" },
        { AttributeName::Name, "Name" },
        { AttributeName::NamelistItem, "NamelistItem" },
        { AttributeName::ObjectPointer, "ObjectPointer" },
        { AttributeName::Ordering, "Ordering" },
        { AttributeName::PictureString, "PictureString" },
        { AttributeName::Priority, "Priority" },
        { AttributeName::Producer, "Producer" },
        { AttributeName::Prototyped, "Prototyped" },
        { AttributeName::Pure, "Pure" },
        { AttributeName::Ranges, "Ranges" },
        { AttributeName::Recursive, "Recursive" },
        { AttributeName::ReturnAddress, "ReturnAddress" },
        { AttributeName::Segment, "Segment" },
        { AttributeName::Sibling, "Sibling" },
        { AttributeName::Small, "Small" },
        { AttributeName::Signature, "Signature" },
        { AttributeName::Specification, "Specification" },
        { AttributeName::StartScope, "StartScope" },
        { AttributeName::StaticLink, "StaticLink" },
        { AttributeName::StmtList, "StmtList" },
        { AttributeName::StringLength, "StringLength" },
        { AttributeName::ThreadsScaled, "ThreadsScaled" },
        { AttributeName::Trampoline, "Trampoline" },
        { AttributeName::Type, "Type" },
        { AttributeName::UpperBound, "UpperBound" },
        { AttributeName::UseLocation, "UseLocation" },
        { AttributeName::UseUTF8, "UseUTF8" },
        { AttributeName::VariableParameter, "VariableParameter" },
        { AttributeName::Virtuality, "Virtuality" },
        { AttributeName::Visibility, "Visibility" },
        { AttributeName::VTableElemLocation, "VTableElemLocation" }
    };
    const auto _attributeNameStringsCount = sizeof(_attributeNameStrings) / sizeof(AttributeName);


    const char* AttributeNameToString(AttributeName name)
    {
        for (std::size_t i = 0; i < _attributeNameStringsCount; i++)
        {
            if (_attributeNameStrings[i].enum_ == name)
                return _attributeNameStrings[i].string;
        }
        return nullptr;
    }
}
