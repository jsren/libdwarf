#pragma once

#include <stdio.h>
#include <tchar.h>
#include <stdint.h>

namespace dwarf
{

    enum class DIEType : uint16_t
    {
        ArrayType           = 0x01,
        ClassType           = 0x02,
        EntryPoint          = 0x03,
        EnumerationType     = 0x04,
        FormalParameter     = 0x05,
        ImportedDeclaration = 0x08,
        Label               = 0x0A,
        LexicalBlock        = 0x0B,
        Member              = 0x0D,
        PointerType         = 0x0F,
        ReferenceType       = 0x10,
        CompileUnit         = 0x11,
        StringType          = 0x12,
        StructureType       = 0x13,
        SubroutineType      = 0x15,
        Typedef             = 0x16,
        UnionType           = 0x17,
        UnspecifiedParams   = 0x18,
        Variant             = 0x19,
        CommonBlock         = 0x1A,
        CommonInclusion     = 0x1B,
        Inheritance         = 0x1C,
        InlinedSubroutine   = 0x1D,
        Module              = 0x1E,
        PtrToMemberType     = 0x1F,
        SetType             = 0x20,
        SubrangeType        = 0x21,
        WithStmnt           = 0x22,
        AccessDeclaration   = 0x23,
        BaseType            = 0x24,
        CatchBlock          = 0x25,
        ConstType           = 0x26,
        Constant            = 0x27,
        Enumerator          = 0x28,
        FileType            = 0x29,
        Friend              = 0x2A,
        Namelist            = 0x2B,
        NamelistItem        = 0x2C,
        PackedType          = 0x2D,
        Subprogram          = 0x2E,
        TemplateTypeParam   = 0x2F,
        TemplateValueParam  = 0x30,
        ThrownType          = 0x31,
        TryBlock            = 0x32,
        VariantPart         = 0x33,
        Variable            = 0x34,
        VolatileType        = 0x35,
        DwarfProcedure      = 0x36,
        RestrictType        = 0x37,
        InterfaceType       = 0x38,
        Namespace           = 0x39,
        ImportedModule      = 0x3A,
        UnspecifiedType     = 0x3B,
        PartialUnit         = 0x3C,
        ImportedUnit        = 0x3D,
        Condition           = 0x3F,
        SharedType          = 0x40,
        TypeUnit            = 0x41,
        RValueReferenceType = 0x42,
        TemplateAlias       = 0x43
    };

    enum class Attribute
    {
        AbstractOrigin,     // Instance of inline subprogram
        Accessibility,      // C++ declarations, base classes & inherited members
        AddressClass,       // Pointer or reference or function ptr type
        Allocated,          // Allocation status of type
        Artificial,         // Marks an object or type not actually declared in the source
        Associated,         // Association status
        BaseTypes,          // Marks a primitive data type of a compilation unit
        BinaryScale,        // Binary scale factor for fixed-point type
        BitOffset,          // Base type or data member bit offset
        BitSize,            // Base type or data member bit size
        BitStride,          // Array element, subrange or enum stride
        ByteStride,         // Type or object size (bytes)
        CallColumn,         // Column position of inlined subroutine call
        CallFile,           // File of inlined subroutine call
        CallLine,           // Line number of inlined subroutine call
        CallingConvention,  // Subprogram calling convention
        CommonReference,    // Common block usage
        CompDir,            // Compilation directory
        ConstValue,         // Constant, enum literal, template value
        ConstExpr,          // Compile-time constant object or function
        ContainingType,     // Containing type of pointer-to-member type
        Count,              // Elements of subrange type
        DataBitOffset,      // 
        DataLocation,       // Indirection to actual data
        DataMemberLocation, // Data member & inherited member location
        DecimalScale,       // Decimal scale factor
        DecimalSign,        // Decimal sign representation
        DeclColumn,         // Column positino of source declaration
        DeclFile,           // File containing source declaration
        DeclLine,           // Line number of source declaration
        Declaration,        // Incomplete, non-defining or separate entity declartion
        DefaultValue,       // Default value of a parameter
        Description         // Artificial name or description
    };

}