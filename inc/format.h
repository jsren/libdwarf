/* format.h - (c) James S Renwick 2015 */
/*
   Each debugging information entry begins with an unsigned LEB128 number 
   containing the abbreviation code for the entry. This code represents an 
   entry within the abbreviations table associated with the compilation unit 
   containing this entry. The abbreviation code is followed by a series of 
   attribute values.
*/

#pragma once
#include <stdint.h>
#include <malloc.h>
#include <memory>
#include <type_traits>

#include "const.h"
#include "platform.h"
#include "glue.h"

namespace dwarf
{

    // .debug_info section header
    _pack_start
        struct CompilationUnitHeader32
    {
        uint32_t unitLength;
        uint16_t version;
        uint32_t debugAbbrevOffset;
        uint8_t  addressSize;
    } _pack_end;

    // .debug_types section header
    _pack_start struct TypeUnitHeader32
    {
        uint32_t unitLength;
        uint16_t version;
        uint32_t debugAbbrevOffset;
        uint8_t  addressSize;
        uint64_t typeSignature;
        uint32_t typeOffset;
    } _pack_end;


#pragma region .DEBUG_PUBNAMES/.DEBUG_PUBTYPES

    _pack_start
        struct NameTableHeader32
    {
    public:
        uint32_t unitLength;      // Length of the set of entries for this compilation unit, not including the 
                                  // length field itself
        uint16_t version;         // Version identifier containing the value 2
        uint32_t debugInfoOffset; // Offset into the .debug_info section of the compilation unit header
        uint32_t debugInfoLength; // Length containing the size in bytes of the contents of the .debug_info section
                                  // generated to represent this compilation unit
    } _pack_end;

    _pack_start
        struct NameTableHeader64
    {
        unsigned : 32;            // Should be 0xFFFFFFFF
        uint64_t unitLength;      // Length of the set of entries for this compilation unit, not including the 
                                  // length field itself
        uint16_t version;         // Version identifier containing the value 2
        uint64_t debugInfoOffset; // Offset into the .debug_info section of the compilation unit header
        uint64_t debugInfoLength; // Length containing the size in bytes of the contents of the .debug_info section
                                  // generated to represent this compilation unit
    } _pack_end;

#pragma endregion


#pragma region .DEBUG_ARRANGES
    _pack_start
        struct AddressRangeTableHeader32
    {
        uint32_t unitLength;      // Length of the set of entries for this compilation unit, not including the 
                                  // length field itself
        uint16_t version;         // Version identifier containing the value 2
        uint32_t debugInfoOffset; // Offset into the .debug_info section of the compilation unit header
        uint8_t  addressSize;     // The size in bytes of an address (or the offset portion for segmented) on the target system
        uint8_t  segmentSize;     // The size in bytes of a segment selector on the target system
    } _pack_end;


    _pack_start
        struct AddressRangeTableHeader64
    {
        unsigned : 32;            // Should be 0xFFFFFFFF
        uint64_t unitLength;      // Length of the set of entries for this compilation unit, not including the 
                                  // length field itself
        uint16_t version;         // Version identifier containing the value 2
        uint64_t debugInfoOffset; // Offset into the .debug_info section of the compilation unit header
        uint8_t  addressSize;     // The size in bytes of an address (or the offset portion for segmented) on the target system
        uint8_t  segmentSize;     // The size in bytes of a segment selector on the target system
    } _pack_end;

#pragma endregion


    struct CIEntry32
    {
        uint32_t length;
        uint32_t cieID;
        uint8_t  version;

        // ... further fields ...
    };

    struct FDEntry32
    {
        uint32_t length;
        uint32_t ptrCIEntry;

    };


    struct AttributeSpecification
    {
        AttributeName name;
        AttributeForm form;
        AttributeClass class_;

    public:
        static uint32_t parse(const uint8_t* buffer, std::size_t length, AttributeSpecification& attr_out);

    };


    class DebugInfoAbbreviation
    {
    public:
		uint64_t id{};
		DIEType tag{};

        size_t attributeCount{};
        const void* attributeStart{};

        size_t childCount{};
        std::unique_ptr<uint64_t[]> childIDs{};

    public:

        DebugInfoAbbreviation() = default;

        DebugInfoAbbreviation(uint64_t id, DIEType tag, size_t attributeCount, 
            const void* attributeStart, size_t childCount, std::unique_ptr<uint64_t[]> childIDs)
			: id(id), tag(tag), attributeCount(attributeCount), attributeStart(attributeStart), 
            childCount(childCount), childIDs(std::move(childIDs)) { }

        DebugInfoAbbreviation(DebugInfoAbbreviation&&) = default;
        DebugInfoAbbreviation& operator =(DebugInfoAbbreviation&&) = default;
        
    public:
		static uint32_t parse(const uint8_t* buffer, std::size_t length, DebugInfoAbbreviation& entry_out);
	};


    class Attribute : public AttributeSpecification
    {
    public:
        const uint8_t* data{};
        std::size_t size{};

    public:
        Attribute() = default;
        inline Attribute(const AttributeSpecification& def, const uint8_t* data, std::size_t size)
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
        uint64_t id;
        uint64_t abbreviationId;
        DIEType type;
        size_t attributeCount;
        std::unique_ptr<Attribute[]> attributes;
    };



}
