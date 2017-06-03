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


    // Returns -1 upon error, will advance valueData ptr to start of value
    // 'dwarfWidth' should be 4 or 8
    std::size_t attributeSize(const dwarf4::AttributeSpecification& attr, 
        std::size_t addressSize, uint8_t dwarfWidth, const uint8_t*& value)
    {
        using namespace dwarf4;

        switch (attr.form) {
            // AttributeClass::Address
            case AttributeForm::Address: return addressSize;
            // AttributeClass::Block
            case AttributeForm::Block1: return *(value++);
            case AttributeForm::Block2: { uint16_t size; memcpy(&size, value, 2); value += 2; return size; }
            case AttributeForm::Block4: { uint32_t size; memcpy(&size, value, 4); value += 4; return size; }
            case AttributeForm::Block:  { uint64_t size; value += uleb_read(value, size); return size; }
            // AttributeClass::Constant
            case AttributeForm::Data1: return 1;
            case AttributeForm::Data2: return 2;
            case AttributeForm::Data4: return 4;
            case AttributeForm::Data8: return 8;
            case AttributeForm::SData: { uint64_t _; return uleb_read(value, _); }
            case AttributeForm::UData: { uint64_t _; return uleb_read(value, _); }
            // AttributeClass::ExprLoc
            case AttributeForm::ExprLoc:
                { uint64_t size; value += uleb_read(value, size); return size; }
            // AttributeClass::Flag
            case AttributeForm::Flag: return 1;
            case AttributeForm::FlagPresent: return 0;
            // AttributeClass::SectionPointer
            case AttributeForm::SecOffset: return dwarfWidth;
            // AttributeClass::UnitReference
            case AttributeForm::Ref1: return *(value++); 
            case AttributeForm::Ref2: { uint16_t size; memcpy(&size, value, 2); value += 2; return size; }
            case AttributeForm::Ref4: { uint32_t size; memcpy(&size, value, 4); value += 4; return size; }
            case AttributeForm::Ref8: { uint64_t size; memcpy(&size, value, 4); value += 8; return size; }
            case AttributeForm::RefUData: { uint64_t size; value += uleb_read(value, size); return size; }
            case AttributeForm::RefSig8: return 8; 
            // AttributeClass::Reference
            case AttributeForm::RefAddr: return dwarfWidth;
            // AttributeClass::String
            case AttributeForm::String: return strlen(reinterpret_cast<const char*>(value)) + 1;
            case AttributeForm::Strp: return dwarfWidth;
        }
        // Indicate error - unknown form
        return static_cast<std::size_t>(-1);
    }


    uint32_t nextAbbreviation(const uint8_t* buffer, std::size_t length, 
        uint64_t& abbrevID_out)
    {
        const uint8_t* origBuffer = buffer;

        uint32_t tag = 0;
        bool hasChildren = false;

        // Read header
        buffer += dwarf::uleb_read(buffer, abbrevID_out);
        if (abbrevID_out != 0) {
            buffer += dwarf::uleb_read(buffer, tag);
        }
        // Terminate if null entry
        else return buffer - origBuffer;

        // Skip over 'hasChildren'
        buffer++;

        // Skip attributes
        while (true)
        {
            uint32_t name, form;
            buffer += dwarf::uleb_read(buffer, name);
            buffer += dwarf::uleb_read(buffer, form);
            if (name == 0 && form == 0) break;
        }
        return buffer - origBuffer;
    }

    template<typename DwarfContext>
    static uint32_t nextDIE(const uint8_t* buffer, std::size_t length,
        const DwarfContext& context, uint64_t& abbrevID_out, 
        dwarf4::DIEType& type_out, const char* name_out)
    {
        const uint8_t* origBuffer = buffer;

        uint32_t tag = 0;
        bool hasChildren = false;

        auto& debug_abbrev = context[SectionType::debug_abbrev];

        // Read header
        buffer += dwarf::uleb_read(buffer, abbrevID_out);
        // Terminate if null entry
        if (abbrevID_out == 0) return buffer - origBuffer;

        // Get abbreviation data from index
        auto index = context.abbreviationIndex.at(abbrevID_out);
        const uint8_t* abbrevData = debug_abbrev.data.get() + index;

        // Read abbreviation header
        uint64_t _;
        abbrevData += dwarf::uleb_read(abbrevData, _);
        abbrevData += dwarf::uleb_read(abbrevData, tag);
        type_out = static_cast<dwarf4::DIEType>(tag);

        hasChildren = abbrevData[0];
        abbrevData++;

        // Handle attributes
        while (true)
        {
            // Read abbreviation attribute specifications
            dwarf4::AttributeSpecification attr;
            abbrevData += dwarf4::AttributeSpecification::parse(abbrevData,
                length - (buffer - origBuffer), attr);

            // Break upon NULL specification
            if (attr.name == dwarf4::AttributeName::None &&
                attr.form == dwarf4::AttributeForm::None) break;

            // Get attribute size 
            std::size_t size = attributeSize(attr, DwarfContext::bits/8, DwarfContext::bits/8, buffer);
            if (size == static_cast<std::size_t>(-1)) return static_cast<uint32_t>(-1);

            // Handle 'name' attribute
            if (attr.name == dwarf4::AttributeName::Name)
            {
                if (attr.form == dwarf4::AttributeForm::String) {
                    name_out = reinterpret_cast<const char*>(buffer);
                } else if (attr.form == dwarf4::AttributeForm::Strp) {

                }
            }

            // Advance buffer past value
            buffer += size;
        }

        return buffer - origBuffer;
    }


    error_t DwarfContext64::buildIndexes()
    {
        auto& debug_abbrev = (*this)[SectionType::debug_abbrev];
        auto& debug_info = (*this)[SectionType::debug_info];

        // Index abbreviation table - this must be done first
        {
            const uint8_t* buffer = debug_abbrev.data.get();
            uint32_t bufferSize = debug_abbrev.size;

            while (true)
            {
                // Parse next abbreviation
                uint64_t abbrevID;
                auto size = nextAbbreviation(buffer, bufferSize, abbrevID);
                if (abbrevID == 0) break;

                // Store ID<->offset in index
                this->abbreviationIndex[abbrevID] = buffer - debug_abbrev.data.get();

                // Update buffer view
                bufferSize -= size;
                buffer += size;
            }
        }

        // Index debug info entry table
        {
            const uint8_t* buffer = debug_info.data.get();
            uint32_t bufferSize = debug_info.size;

            while (true)
            {
                // Parse next DIE
                uint64_t abbrevID; dwarf4::DIEType dietype; const char* name;
                auto size = nextDIE(buffer, bufferSize, debug_abbrev, abbrevID, dietype, name);
                if (abbrevID == 0) break;

                // Add DIE to index
                entryIDIndex.emplace_back(dietype, abbrevID, name, buffer - debug_info.data.get());

                // Update buffer view
                bufferSize -= size;
                buffer += size;
            }
        }

        return 0;
    }
}
