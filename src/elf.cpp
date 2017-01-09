/* elf.cpp - (c) James S Renwick 2015-2017 */
#include <cstring>
#include "elf.h"


namespace elf
{
    const char StringTable32::emptyName[1] = { '\0' };


    error_t parseElfHeader(const uint8_t* buffer, size_t bufferSize, 
        Header32& header_out) noexcept
    {
        // Check header size
        if (bufferSize < sizeof(elf::Header32)) return -1;
        
        // Copy out
        memcpy(&header_out, buffer, sizeof(elf::Header32));

        return sizeof(elf::Header32);
    }


    static error_t parseProgramHeaderEntry(const uint8_t* buffer, size_t bufferSize,
        ProgramHeaderEntry32& entry_out) noexcept
    {
        // Check header size
        if (bufferSize < sizeof(ProgramHeaderEntry32)) return -1;

        // Copy out
        memcpy(&entry_out, buffer, sizeof(ProgramHeaderEntry32));

        return sizeof(ProgramHeaderEntry32);
    }

    error_t parseProgramHeader(const uint8_t* buffer, size_t bufferSize,
        uint16_t entryCount, ProgramHeader32& header_out, bool copyData)
    {
        // Program header is optional, so handle non-present case
        if (entryCount == 0)
        {
            header_out.segmentCount = 0; 
            header_out.segments.reset();
            return 0;
        }

        // Check buffer size
        if (bufferSize < sizeof(ProgramHeaderEntry32)) return -1;

        // Use copy of source
        if (copyData)
        {
            auto table = new ProgramHeaderEntry32[entryCount];

            // Parse section table entries
            for (uint16_t i = 0; i < entryCount; i++)
            {
                auto res = parseProgramHeaderEntry(buffer, bufferSize, table[i]);
                if (res < 0) return res; else { buffer += res; bufferSize -= res; }
            }
            header_out.segments.reset(table, true);
        }

        // Use source data directly
        else
        {
            header_out.segments.reset(reinterpret_cast<
                const ProgramHeaderEntry32*>(buffer), false);
        }

        header_out.segmentCount = entryCount;
        return 0;
    }


    static error_t parseSectionHeader(const uint8_t* buffer, size_t bufferSize,
        SectionHeader32& header_out) noexcept
    {
        // Check header size
        if (bufferSize < sizeof(SectionHeader32)) return -1;

        // Copy out
        memcpy(&header_out, buffer, sizeof(SectionHeader32));

        return sizeof(SectionHeader32);
    }


    error_t parseSectionTable(const uint8_t* buffer, size_t bufferSize,
        uint16_t sectionCount, SectionTable32& table_out, bool copyData)
    {
        // Check buffer size
        if (bufferSize < sizeof(SectionHeader32) * sectionCount) return -1;

        // Use copy of source
        if (copyData)
        {
            // Allocate & fill table
            auto table = new SectionHeader32[sectionCount];

            // Parse section table entries
            for (decltype(sectionCount) i = 0; i < sectionCount; i++)
            {
                auto res = parseSectionHeader(buffer, bufferSize, table[i]);
                if (res < 0) return res; else { buffer += res; bufferSize -= res; }
            }
            table_out.headers.reset(table, true);
        }

        // Use source data directly
        else
        {
            table_out.headers.reset(reinterpret_cast<const SectionHeader32*>(buffer), false);
        }

        table_out.headerCount = sectionCount;
        return 0;
    }

    
    static error_t parseSymbolEntry(const uint8_t* buffer, size_t bufferSize, 
        SymbolTableEntry32& entry_out)
    {
        // Check header size
        if (bufferSize < sizeof(SymbolTableEntry32)) return -1;

        // Copy out
        memcpy(&entry_out, buffer, sizeof(SymbolTableEntry32));

        return sizeof(SymbolTableEntry32);
    }


    error_t parseSymbolTable(const uint8_t* buffer, size_t bufferSize, uint32_t entryCount, 
        SymbolTable32& table_out, bool copyData)
    {
        // Check buffer size
        if (bufferSize < sizeof(SymbolTableEntry32) * entryCount) return -1;

        // Use copy of source
        if (copyData)
        {
            // Allocate & fill table
            auto table = new SymbolTableEntry32[entryCount];

            // Parse section table entries
            for (decltype(entryCount) i = 0; i < entryCount; i++)
            {
                auto res = parseSymbolEntry(buffer, bufferSize, table[i]);
                if (res < 0) return res; else { buffer += res; bufferSize -= res; }
            }
            table_out.entries.reset(table, true);
        }

        // Use source data directly
        else
        {
            table_out.entries.reset(reinterpret_cast<const SymbolTableEntry32*>(buffer), false);
        }

        table_out.entryCount = entryCount;
        return 0;
    }
}
