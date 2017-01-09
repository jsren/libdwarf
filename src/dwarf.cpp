#include "dwarf.h"
#include "format.h"

namespace dwarf
{

    SectionType SectionTypeFromString(const char* name)
    {
        if (strcmp(name, ".debug_info") == 0) {
            return dwarf::SectionType::debug_info;
        }
        else if (strcmp(name, ".debug_abbrev") == 0) {
            return dwarf::SectionType::debug_abbrev;
        }
        else if (strcmp(name, ".debug_aranges") == 0) {
            return dwarf::SectionType::debug_aranges;
        }
        else if (strcmp(name, ".debug_ranges") == 0) {
            return dwarf::SectionType::debug_ranges;
        }
        else if (strcmp(name, ".debug_line") == 0) {
            return dwarf::SectionType::debug_line;
        }
        else if (strcmp(name, ".debug_str") == 0) {
            return dwarf::SectionType::debug_str;
        }
        else return SectionType::invalid;
    }


    void loadAbbreviationTable(DwarfContext32& context, uint8_t *abbv_table) noexcept
    {
        
    }


    void indexAbbreviationTable(uint8_t *abbrv_table) noexcept
    {
        // Pass 1 - count the number of entries

    }

    int32_t uleb_read(const uint8_t data[], /*out*/ uint32_t &value)
    {
        // Perform manual unrolling
        value = data[0];
        if ((data[0] & 0b10000000) == 0) return 1;
        else value &= 0b01111111;

        value |= ((uint32_t)data[1] << 7);
        if ((data[1] & 0b10000000) == 0) return 2;
        else value &= ((0b01111111 << 7) | 0xFF);

        value |= ((uint32_t)data[2] << 14);
        if ((data[2] & 0b10000000) == 0) return 3;
        else value &= ((0b01111111 << 14) | 0xFFFF);

        value |= ((uint32_t)data[3] << 21);
        if ((data[3] & 0b10000000) == 0) return 4;
        else value &= ((0b01111111 << 21) | 0xFFFFFF);

        value |= ((uint32_t)data[4] << 28);
        if ((data[4] & 0b10000000) == 0) return 5;

        // Consume any extra bytes
        for (int i = sizeof(value) + 1; true; i++) {
            if ((data[i] & 0b10000000) == 0) return i + 1;
        }
        return 0; // This should never happen...
    }

    int32_t uleb_read(const uint8_t data[], /*out*/ uint64_t &value)
    {
        int32_t i = 0;
        value = 0; // Zero 
        
        for (uint8_t shift = 0; i <= (uint8_t)sizeof(value); shift += 7, i++)
        {
            value |= ((uint64_t)data[i] << shift);
            if ((data[i] & 0b10000000) == 0) return i + 1;
            else value &= ~((uint64_t)(0b10000000 << shift));
        }

        // Consume any extra bytes
        while (data[i++] & 0b10000000) { }
        return i;
    }
}
