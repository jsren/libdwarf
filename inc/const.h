/* const.h - (c) James S Renwick 2015 */
#pragma once

#include <stdint.h>

namespace dwarf
{
    // Tags
    enum class DIEType : uint16_t
    {
        None                = 0x00,
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

    enum class AttributeName : uint16_t
    {
        AbstractOrigin     = 0x31, // Instance of inline subprogram
        Accessibility      = 0x32, // C++ declarations, base classes & inherited members
        AddressClass       = 0x33, // Pointer or reference or function ptr type
        Allocated          = 0x4E, // Allocation status of type
        Artificial         = 0x34, // Marks an object or type not actually declared in the source
        Associated         = 0x4F, // Association status
        BaseTypes          = 0x35, // Marks a primitive data type of a compilation unit
        BinaryScale        = 0x5B, // Binary scale factor for fixed-point type
        BitOffset          = 0x0C, // Base type or data member bit offset
        BitSize            = 0x0D, // Base type or data member bit size
        BitStride          = 0x2E, // Array element, subrange or enum stride
        ByteSize           = 0x0B, // Data object or data type size
        ByteStride         = 0x51, // Type or object size (bytes)
        CallColumn         = 0x57, // Column position of inlined subroutine call
        CallFile           = 0x58, // File of inlined subroutine call
        CallLine           = 0x59, // Line number of inlined subroutine call
        CallingConvention  = 0x36, // Subprogram calling convention
        CommonReference    = 0x1A, // Common block usage
        CompDir            = 0x1B, // Compilation directory
        ConstValue         = 0x1C, // Constant, enum literal, template value
        ConstExpr          = 0x6C, // Compile-time constant object or function
        ContainingType     = 0x1D, // Containing type of pointer-to-member type
        Count              = 0x37, // Elements of subrange type
        DataBitOffset      = 0x6B, // 
        DataLocation       = 0x50, // Indirection to actual data
        DataMemberLocation = 0x38, // Data member & inherited member location
        DecimalScale       = 0x5C, // Decimal scale factor
        DecimalSign        = 0x5E, // Decimal sign representation
        DeclColumn         = 0x39, // Column positino of source declaration
        DeclFile           = 0x3A, // File containing source declaration
        DeclLine           = 0x3B, // Line number of source declaration
        Declaration        = 0x3C, // Incomplete, non-defining or separate entity declartion
        DefaultValue       = 0x1E, // Default value of a parameter
        Description        = 0x5A, // Artificial name or description
        DigitCount         = 0x5F, // Digit count for packed decimal or numeric string type
        Discr              = 0x15, // Disriminant of variant part
        DiscrList          = 0x3D, // IList of discriminant values
        DiscrValue         = 0x16,
        Elemental          = 0x66, // Elemental property of a subroutine
        Encoding           = 0x3E, // Encoding of a base type
        Endianity          = 0x65, // Endianity of data
        EntryPC            = 0x52, // Entry address of module init or (inlined-)subprogram
        EnumClass          = 0x6D, // Type-safe enumeration definition
        Explicit           = 0x63, // Explicit property of a member function
        Extension          = 0x54, // Previous namespace extension or original namespace
        External           = 0x3F, // External subroutine or variable
        FrameBase          = 0x40, // Subroutine frame base address
        Friend             = 0x41, // Friend relationship
        HighPC             = 0x12, // Contiguous range of code addresses
        IdentifierCase     = 0x42, // Identifier case rule
        Import             = 0x18, // Imported declaration or unit; namespace alias; using declaration or directive
        Inline             = 0x20, // Abstract instance or inlined subroutine
        IsOptional         = 0x21, // Optional parameter
        Language           = 0x13, // Programming language
        LinkageName        = 0x6E, // Object file linkage name of an entity
        Location           = 0x02, // Data object location
        LowPC              = 0x11, // Code address or range of addresses
        LowerBound         = 0x22, // Lower bound of subrange
        MacroInfo          = 0x43, // Macro information
        MainSubprogram     = 0x6A, // Main or starting subprogram or unit containing such
        Mutable            = 0x61, // Mutable property of member data
        Name               = 0x03, // Name of declaration or path name of compilation source
        NamelistItem       = 0x44, // Namelist item
        ObjectPointer      = 0x64, // Object (this, self) pointer of member interface
        Ordering           = 0x09, // Array row/column ordering 
        PictureString      = 0x60, // Picture string for numeric string type
        Priority           = 0x45, // Module priority
        Producer           = 0x25, // Compiler identification
        Prototyped         = 0x27, // Subroutine prototype
        Pure               = 0x67, // Pure property of a subroutine
        Ranges             = 0x55, // Non-contiguous range of code addresses
        Recursive          = 0x68, // Recursive property of a subroutine
        ReturnAddress      = 0x2A, // Subroutine return address save location
        Segment            = 0x46, // Addressing information
        Sibling            = 0x01, // Debugging information entry relationship
        Small              = 0x5D, // Scale factor for fixed-point type
        Signature          = 0x69, // Type signature
        Specification      = 0x47, // Incomplete, non-defining or separate declaration corresponding to a declaration
        StartScope         = 0x2C, // Object or type declaration
        StaticLink         = 0x48, // Location of uplevel frame
        StmtList           = 0x10, // Line number information for unit
        StringLength       = 0x19, // String length of string type
        ThreadsScaled      = 0x62, // UPC array bound THREADS scale factor
        Trampoline         = 0x56, // Target subroutine
        Type               = 0x49, // Type of declaration or subroutine return
        UpperBound         = 0x2F, // Upper bound of subrange
        UseLocation        = 0x4A, // Member location for pointer to member type
        UseUTF8            = 0x53, // Compilation unit uses UTF-8 strings
        VariableParameter  = 0x4B, // Non-constant parameter flag
        Virtuality         = 0x4C, // Virtuality indication
        Visibility         = 0x17, // Visibility of declaration
        VTableElemLocation = 0x4D  // Virtual function vtable slot
    };

    enum class AttributeForm
    {
        Address     = 0x01,
        Block2      = 0x03,
        Block4      = 0x04,
        Data2       = 0x05,
        Data4       = 0x06,
        Data8       = 0x07,
        String      = 0x08,
        Block       = 0x09,
        Block1      = 0x0A,
        Data1       = 0x0B,
        Flag        = 0x0C, // Single-byte flag. 0x00 indicates not present.
        SData       = 0x0D,
        Strp        = 0x0E,
        UData       = 0x0F,
        RefAddr     = 0x10,
        Ref1        = 0x11,
        Ref2        = 0x12,
        Ref4        = 0x13,
        Ref8        = 0x14,
        RefUData    = 0x15,
        Indirect    = 0x16,
        SecOffset   = 0x17,
        ExprLoc     = 0x18, // DWARF Expression or Location Description
        FlagPresent = 0x19, // Single-byte flag
        RefSig8     = 0x20,
    };

    enum class AttributeClass
    {
        None,
        Address,
        Block,
        Constant,
        String,
        Flag,
        Reference,      // DIE reference within section
        UnitReference,  // DIE reference within compilation unit
        SectionPointer,
        ExprLoc
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

    enum class OpCode : uint8_t
    {
        Address = 0x03,
        Deref   = 0x06,

        Const1U = 0x08, // 1-byte unsigned constant
        Const1S = 0x09, // 1-byte signed constant
        Const2U = 0x0A, // 2-byte unsigned constant
        Const2S = 0x0B, // 2-byte signed constant
        Const4U = 0x0C, // 4-byte unsigned constant
        Const4S = 0x0D, // 4-byte signed constant
        Const8U = 0x0E, // 8-byte unsigned constant
        Const8S = 0x0F, // 8-byte signed constant
        ConstU  = 0x10, // ULEB128 constant
        ConstS  = 0x11, // SLEB128 constant

        Dup        = 0x12,
        Drop       = 0x13,
        Over       = 0x14,
        Pick       = 0x15,
        Swap       = 0x16,
        Rot        = 0x17,
        XDeref     = 0x18,
        Abs        = 0x19,
        And        = 0x1A,
        Div        = 0x1B,
        Minus      = 0x1C,
        Mod        = 0x1D,
        Mul        = 0x1E,
        Neg        = 0x1F,
        Not        = 0x20,
        Or         = 0x21,
        Plus       = 0x22,
        PlusUConst = 0x23,
        Shl        = 0x24,
        Shr        = 0x25,
        Shra       = 0x26,
        Xor        = 0x27,
        Skip       = 0x2F,
        Bra        = 0x28,
        Eq         = 0x29,
        Ge         = 0x2A,
        Gt         = 0x2B,
        Le         = 0x2C,
        Lt         = 0x2D,
        Ne         = 0x2E,
        //LitBase  = 0x30, // Literal values from 0 to 31 inclusive
        //RegBase  = 0x50, // Register nos. from 0 to 31 inclusive
        RegX       = 0x90,
        FBReg      = 0x91,
        BRegX      = 0x92,
        Piece      = 0x93,
        DerefSize  = 0x94,
        XDerefSize = 0x95,
        Nop        = 0x96,

        PushObjectAddress = 0x97,
        Call2             = 0x98,
        Call4             = 0x99,
        CallRef           = 0x9A,
        FormTLSAddress    = 0x9B,
        CallFrameCFA      = 0x9C,
        BitPiece          = 0x9D,
        ImplicitValue     = 0x9E,
        StackValue        = 0x9F
    };


    /* Opcodes used in line number calculation */
    enum class LineOpCode : uint8_t
    {
        Copy             = 0x01,
        AdvancePC        = 0x02,
        AdvanceLine      = 0x03,
        SetFile          = 0x04,
        SetColumn        = 0x05,
        NegateStmnt      = 0x06,
        SetBasicBlock    = 0x07,
        ConstAddPC       = 0x08,
        FixedAdvancePC   = 0x09,
        SetPrologueEnd   = 0x0A,
        SetEpilogueBegin = 0x0B,
        SetISA           = 0x0C
    };

    /* Extended opcodes used in line number calculation */
    enum class LineOpCodeEx : uint8_t
    {
        EndSequence      = 0x01,
        SetAddress       = 0x02,
        DefineFile       = 0x03,
        SetDescriminator = 0x04
    };

}