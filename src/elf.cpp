/* elf.cpp - (c) James S Renwick 2015 */
#include <cstring>
#include "elf.h"

namespace elf
{
    const char emptyName[1] = { '\0' };

    error_t parseElfHeader(const uint8_t* buffer, size_t bufferSize, 
        Header32& header_out) noexcept
    {
        // Check header size
        if (bufferSize < sizeof(elf::Header32)) return -1;
        
        // Copy out
        memcpy(&header_out, buffer, sizeof(elf::Header32));

        return sizeof(elf::Header32);
    }

    error_t parseSectionHeader(const uint8_t* buffer, size_t bufferSize,
        SectionHeader32& header_out) noexcept
    {
        // Check header size
        if (bufferSize < sizeof(elf::SectionHeader32)) return -1;

        // Copy out
        memcpy(&header_out, buffer, sizeof(elf::SectionHeader32));

        return sizeof(elf::SectionHeader32);
    }

    error_t parseSectionTable(const uint8_t* buffer, size_t bufferSize,
        decltype(Header32::e_shnum) sectionCount, SectionHeader32*& table_out)
    {
        // Check buffer size
        if (bufferSize < sizeof(elf::SectionHeader32) * sectionCount) return -1;

        // Allocate & fill table
        table_out = new SectionHeader32[sectionCount];

        // Parse section table entries
        for (decltype(sectionCount) i = 0; i < sectionCount; i++) 
        {
            auto res = parseSectionHeader(buffer, bufferSize, table_out[i]);
            if (res < 0) return res; else { buffer += res; bufferSize -= res; }
        }
        return sectionCount;
    }
}
