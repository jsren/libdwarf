/* dwarf.h - (c) James S Renwick 2015-16 */
#pragma once
#include <cstddef>
#include <stdint.h>
#include <cstring>
#include "const.h"

namespace dwarf
{
    using dwarfArch = bool;
    constexpr dwarfArch dwarf32 = true;
    constexpr dwarfArch dwarf64 = false;

    enum class SectionType : uint8_t
    {
        invalid,
        debug_info,
        debug_abbrev,
        debug_aranges,
        debug_ranges,
        debug_line,
        debug_str
    };


    template<dwarfArch dwarf32=dwarf32>
    struct DwarfSection
    {
        SectionType type;
        uint8_t* data;
        uint32_t size;

        DwarfSection() noexcept = default;

        explicit DwarfSection(SectionType type, const uint8_t* data, uint32_t size) noexcept
            : type(type), data(nullptr), size(size)
        {
            this->data = new uint8_t[size];
            memcpy(this->data, data, size);
        }

        inline ~DwarfSection() { if (data != nullptr) delete[] data; }
    };
    template<>
    struct DwarfSection<dwarf64>
    {
        SectionType type;
        uint8_t* data;
        uint64_t size;

        DwarfSection() noexcept = default;

        explicit DwarfSection(SectionType type, const uint8_t* data, uint64_t size) noexcept 
            : type(type), data(nullptr), size(size) 
        {
            this->data = new uint8_t[size];
            memcpy(this->data, data, size);
        }

        inline ~DwarfSection() { if (data != nullptr) delete[] data; }
    };


    template<dwarfArch dwarf32=dwarf32>
    struct DwarfContext
    {
        const DwarfSection<>* sections;

        explicit DwarfContext(const DwarfSection<> sections[6]) noexcept
            : sections(sections) { }
    };
    template<>
    struct DwarfContext<dwarf64>
    {
        const DwarfSection<dwarf64>* sections;
        
        explicit DwarfContext(const DwarfSection<dwarf64> sections[6], size_t sectionCount) noexcept
            : sections(sections) { }
    };

}
