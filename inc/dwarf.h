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






    struct DieIndexEntry
    {
        uint64_t id;
        DIEType type;
        uint64_t parentId;
        const char* name;
    };

    template<typename Iter>
    struct DieIndexIterator
    {
        Iter iter{};
        uint64_t index{};

        DieIndexIterator() = default;
        inline DieIndexIterator(const Iter& iter) : iter(iter) { }

    public:
        inline bool operator !=(const DieIndexIterator& other) {
            return iter != other.iter;
        }
        inline bool operator ==(const DieIndexIterator& other) {
            return iter == other.iter;
        }

        inline auto& operator++() {
            iter++; index++; return *this;
        }
        inline DieIndexEntry operator*() {
            return { index, std::get<0>(*iter), std::get<1>(*iter), std::get<2>(*iter) };
        }
    };

    template<typename BeginIter, typename EndIter>
    struct DieIndexIteratorProxy
    {
        friend class DwarfContext32;
        friend class DwarfContext64;

    private:
        BeginIter _begin;
        EndIter _end;

    public:
        inline DieIndexIterator<BeginIter> begin() {
            return _begin;
        }
        inline DieIndexIterator<EndIter> end() {
            return _end;
        }
    };



    class DwarfContext32
    {
        friend class DebugEntryParser;

    public:
        static constexpr const uint8_t bits = 32;

    private:
        CompilationUnitHeader32 header{};

        using EntryIndex = std::tuple<DIEType, uint64_t, const char*, std::size_t>;
        std::unordered_map<uint64_t, std::size_t> abbreviationIndex{};
        std::vector<EntryIndex> entryIndex{};

    public:
        const Array<DwarfSection32> sections{0};

        DieIndexIteratorProxy<decltype(entryIndex.cbegin()),
            decltype(entryIndex.cend())> dieIndex{};

    public:
        inline explicit DwarfContext32(Array<DwarfSection32>&& sections) : 
            sections(std::move(sections))
        {
            for (auto& section : this->sections) if (section.type == SectionType::debug_info) {
                memcpy(&header, section.data.get(), sizeof(header)); break;
            }
        }

    public:

        error_t buildIndexes();

        DebugInfoEntry dieFromId(uint64_t id);

        const DwarfSection32& operator[](SectionType type) const;


        inline const auto& unitHeader() const {
            return header;
        }
    };


    class DwarfContext64
    {
        friend class DebugEntryParser;

    public:
        static constexpr const uint8_t bits = 64;

    private:
        CompilationUnitHeader32 header;

        using EntryIndex = std::tuple<DIEType, uint64_t, const char*, std::size_t>;
        std::unordered_map<uint64_t, std::size_t> abbreviationIndex{};
        std::vector<EntryIndex> entryIndex{};


    public:
        const Array<DwarfSection64> sections{0};

        DieIndexIteratorProxy<decltype(entryIndex.cbegin()),
            decltype(entryIndex.cend())> dieIndex{};

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

        DebugInfoEntry dieFromId(uint64_t id);

        const DwarfSection64& operator[](SectionType type) const;


        inline const auto& dieByID(uint64_t dieID) const {
            return entryIndex[dieID];
        }

        inline const auto& unitHeader() const {
            return header;
        }
    };

}
