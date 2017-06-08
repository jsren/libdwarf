/* dwarf.h - (c) James S Renwick 2015-17 */
#pragma once
#include <cstddef>
#include <stdint.h>
#include <string.h>
#include <memory>
#include <unordered_map>
#include "const.h"
#include "../array.h"
#include "format.h"

namespace dwarf
{
    typedef signed long int error_t;


	/* Reads an unsigned LEB value from the given buffer of the specified length. 
	   Returns the number of bytes read. */
	uint32_t uleb_read(const uint8_t data[], std::size_t length, uint32_t &value_out);
	/* Reads an unsigned LEB value from the given buffer of the specified length.
	   Returns the number of bytes read. */
    uint32_t uleb_read(const uint8_t data[], std::size_t length, uint64_t &value_out);

	/* Reads a signed LEB value from the given buffer of the specified length.
	   Returns the number of bytes read. */
    uint32_t sleb_read(const uint8_t data[], std::size_t length, int32_t &value_out);
	/* Reads a signed LEB value from the given buffer of the specified length.
	   Returns the number of bytes read. */
    uint32_t sleb_read(const uint8_t data[], std::size_t length, int64_t &value_out);



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



    struct DwarfSection
    {
        SectionType type = SectionType::invalid;
        elf::Pointer<uint8_t[]> data{};
        uint64_t size{};

    public:
        DwarfSection() = default;

        inline DwarfSection(SectionType type, elf::Pointer<uint8_t[]> data, uint64_t size) noexcept
            : type(type), data(std::move(data)), size(size) { }

        explicit DwarfSection(const DwarfSection& other);
        DwarfSection& operator=(const DwarfSection& other);

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
        friend class DwarfContext;
        friend class DwarfContext;

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


	enum class DwarfWidth
	{
		Bits32,
		Bits64
	};



    class DwarfContext
    {
        friend class DebugEntryParser;


    private:
        std::unique_ptr<CompilationUnitHeader> header{};

        using EntryIndex = std::tuple<DIEType, uint64_t, const char*, std::size_t>;
        std::unordered_map<uint64_t, std::size_t> abbreviationIndex{};
        std::vector<EntryIndex> entryIndex{};

    public:
        const Array<DwarfSection> sections{0};

		const DwarfWidth width{};

        DieIndexIteratorProxy<decltype(entryIndex.cbegin()),
            decltype(entryIndex.cend())> dieIndex{};

    public:
		explicit DwarfContext(Array<DwarfSection>&& sections, DwarfWidth width);

    public:

        error_t buildIndexes();

        DebugInfoEntry dieFromId(uint64_t id);

        const DwarfSection& operator[](SectionType type) const;


        inline const auto& unitHeader() const {
            return *header.get();
        }
    };

}
