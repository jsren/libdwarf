/* dwarf.h - (c) James S Renwick 2015-17 */
#pragma once
#include <cstddef>
#include <stdint.h>
#include <cstring>
#include <memory>
#include "const.h"
#include "../array.h"

namespace dwarf
{

    int32_t uleb_read(const uint8_t data[], uint32_t &value_out);
    int32_t uleb_read(const uint8_t data[], uint64_t &value_out);

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


    SectionType SectionTypeFromString(const char* str);


    struct DwarfSection32
    {
        SectionType type = SectionType::invalid;
		elf::Pointer<uint8_t[]> data{};
		uint32_t size{};

    public:
        DwarfSection32() = default;

        inline DwarfSection32(SectionType type, elf::Pointer<uint8_t[]> data, uint32_t size)
            : type(type), data(std::move(data)), size(size) { }

        
        inline explicit DwarfSection32(const DwarfSection32& other) : type(other.type), size(other.size)
        {
            if (other.data.ownsData()) {
                data.reset(new uint8_t[size], true); memcpy(this->data.get(), other.data.get(), size);
            }
            else data.reset(other.data.get(), false);
        }
        inline DwarfSection32& operator=(const DwarfSection32& other)
        {
            if (this == &other) return *this;
            type = other.type; size = other.size;
            data.reset(new uint8_t[size], true); 
            memcpy(this->data.get(), other.data.get(), size);
            return *this;
        }
    };


    struct DwarfSection64
    {
        SectionType type = SectionType::invalid;
        elf::Pointer<uint8_t[]> data{};
        uint64_t size{};

    public:
        DwarfSection64()  = default;

        inline DwarfSection64(SectionType type, elf::Pointer<uint8_t[]> data, uint64_t size) noexcept
            : type(type), data(std::move(data)), size(size) { }

        inline explicit DwarfSection64(const DwarfSection64& other) : type(other.type), size(other.size)
        {
            if (other.data.ownsData()) {
                data.reset(new uint8_t[size], true); memcpy(this->data.get(), other.data.get(), size);
            }
            else data.reset(other.data.get(), false);
        }

        inline DwarfSection64& operator=(const DwarfSection64& other)
        {
            if (this == &other) return *this;
            type = other.type; size = other.size;
            data.reset(new uint8_t[size], true);
            memcpy(this->data.get(), other.data.get(), size);
            return *this;
        }
    };


    struct DwarfContext32
    {
        const Array<DwarfSection32> sections{0};

    public:
        inline explicit DwarfContext32(Array<DwarfSection32>&& sections)
            : sections(std::move(sections)) { }
    };

    struct DwarfContext64
    {
        const Array<DwarfSection64> sections{0};

    public:
        inline explicit DwarfContext64(Array<DwarfSection64>&& sections)
            : sections(std::move(sections)) { }
    };

}
