/* format.h - (c) James S Renwick 2015 */
/*
   Each debugging information entry begins with an unsigned LEB128 number 
   containing the abbreviation code for the entry. This code represents an 
   entry within the abbreviations table associated with the compilation unit 
   containing this entry. The abbreviation code is followed by a series of 
   attribute values.
*/

#pragma once
#include <stdint.h>

namespace dwarf
{

    int32_t uleb_read(uint8_t *data, /*out*/ uint32_t &value);
    int32_t uleb_read(uint8_t *data, /*out*/ uint64_t &value);


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

    // .debug_info section header
    struct CompilationUnitHeader32
    {
    public:
        uint32_t unitLength;
        uint16_t version;
        uint32_t debugAbbrevOffset;
        uint8_t  addressSize;
    };
    struct TypeUnitHeader32
    {
    public:
        uint32_t unitLength;
        uint16_t version;
        uint32_t debugAbbrevOffset;
        uint8_t  addressSize;
        uint64_t typeSignature;
        uint32_t typeOffset;
    };


#pragma region .DEBUG_PUBNAMES/.DEBUG_PUBTYPES

    __pragma(pack(push, 1))
    struct NameTableHeader32
    {
    public:
        uint32_t unitLength;      // Length of the set of entries for this compilation unit, not including the 
                                  // length field itself
        uint16_t version;         // Version identifier containing the value 2
        uint32_t debugInfoOffset; // Offset into the .debug_info section of the compilation unit header
        uint32_t debugInfoLength; // Length containing the size in bytes of the contents of the .debug_info section
                                  // generated to represent this compilation unit
    };
    __pragma(pack(pop))

    __pragma(pack(push, 1))
    struct NameTableHeader64
    {
    public:
        unsigned : 32;            // Should be 0xFFFFFFFF
        uint64_t unitLength;      // Length of the set of entries for this compilation unit, not including the 
                                  // length field itself
        uint16_t version;         // Version identifier containing the value 2
        uint64_t debugInfoOffset; // Offset into the .debug_info section of the compilation unit header
        uint64_t debugInfoLength; // Length containing the size in bytes of the contents of the .debug_info section
                                  // generated to represent this compilation unit
    };
    __pragma(pack(pop))

#pragma endregion


#pragma region .DEBUG_ARRANGES
    __pragma(pack(push, 1))
    struct AddressRangeTableHeader32
    {
    public:
        uint32_t unitLength;      // Length of the set of entries for this compilation unit, not including the 
                                  // length field itself
        uint16_t version;         // Version identifier containing the value 2
        uint32_t debugInfoOffset; // Offset into the .debug_info section of the compilation unit header
        uint8_t  addressSize;     // The size in bytes of an address (or the offset portion for segmented) on the target system
        uint8_t  segmentSize;     // The size in bytes of a segment selector on the target system
    };
    __pragma(pack(pop))

    __pragma(pack(push, 1))
    struct AddressRangeTableHeader64
    {
    public:
        unsigned : 32;            // Should be 0xFFFFFFFF
        uint64_t unitLength;      // Length of the set of entries for this compilation unit, not including the 
                                  // length field itself
        uint16_t version;         // Version identifier containing the value 2
        uint64_t debugInfoOffset; // Offset into the .debug_info section of the compilation unit header
        uint8_t  addressSize;     // The size in bytes of an address (or the offset portion for segmented) on the target system
        uint8_t  segmentSize;     // The size in bytes of a segment selector on the target system
    };
    __pragma(pack(pop))
#pragma endregion

}
