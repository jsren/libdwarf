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
        EntryPoint          = 0x03, // An alternate entry point
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
        InlinedSubroutine   = 0x1D, // A particular inlined instance of a subroutine or function
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
        Subprogram          = 0x2E, // A subroutine or function
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
        Description,        // Artificial name or description
        DigitCount,         // Digit count for packed decimal or numeric string type
        Discr,              // Disriminant of variant part
        DiscrList,          // List of discriminant values
        Elemental,          // Elemental property of a subroutine
        Encoding,           // Encoding of a base type
        Endianity,          // Endianity of data
        EntryPC,            // Entry address of module init or (inlined-)subprogram
        EnumClass,          // Type-safe enumeration definition
        Explicit,           // Explicit property of a member function
        Extension,          // Previous namespace extension or original namespace
        External,           // External subroutine or variable
        FrameBase,          // Subroutine frame base address
        Friend,             // Friend relationship
        HighPC,             // Contiguous range of code addresses
        IdentifierCase,     // Identifier case rule
        Import,             // Imported declaration or unit; namespace alias; using declaration or directive
        Inline,             // Abstract instance or inlined subroutine
        IsOptional,         // Optional parameter
        Language,           // Programming language
        LinkageName,        // Object file linkage name of an entity
        Location,           // Data object location
        LowPC,              // Code address or range of addresses
        LowerBound,         // Lower bound of subrange
        MacroInfo,          // Macro information
        MainSubprogram,     // Main or starting subprogram or unit containing such
        Mutable,            // Mutable property of member data
        Name,               // Name of declaration or path name of compilation source
        NamelistItem,       // Namelist item
        ObjectPointer,      // Object (this, self) pointer of member interface
        Ordering,           // Array row/column ordering 
        PictureString,      // Picture string for numeric string type
        Priority,           // Module priority
        Producer,           // Compiler identification
        Prototyped,         // Subroutine prototype
        Pure,               // Pure property of a subroutine
        Ranges,             // Non-contiguous range of code addresses
        Recursive,          // Recursive property of a subroutine
        ReturnAddress,      // Subroutine return address save location
        Segment,            // Addressing information
        Sibling,            // Debugging information entry relationship
        Small,              // Scale factor for fixed-point type
        Signature,          // Type signature
        Specification,      // Incomplete, non-defining or separate declaration corresponding to a declaration
        StartScope,         // Object or type declaration
        StaticLink,         // Location of uplevel frame
        StmtList,           // Line number information for unit
        StringLength,       // String length of string type
        ThreadsScaled,      // UPC array bound THREADS scale factor
        Trampoline,         // Target subroutine
        Type,               // Type of declaration or subroutine return
        UpperBound,         // Upper bound of subrange
        UseLocation,        // Member location for pointer to member type
        UseUTF8,            // Compilation unit uses UTF-8 strings
        VariableParameter,  // Non-constant parameter flag
        Virtuality,         // Virtuality indication
        Visibility,         // Visibility of declaration
        VTableElemLocation  // Virtual function vtable slot
    };

    enum class BaseType : uint8_t
    {
        Address        = 0x01,
        Boolean        = 0x02,
        ComplexFloat   = 0x03,
        Float          = 0x04,
        Signed         = 0x05,
        SignedChar     = 0x06,
        Unsigned       = 0x07,
        UnsignedChar   = 0x08,
        ImaginaryFloat = 0x09,
        PackedDecimal  = 0x0A,
        NumericString  = 0x0B,
        Edited         = 0x0C,
        SignedFixed    = 0x0D,
        UnsignedFixed  = 0x0E,
        DecimalFloat   = 0x0F,
        UTF            = 0x10
    };

    enum class DecimalSign : uint8_t
    {
        Unsigned          = 0x01, // Unsigned
        LeadingOverpunch  = 0x02, // Sign is encoded in the most significant digit in a target-dependent manner
        TrailingOverpunch = 0x03, // Sign is encoded in the least significant digit in a target-dependent manner
        LeadingSeparate   = 0x04, // Sign is a '+' or '-' character to the left of the most significant digit
        TrailingSeparate  = 0x05  // Sign is a ‘+’ or ‘-’ character to the right of the least significant digit
    };

    enum class Endianness : uint8_t  // Dubbed "endianity" in spec
    {
        Default = 0x00,
        Big     = 0x01,
        Little  = 0x02
    };

    enum class Accessibility : uint8_t
    {
        Public    = 0x01,
        Protected = 0x02,
        Private   = 0x03
    };

    enum class Visibility : uint8_t
    {
        Local     = 0x01,
        Exported  = 0x02,
        Qualified = 0x03
    };

    enum class Virtuality : uint8_t
    {
        None        = 0x00,
        Virtual     = 0x01,
        PureVirtual = 0x02
    };

    enum class Language : uint16_t
    {
        C89          = 0x00,
        C            = 0x02,
        Ada83        = 0x03,
        CPlusPlus    = 0x04,
        Cobol74      = 0x05,
        Cobol85      = 0x06,
        Fortran77    = 0x07,
        Fortran90    = 0x08,
        Pascal83     = 0x09,
        Modula2      = 0x0A,
        Java         = 0x0B,
        C99          = 0x0C,
        Ada95        = 0x0D,
        Fortran95    = 0x0E,
        PLI          = 0x0F,
        ObjC         = 0x10,
        ObjCPlusPlus = 0x11,
        UPC          = 0x12,
        D            = 0x13,
        Python       = 0x14
    };

    enum class IdentifierCase : uint8_t
    {
        CaseSensitive   = 0x00,
        UpCase          = 0x01,
        DownCase        = 0x02,
        CaseInsensitive = 0x03
    };

    enum class CallingConvention : uint8_t
    {
        Normal  = 0x01,
        Program = 0x02,
        Nocall  = 0x03
    };

    enum class Inlining : uint8_t
    {
        NotInlined         = 0x00, // Not declared inline nor inlined by the compiler
        Inlined            = 0x01, // Not declared inline but inlined by the compiler 
        DeclaredNotInlined = 0x02, // Declared inline but not inlined by the compiler 
        DeclaredInlined    = 0x03  // Declared inline and inlined by the compiler
    };

    enum class ArrayOrdering : uint8_t
    {
        RowMajor = 0x0,
        ColMajor = 0x1
    };

    enum class DiscriminantList : uint8_t
    {
        Label = 0x0,
        Range = 0x1
    };

    enum class MacroInfoType : uint8_t
    {
        Define    = 0x01,
        Undef     = 0x02,
        StartFile = 0x03,
        EndFile   = 0x04,
        VendorExt = 0xFF
    };



}