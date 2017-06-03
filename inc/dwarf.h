/* dwarf.h - (c) James S Renwick 2015-17 */
#pragma once
#include <cstddef>
#include <stdint.h>
#include <cstring>
#include <memory>
#include <unordered_map>
#include "const.h"
#include "../array.h"
#include "format.h"

namespace dwarf
{
    typedef signed long int error_t;

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

        explicit DwarfSection32(const DwarfSection32& other);
        DwarfSection32& operator=(const DwarfSection32& other);

        inline operator bool() const { 
            return type != SectionType::invalid;
        }
    };


    struct DwarfSection64
    {
        SectionType type = SectionType::invalid;
        elf::Pointer<uint8_t[]> data{};
        uint64_t size{};

    public:
        DwarfSection64() = default;

        inline DwarfSection64(SectionType type, elf::Pointer<uint8_t[]> data, uint64_t size) noexcept
            : type(type), data(std::move(data)), size(size) { }

        explicit DwarfSection64(const DwarfSection64& other);
        DwarfSection64& operator=(const DwarfSection64& other);

        inline operator bool() const {
            return type != SectionType::invalid;
        }
    };


    struct DwarfContext32
    {
        static constexpr const uint8_t bits = 32;

        const Array<DwarfSection32> sections{0};

        dwarf4::CompilationUnitHeader32 header;

        using EntryIndex = std::tuple<dwarf4::DIEType, uint64_t, const char*, std::size_t>;
        std::unordered_map<uint64_t, std::size_t> abbreviationIndex{};
        std::vector<EntryIndex> entryIDIndex{};

    public:
        inline explicit DwarfContext32(Array<DwarfSection32>&& sections)
            : sections(std::move(sections))
        {
            for (auto& section : this->sections) if (section.type == SectionType::debug_info) {
                memcpy(&header, section.data.get(), sizeof(header)); break;
            }
        }

    public:
        error_t buildIndexes();
        const DwarfSection32& operator[](SectionType type) const;
    };


    struct DwarfContext64
    {
        static constexpr const uint8_t bits = 64;

        const Array<DwarfSection64> sections{0};
        dwarf4::CompilationUnitHeader32 header;

        using EntryIndex = std::tuple<dwarf4::DIEType, uint64_t, const char*, std::size_t>;
        std::unordered_map<uint64_t, std::size_t> abbreviationIndex{};
        std::vector<EntryIndex> entryIDIndex{};

    public:
        inline explicit DwarfContext64(Array<DwarfSection64>&& sections)
            : sections(std::move(sections))
        { 
            for (auto& section : this->sections) if (section.type == SectionType::debug_info) {
                memcpy(&header, section.data.get(), sizeof(header)); break;
            }
        }

    public:
        error_t buildIndexes();
        const DwarfSection64& operator[](SectionType type) const;
    };

}
