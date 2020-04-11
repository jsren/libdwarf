/* elf.cpp - (c) James S Renwick 2015-2017 */
#include <cstring>
#include "elf.hpp"


namespace elf
{
    template<typename T>
    static bool _decode_common(const std::uint8_t* data, std::size_t dataSize, T& value_out)
    {
        static_assert(std::is_trivially_copyable<T>::value, "");
        if (dataSize < sizeof(T)) return false;

        std::memcpy(&value_out, data, sizeof(T));
        return true;
    }


    bool decodeElfHeader(const std::uint8_t* data, std::size_t dataSize, Header& header_out)
    {
        // Check magic
        if (data[0] != '\x7f' && data[1] != 'E' && data[2] != 'L' && data[3] != 'F') return false;

        // If 64-bit
        if (static_cast<ElfClass>(data[4]) == ElfClass::class64) {
            return _decode_common(data, dataSize, header_out);
        }
        // If 32-bit
        else if (static_cast<ElfClass>(data[4]) == ElfClass::class32)
        {
            Header32 h32;
            if (!_decode_common(data, dataSize, h32)) return false;
            std::memcpy(&header_out, &h32, sizeof(h32.e_ident));
            header_out.e_type      = h32.e_type;
            header_out.e_machine   = h32.e_machine;
            header_out.e_version   = h32.e_version;
            header_out.e_entry     = h32.e_entry;
            header_out.e_phoff     = h32.e_phoff;
            header_out.e_shoff     = h32.e_shoff;
            header_out.e_flags     = h32.e_flags;
            header_out.e_ehsize    = h32.e_ehsize;
            header_out.e_phentsize = h32.e_phentsize;
            header_out.e_phnum     = h32.e_phnum;
            header_out.e_shentsize = h32.e_shentsize;
            header_out.e_shnum     = h32.e_shnum;
            header_out.e_shstrndx  = h32.e_shstrndx;
            return true;
        }
        return false;
    }

    bool decodeSectionHeader(const std::uint8_t* data, std::size_t dataSize, ElfClass el_class,
                             SectionHeader& header_out)
    {
        // If 64-bit
        if (el_class == ElfClass::class64) {
            return _decode_common(data, dataSize, header_out);
        }
        else if (el_class == ElfClass::class32)
        {
            SectionHeader32 h32;
            if (!_decode_common(data, dataSize, h32)) return false;
            header_out.sh_name = h32.sh_name;
            header_out.sh_type = h32.sh_type;
            header_out.sh_flags = h32.sh_flags;
            header_out.sh_addr = h32.sh_addr;
            header_out.sh_offset = h32.sh_offset;
            header_out.sh_size = h32.sh_size;
            header_out.sh_link = h32.sh_link;
            header_out.sh_info = h32.sh_info;
            header_out.sh_addralign = h32.sh_addralign;
            header_out.sh_entsize = h32.sh_entsize;
            return true;
        }
        return false;
    }

    bool decodeSymbolTableEntry(const std::uint8_t* data, std::size_t dataSize, ElfClass el_class,
                                SymbolTableEntry& entry_out)
    {
        // If 64-bit
        if (el_class == ElfClass::class64) {
            return _decode_common(data, dataSize, entry_out);
        }
        else if (el_class == ElfClass::class32)
        {
            SymbolTableEntry32 e32;
            if (!_decode_common(data, dataSize, e32)) return false;
            entry_out.st_name = e32.st_name;
            entry_out.st_info_type = e32.st_info_type;
            entry_out.st_info_binding = e32.st_info_binding;
            entry_out.st_other = e32.st_other;
            entry_out.st_shndx = e32.st_shndx;
            entry_out.st_value = e32.st_value;
            entry_out.st_size = e32.st_size;
        }
        return false;
    }
}
