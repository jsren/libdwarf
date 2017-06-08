/* elf.h - (c) James S Renwick 2015-2017 */
#pragma once

#include "platform.h"
#include "../pointer.h"

#include <stdint.h>
#include <memory>

#define EI_NIDENT 16


namespace elf
{

    typedef signed long int error_t;

    _pack_start
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
    } _pack_end;

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

    _pack_start 
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

    } _pack_end;

    struct SectionTable32
    {
        Pointer<const SectionHeader32[]> headers;
        uint16_t headerCount;

        inline const SectionHeader32& operator[](size_t index) const {
            return headers[index];
        }
    };


    enum class SegmentType : uint32_t
    {
        Null          = 0x0, // Unused - ignore entry
        Load          = 0x1, // Segment is loadable
        Dynamic       = 0x2, // Segment contains dynamic linking information
        Interpreted   = 0x3, // Interpreter location for interpreted executables
        Note          = 0x4, // Location and size of auxiliary information
        SHLib         = 0x5, // Reserved
        ProgramHeader = 0x6  // Location and size of Program Header for loading
    };

    enum class SegmentFlags : uint32_t
    {
        Executable = 0x0, // Segment is executable
        Writable   = 0x1, // Segment can be written to
        Readable   = 0x2  // Segment can be read from
    };


    /* Entry describing a loaded program segment. */
    _pack_start
    struct ProgramHeaderEntry32
    {
        SegmentType  p_type;   // Segment type
        uint32_t     p_offset; // Segment offset in source file
        uint32_t     p_vaddr;  // Segment virtual address load location
        uint32_t     p_paddr;  // Segment physical address load location
        uint32_t     p_filesz; // Segment source size in bytes
        uint32_t     p_memsz;  // Segment load size in bytes
        SegmentFlags p_flags;  // Segment flags
        uint32_t     p_align;  // Segment load alignment

    } _pack_end;


    struct ProgramHeader32
    {
        Pointer<const ProgramHeaderEntry32[]> segments;
        uint32_t segmentCount;

        inline const ProgramHeaderEntry32& operator[](size_t index) const {
            return segments[index];
        }
    };



    struct StringTable32
    {
    private:
        static const char emptyName[1];

    public:
        Pointer<const char[]> buffer{};
        uint32_t size = 0;

        StringTable32() = default;

        StringTable32(Pointer<const char[]> buffer, uint32_t size) 
            : buffer(std::move(buffer)), size(size) { }
    public:
        const char* operator[](size_t index) const {
            return size != 0 ? &buffer[index] : emptyName;
        }
    };


    enum class SymbolBinding : uint8_t
    {
        Local  = 0,
        Global = 1,
        Weak   = 2
    };

    enum class SymbolType : uint8_t
    {
        None     = 0,
        Object   = 1,
        Function = 2,
        Section  = 3,
        File     = 4
    };

    _pack_start
    struct SymbolTableEntry32
    {
        uint32_t st_name;
        uint32_t st_value;
        uint32_t st_size;
        
        SymbolType    st_info_type    : 4;
        SymbolBinding st_info_binding : 4;
        uint8_t       st_other;
        uint16_t      st_shndx; // Section header index (or special?)

    } _pack_end;

    static_assert(sizeof(SymbolTableEntry32) == 16, "");


    struct SymbolTable32
    {
        Pointer<const SymbolTableEntry32[]> entries;
        uint32_t entryCount;

         const SymbolTableEntry32& operator[](size_t index) const {
            return entries[index];
        }
    };

    _pack_start
    struct NoteEntry
    {
        uint32_t namesz;
        uint32_t descsz;
        uint32_t type;

        Pointer<const char[]> name;
        Pointer<const char[]> description;
    } _pack_end;



    error_t parseElfHeader(const uint8_t* buffer, size_t bufferSize,
        Header32& header_out) noexcept;

    error_t parseSectionHeader(const uint8_t* buffer, size_t bufferSize,
        SectionHeader32& header_out) noexcept;

	error_t parseSectionTable(const uint8_t* buffer, size_t bufferSize,
        uint16_t sectionCount, SectionTable32& table_out, bool copyData = true);

    error_t parseSymbolTable(const uint8_t* buffer, size_t bufferSize,
        uint32_t entryCount, SymbolTable32& table_out, bool copyData = true);

    error_t parseProgramHeader(const uint8_t* buffer, size_t bufferSize,
        uint16_t entryCount, ProgramHeader32& header_out, bool copyData = true);
}
