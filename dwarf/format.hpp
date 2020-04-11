/* format.h - (c) James S Renwick 2015, 2020 */
/*
   Each debugging information entry begins with an unsigned LEB128 number
   containing the abbreviation code for the entry. This code represents an
   entry within the abbreviations table associated with the compilation unit
   containing this entry. The abbreviation code is followed by a series of
   attribute values.
*/

#pragma once
#include <cstdint>
#include <malloc.h>
#include <memory>
#include <type_traits>
#include "const.hpp"

namespace dwarf
{

    // .debug_info section header
    struct __attribute__((packed)) CompilationUnitHeader32
    {
        std::uint32_t unitLength;
        std::uint16_t version;
        std::uint32_t debugAbbrevOffset;
        std::uint8_t  addressSize;
    };

	// .debug_info section header
	struct CompilationUnitHeader64
	{
		std::uint32_t : 32;
		std::uint64_t unitLength;
		std::uint16_t version;
		std::uint64_t debugAbbrevOffset;
		std::uint8_t  addressSize;
	};

	struct CompilationUnitHeader
	{
		virtual std::uint64_t unitLength() const = 0;
		virtual std::uint16_t version() const = 0;
		virtual std::uint64_t debugAbbrevOffset() const = 0;
		virtual std::uint8_t addressSize() const = 0;
	};

	namespace _detail
	{
		template<typename HeaderType>
		struct CompilationUnitHeaderImpl : public CompilationUnitHeader
		{
		private:
			HeaderType header;

		public:
			inline explicit CompilationUnitHeaderImpl(HeaderType header)
				: header(header) { };

			inline std::uint64_t unitLength() const override final {
				return header.unitLength;
			};
			inline std::uint16_t version() const override final {
				return header.version;
			}
			inline std::uint64_t debugAbbrevOffset() const override final {
				return header.debugAbbrevOffset;
			}
			inline std::uint8_t addressSize() const override final {
				return header.addressSize;
			}
		};
		using CompilationUnitHeader32 =
			CompilationUnitHeaderImpl<dwarf::CompilationUnitHeader32>;
		using CompilationUnitHeader64 =
			CompilationUnitHeaderImpl<dwarf::CompilationUnitHeader64>;
	}


    // .debug_types section header
    struct __attribute__((packed)) TypeUnitHeader32
    {
        std::uint32_t unitLength;
        std::uint16_t version;
        std::uint32_t debugAbbrevOffset;
        std::uint8_t  addressSize;
        std::uint64_t typeSignature;
        std::uint32_t typeOffset;
    };


#pragma region .DEBUG_PUBNAMES/.DEBUG_PUBTYPES

    struct __attribute__((packed)) NameTableHeader32
    {
    public:
        std::uint32_t unitLength;      // Length of the set of entries for this compilation unit, not including the
                                  // length field itself
        std::uint16_t version;         // Version identifier containing the value 2
        std::uint32_t debugInfoOffset; // Offset into the .debug_info section of the compilation unit header
        std::uint32_t debugInfoLength; // Length containing the size in bytes of the contents of the .debug_info section
                                  // generated to represent this compilation unit
    };

    struct __attribute__((packed)) NameTableHeader64
    {
        unsigned : 32;            // Should be 0xFFFFFFFF
        std::uint64_t unitLength;      // Length of the set of entries for this compilation unit, not including the
                                  // length field itself
        std::uint16_t version;         // Version identifier containing the value 2
        std::uint64_t debugInfoOffset; // Offset into the .debug_info section of the compilation unit header
        std::uint64_t debugInfoLength; // Length containing the size in bytes of the contents of the .debug_info section
                                  // generated to represent this compilation unit
    };

#pragma endregion


#pragma region .DEBUG_ARRANGES
    struct __attribute__((packed)) AddressRangeTableHeader32
    {
        std::uint32_t unitLength;      // Length of the set of entries for this compilation unit, not including the
                                  // length field itself
        std::uint16_t version;         // Version identifier containing the value 2
        std::uint32_t debugInfoOffset; // Offset into the .debug_info section of the compilation unit header
        std::uint8_t  addressSize;     // The size in bytes of an address (or the offset portion for segmented) on the target system
        std::uint8_t  segmentSize;     // The size in bytes of a segment selector on the target system
    };


    struct __attribute__((packed)) AddressRangeTableHeader64
    {
        unsigned : 32;            // Should be 0xFFFFFFFF
        std::uint64_t unitLength;      // Length of the set of entries for this compilation unit, not including the
                                  // length field itself
        std::uint16_t version;         // Version identifier containing the value 2
        std::uint64_t debugInfoOffset; // Offset into the .debug_info section of the compilation unit header
        std::uint8_t  addressSize;     // The size in bytes of an address (or the offset portion for segmented) on the target system
        std::uint8_t  segmentSize;     // The size in bytes of a segment selector on the target system
    };

#pragma endregion


    struct CIEntry32
    {
        std::uint32_t length;
        std::uint32_t cieID;
        std::uint8_t  version;

        // ... further fields ...
    };

    struct FDEntry32
    {
        std::uint32_t length;
        std::uint32_t ptrCIEntry;

    };


    struct AttributeSpecification
    {
        AttributeName name;
        AttributeForm form;
        AttributeClass class_;

    public:
        static std::uint32_t parse(const std::uint8_t* buffer, std::size_t length, AttributeSpecification& attr_out);

    };


    class DebugInfoAbbreviation
    {
    public:
		std::uint64_t id{};
		DIEType tag{};

        std::size_t attributeCount{};
        const void* attributeStart{};

        std::size_t childCount{};
        std::unique_ptr<std::uint64_t[]> childIDs{};

    public:

        DebugInfoAbbreviation() = default;

        DebugInfoAbbreviation(std::uint64_t id, DIEType tag, std::size_t attributeCount,
            const void* attributeStart, std::size_t childCount, std::unique_ptr<std::uint64_t[]> childIDs)
			: id(id), tag(tag), attributeCount(attributeCount), attributeStart(attributeStart),
            childCount(childCount), childIDs(std::move(childIDs)) { }

        DebugInfoAbbreviation(DebugInfoAbbreviation&&) = default;
        DebugInfoAbbreviation& operator =(DebugInfoAbbreviation&&) = default;

    public:
		static std::uint32_t parse(const std::uint8_t* buffer, std::size_t length, DebugInfoAbbreviation& entry_out);
	};


    class Attribute : public AttributeSpecification
    {
    public:
        const std::uint8_t* data{};
        std::size_t size{};

    public:
        Attribute() = default;
        inline Attribute(const AttributeSpecification& def, const std::uint8_t* data, std::size_t size)
            : AttributeSpecification(def), data(data), size(size) { }

    public:
        template<class T, class=std::enable_if_t<std::is_trivially_copyable<T>::value>>
        inline T valueAs() {
            T value; std::memcpy(&value, this->valueData.get(), sizeof(T));
            return value;
        }
    };



    struct DebugInfoEntry
    {
        std::uint64_t id;
        std::uint64_t abbreviationId;
        DIEType type;
        std::size_t attributeCount;
        std::unique_ptr<Attribute[]> attributes;
    };



}
