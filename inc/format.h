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
#include "platform.h"

#include "const.h"
#include "platform.h"
#include "glue.h"

namespace dwarf
{
    int32_t uleb_read(uint8_t data[], /*out*/ uint32_t &value);
    int32_t uleb_read(uint8_t data[], /*out*/ uint64_t &value);

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

    struct AttributeDefinition
    {
        AttributeName name;
        AttributeForm form;
        AttributeClass class_;
    };


    class DIEDefinition
    {
    public:
        uint64_t id;
        DIEType tag;

        size_t attributeCount;
        AttributeDefinition *attributes;

        size_t childCount;
        DIEDefinition *children;

    public:

        DIEDefinition() { }
        DIEDefinition(uint64_t id, DIEType tag, size_t childCount,
            DIEDefinition children[]) : id(id), tag(tag), childCount(childCount),
            children(children) { }
        
    public:
        static DIEDefinition parse(glue::IFile &file);
    };


    class Attribute
    {
    public:
        AttributeDefinition &definition;
        uint8_t *valueData;

    public:
        Attribute(AttributeDefinition &def, uint8_t *data)
            :definition(def), valueData(data) { }

    public:
        template<class T>
        inline T valueAs() { return *((T*)(this->valueData)); }

    };
}
