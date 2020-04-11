/* elf.h - (c) James S Renwick 2015-2017 */
#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <cstring>

#define EI_NIDENT 16


namespace elf
{
    enum class ElfClass : std::uint8_t
    {
        class32 = 1,
        class64 = 2
    };


    struct __attribute__((packed)) Header32
    {
        unsigned char e_ident[EI_NIDENT]; // ELF Magic
        std::uint16_t e_type;        // Object file type
        std::uint16_t e_machine;     // Architecture
        std::uint32_t e_version;     // Object file version
        std::uint32_t e_entry;       // Virtual entry point address
        std::uint32_t e_phoff;       // Program header table offset
        std::uint32_t e_shoff;       // Section header table offset
        std::uint32_t e_flags;       // Processor-specific flags
        std::uint16_t e_ehsize;      // Header size
        std::uint16_t e_phentsize;   // Program header table entry size
        std::uint16_t e_phnum;       // Program header table entry count
        std::uint16_t e_shentsize;   // Section header table entry size
        std::uint16_t e_shnum;       // Section header table entry count
        std::uint16_t e_shstrndx;    // Index of section name string table in header table
    };


    struct __attribute__((packed)) Header64
    {
        unsigned char e_ident[EI_NIDENT]; // ELF Magic
        std::uint16_t e_type;        // Object file type
        std::uint16_t e_machine;     // Architecture
        std::uint32_t e_version;     // Object file version
        std::uint64_t e_entry;       // Virtual entry point address
        std::uint64_t e_phoff;       // Program header table offset
        std::uint64_t e_shoff;       // Section header table offset
        std::uint32_t e_flags;       // Processor-specific flags
        std::uint16_t e_ehsize;      // Header size
        std::uint16_t e_phentsize;   // Program header table entry size
        std::uint16_t e_phnum;       // Program header table entry count
        std::uint16_t e_shentsize;   // Section header table entry size
        std::uint16_t e_shnum;       // Section header table entry count
        std::uint16_t e_shstrndx;    // Index of section name string table in header table

        inline constexpr ElfClass el_class() const noexcept {
            return static_cast<ElfClass>(e_ident[4]);
        }
    };
    using Header = Header64;


    enum class SectionType : std::uint32_t
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


    struct __attribute__((packed)) SectionHeader32
    {
        std::uint32_t    sh_name;      // String table index of section name
        SectionType sh_type;           // Section type
        std::uint32_t    sh_flags;     // Section flags
        std::uint32_t    sh_addr;      // Section load address
        std::uint32_t    sh_offset;    // Section offset in object file
        std::uint32_t    sh_size;      // Section size
        std::uint32_t    sh_link;      // Index of associated section
        std::uint32_t    sh_info;      // Additional type-specific info
        std::uint32_t    sh_addralign; // Section alignment constraints
        std::uint32_t    sh_entsize;   // Entry size for tabular sections
    };
    static_assert(sizeof(SectionHeader32) == 40, "");


    struct __attribute__((packed)) SectionHeader64
    {
        std::uint32_t    sh_name;      // String table index of section name
        SectionType sh_type;           // Section type
        std::uint64_t    sh_flags;     // Section flags
        std::uint64_t    sh_addr;      // Section load address
        std::uint64_t    sh_offset;    // Section offset in object file
        std::uint64_t    sh_size;      // Section size
        std::uint32_t    sh_link;      // Index of associated section
        std::uint32_t    sh_info;      // Additional type-specific info
        std::uint64_t    sh_addralign; // Section alignment constraints
        std::uint64_t    sh_entsize;   // Entry size for tabular sections
    };
    using SectionHeader = SectionHeader64;


    enum class SegmentType : std::uint32_t
    {
        Null          = 0x0, // Unused - ignore entry
        Load          = 0x1, // Segment is loadable
        Dynamic       = 0x2, // Segment contains dynamic linking information
        Interpreted   = 0x3, // Interpreter location for interpreted executables
        Note          = 0x4, // Location and size of auxiliary information
        SHLib         = 0x5, // Reserved
        ProgramHeader = 0x6  // Location and size of Program Header for loading
    };


    enum class SegmentFlags : std::uint32_t
    {
        Executable = 0x0, // Segment is executable
        Writable   = 0x1, // Segment can be written to
        Readable   = 0x2  // Segment can be read from
    };


    /* Entry describing a loaded program segment. */
    struct __attribute__((packed)) ProgramHeaderEntry32
    {
        SegmentType  p_type;   // Segment type
        std::uint32_t     p_offset; // Segment offset in source file
        std::uint32_t     p_vaddr;  // Segment virtual address load location
        std::uint32_t     p_paddr;  // Segment physical address load location
        std::uint32_t     p_filesz; // Segment source size in bytes
        std::uint32_t     p_memsz;  // Segment load size in bytes
        SegmentFlags p_flags;  // Segment flags
        std::uint32_t     p_align;  // Segment load alignment
    };


    enum class SymbolBinding : std::uint8_t
    {
        Local  = 0,
        Global = 1,
        Weak   = 2
    };


    enum class SymbolType : std::uint8_t
    {
        None     = 0,
        Object   = 1,
        Function = 2,
        Section  = 3,
        File     = 4
    };


    struct __attribute__((packed)) SymbolTableEntry32
    {
        std::uint32_t st_name;
        std::uint32_t st_value;
        std::uint32_t st_size;

        SymbolType st_info_type : 4;
        SymbolBinding st_info_binding : 4;
        std::uint8_t st_other;
        std::uint16_t st_shndx; // Section header index (or special)
    };
    static_assert(sizeof(SymbolTableEntry32) == 16, "");


    struct __attribute__((packed)) SymbolTableEntry64
    {
        std::uint32_t st_name;
        SymbolType st_info_type : 4;
        SymbolBinding st_info_binding : 4;
        std::uint8_t st_other;
        std::uint16_t st_shndx; // Section header index (or special)
        std::uint64_t st_value;
        std::uint64_t st_size;
    };
    static_assert(sizeof(SymbolTableEntry64) == 24, "");
    using SymbolTableEntry = SymbolTableEntry64;


    bool decodeElfHeader(const std::uint8_t* data, std::size_t dataSize, Header& header_out);
    bool decodeSectionHeader(const std::uint8_t* data, std::size_t dataSize, ElfClass el_class,
                             SectionHeader& header_out);
    bool decodeSymbolTableEntry(const std::uint8_t* data, std::size_t dataSize, ElfClass el_class,
                                SymbolTableEntry& entry_out);


    template<typename T>
    struct default_decoder
    {
        static_assert(std::is_trivially_copyable<T>::value, "");

        inline constexpr void decode(const std::uint8_t* data, std::size_t index, T& value_out)
        {
            std::memcpy(&value_out, data + (sizeof(T) * index), sizeof(T));
        }
    };


    struct section_header_decoder
    {
        ElfClass el_class;
        std::size_t elementSize;

        inline section_header_decoder(ElfClass el_class) : el_class(el_class)
        {
            elementSize = el_class == ElfClass::class64 ?
                        sizeof(SectionHeader64) : sizeof(SectionHeader32);
        }
        section_header_decoder(const section_header_decoder&) = default;
        section_header_decoder(section_header_decoder&&) = default;

        inline void decode(const std::uint8_t* data, std::size_t index, SectionHeader& value_out)
        {
            decodeSectionHeader(data + (elementSize * index), elementSize, el_class, value_out);
        }
    };


    struct symbol_table_decoder
    {
        ElfClass el_class;
        std::size_t elementSize;

        inline symbol_table_decoder(ElfClass el_class) : el_class(el_class)
        {
            elementSize = el_class == ElfClass::class64 ?
                        sizeof(SymbolTableEntry64) : sizeof(SymbolTableEntry32);
        }
        symbol_table_decoder(const symbol_table_decoder&) = default;
        symbol_table_decoder(symbol_table_decoder&&) = default;

        inline void decode(const std::uint8_t* data, std::size_t index, SymbolTableEntry& value_out)
        {
            decodeSymbolTableEntry(data + (elementSize * index), elementSize, el_class, value_out);
        }
    };


    template<typename T, typename Decoder=default_decoder<T>>
    class table_iterator : Decoder
    {
    public:
        using difference_type = std::size_t;
        using value_type = T;
        using pointer = const T*;
        using reference = const T&;
        using iterator_category = std::input_iterator_tag;
        using postfix = int;

    private:
        const std::uint8_t* data{};
        std::size_t count{};
        T value{};
        std::size_t index{};

    public:
        constexpr table_iterator(const std::uint8_t* data, std::size_t count, Decoder decoder = Decoder()) noexcept
            : Decoder(decoder), data(data), count(count) { }

        table_iterator(const table_iterator&) = default;
        table_iterator(table_iterator&&) = default;
        table_iterator& operator =(const table_iterator&) = default;
        table_iterator& operator =(table_iterator&&) = default;

        constexpr reference operator*() const noexcept {
            return value;
        }
        constexpr pointer operator->() const noexcept {
            return &value;
        }
        constexpr bool operator!=(const table_iterator& other) const noexcept {
            return index != other.index || data != other.data;
        }
        constexpr bool operator==(const table_iterator& other) const noexcept {
            return index == other.index && data == other.data;
        }
        table_iterator& operator++() noexcept
        {
            // If within table, decode next entry
            if (++index < count)
            {
                Decoder::decode(data, index, value);
            }
            // Otherwise use null index & data pointer to indicate end
            else
            {
                index = 0;
                data = nullptr;
            }
            return *this;
        }
        table_iterator operator++(postfix) noexcept
        {
            table_iterator copy = *this;
            this->operator++();
            return copy;
        }
    };


    class iter_section_headers
    {
    private:
        const std::uint8_t* data;
        std::size_t header_count;
        ElfClass el_class;

    public:
        inline constexpr iter_section_headers(const std::uint8_t* data, Header header)
            : data(data + header.e_shoff), header_count(header.e_shnum), el_class(header.el_class()) { }

        inline constexpr iter_section_headers(const std::uint8_t* data, std::size_t header_count, ElfClass el_class)
            : data(data), header_count(header_count), el_class(el_class) { }

        iter_section_headers(const iter_section_headers&) = default;
        iter_section_headers(iter_section_headers&&) = default;
        iter_section_headers& operator =(const iter_section_headers&) = default;
        iter_section_headers& operator =(iter_section_headers&&) = default;

        inline auto begin()
        {
            return table_iterator<SectionHeader, section_header_decoder>(
                data, header_count, section_header_decoder(el_class));
        }

        inline auto end()
        {
            return table_iterator<SectionHeader, section_header_decoder>(
                nullptr, header_count, section_header_decoder(el_class));
        }
    };


    class iter_symbols
    {
    private:
        const std::uint8_t* data{};
        std::size_t entry_count{};
        ElfClass el_class{};

    public:
        inline constexpr iter_symbols(const std::uint8_t* data, SectionHeader section, ElfClass el_class)
            : iter_symbols(data + section.sh_offset,
                           section.sh_size / (el_class == ElfClass::class64 ?
                                              sizeof(SymbolTableEntry64) : sizeof(SymbolTableEntry32)),
                           el_class) { }

        inline constexpr iter_symbols(const std::uint8_t* data, std::size_t entry_count, ElfClass el_class)
            : data(data), entry_count(entry_count), el_class(el_class) { }

        iter_symbols(const iter_symbols&) = default;
        iter_symbols(iter_symbols&&) = default;
        iter_symbols& operator =(const iter_symbols&) = default;
        iter_symbols& operator =(iter_symbols&&) = default;

        inline auto begin()
        {
            return table_iterator<SymbolTableEntry, symbol_table_decoder>(
                data, entry_count, symbol_table_decoder(el_class));
        }

        inline auto end()
        {
            return table_iterator<SymbolTableEntry, symbol_table_decoder>(
                nullptr, entry_count, symbol_table_decoder(el_class));
        }
    };
}
