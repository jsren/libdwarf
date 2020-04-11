/* dwarf.h - (c) James S Renwick 2015-17 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <string.h>
#include <memory>
#include <array>
#include <vector>
#include <unordered_map>
#include "const.hpp"
#include "format.hpp"

namespace dwarf
{
    typedef signed long int error_t;


	/* Reads an unsigned LEB value from the given buffer of the specified length.
	   Returns the number of bytes read. */
	std::uint32_t uleb_read(const std::uint8_t data[], std::size_t length, std::uint32_t &value_out);
	/* Reads an unsigned LEB value from the given buffer of the specified length.
	   Returns the number of bytes read. */
    std::uint32_t uleb_read(const std::uint8_t data[], std::size_t length, std::uint64_t &value_out);

	/* Reads a signed LEB value from the given buffer of the specified length.
	   Returns the number of bytes read. */
    std::uint32_t sleb_read(const std::uint8_t data[], std::size_t length, std::int32_t &value_out);
	/* Reads a signed LEB value from the given buffer of the specified length.
	   Returns the number of bytes read. */
    std::uint32_t sleb_read(const std::uint8_t data[], std::size_t length, std::int64_t &value_out);



    enum class SectionType : std::uint8_t
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
        std::unique_ptr<std::uint8_t[]> data{};
        std::uint64_t size{};

    public:
        DwarfSection() = default;

        inline DwarfSection(SectionType type, std::unique_ptr<std::uint8_t[]> data, std::uint64_t size) noexcept
            : type(type), data(std::move(data)), size(size) { }

        explicit DwarfSection(const DwarfSection& other);
        DwarfSection& operator=(const DwarfSection& other);

        inline operator bool() const {
            return type != SectionType::invalid;
        }
    };



    struct DieIndexEntry
    {
        std::uint64_t id;
        DIEType type;
        std::uint64_t parentId;
        const char* name;
    };

    template<typename Iter>
    struct DieIndexIterator
    {
        Iter iter{};
        std::uint64_t index{};

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

        using EntryIndex = std::tuple<DIEType, std::uint64_t, const char*, std::size_t>;
        std::unordered_map<std::uint64_t, std::size_t> abbreviationIndex{};
        std::vector<EntryIndex> entryIndex{};

    public:
        const std::vector<DwarfSection> sections{0};

		const DwarfWidth width{};

        DieIndexIteratorProxy<decltype(entryIndex.cbegin()),
            decltype(entryIndex.cend())> dieIndex{};

    public:
		explicit DwarfContext(std::vector<DwarfSection>&& sections, DwarfWidth width);

    public:

        error_t buildIndexes();

        DebugInfoEntry dieFromId(std::uint64_t id);

        const DwarfSection& operator[](SectionType type) const;


        inline const auto& unitHeader() const {
            return *header.get();
        }
    };

}
