#pragma once
#include <stdint.h>

#define EI_NIDENT 16

namespace dwarf
{
    struct Header32
    {
    public:
        unsigned char e_ident[EI_NIDENT]; // ELF Magic
        uint16_t      e_type;             // Object file type
        uint16_t      e_machine;          // Architecture
        uint32_t      e_version;          // Object file version
        uint32_t      e_entry;            // Virtual entry point address
        uint32_t      e_phoff;            // Program header table offset
        uint32_t      e_shoff;            // Section header table offset
        uint32_t      e_flags;            // Processor-specific flags
        uint16_t      e_ehsize;           // Header size
        uint16_t      e_phentsize;        // Program header table entry size
        uint16_t      e_phnum;            // Program header table entry count
        uint16_t      e_shentsize;        // Section header table entry size
        uint16_t      e_shnum;            // Section header table entry count
        uint16_t      e_shstrndx;         // Index of section name string table in header table
    };

    enum class SectionType : uint32_t
    {
        Null     = 0x0, // Inactive
        ProgBits = 0x1, // Program-specific
        SymTab   = 0x2, // Symbol table
        StrTab   = 0x3, // String table
        Rela     = 0x4, // Relocation entries
        Hash     = 0x5, // Symbol hash table
        Dynamic  = 0x6, // Dynamic linking info
        Note     = 0x7, // Note? See spec.
        NoBits   = 0x8, // Program-specific, occupies no space
        Rel      = 0x9, // Relocation entries w/o explicit addends
        ShLib    = 0xA, // Reserved. ABI-rejected.
        DynSym   = 0xB  // Dynamic linking symbol table
    };

    struct SectionHeader32
    {
        uint32_t    sh_name;      // String table index of section name
        SectionType sh_type;      // Section type
        uint32_t    sh_flags;     // Section flags
        uint32_t    sh_addr;      // Section load address
        uint32_t    sh_offset;    // Section offset in object file
        uint32_t    sh_size;      // Section size
        uint32_t    sh_link;      // Index of associated section
        uint32_t    sh_info;      // Additional type-specific info
        uint32_t    sh_addralign; // Section alignment constraints
        uint32_t    sh_entsize;   // Entry size for tabular sections
    };
    
    bool loadHeader(const char *filepath, Header32 *header);
}
