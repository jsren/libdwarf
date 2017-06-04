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





    DwarfSection32::DwarfSection32(const DwarfSection32& other) : type(other.type), size(other.size)
    {
        if (other.data.ownsData())
        {
            data.reset(new uint8_t[size], true);
            memcpy(this->data.get(), other.data.get(), size);
        }
        else data.reset(other.data.get(), false);
    }

    DwarfSection32& DwarfSection32::operator=(const DwarfSection32& other)
    {
        if (this == &other) return *this;

        type = other.type; size = other.size;
        data.reset(new uint8_t[size], true);
        memcpy(this->data.get(), other.data.get(), size);

        return *this;
    }

    const DwarfSection32& DwarfContext32::operator[](SectionType type) const
    {
        static const DwarfSection32 invalidSection;

        for (auto& section : sections) { 
            if (section.type == type) return section; 
        }
        return invalidSection;
    }


    DwarfSection64::DwarfSection64(const DwarfSection64& other) : type(other.type), size(other.size)
    {
        if (other.data.ownsData())
        {
            data.reset(new uint8_t[size], true); 
            memcpy(this->data.get(), other.data.get(), size);
        }
        else data.reset(other.data.get(), false);
    }

    DwarfSection64& DwarfSection64::operator=(const DwarfSection64& other)
    {
        if (this == &other) return *this;

        type = other.type; size = other.size;
        data.reset(new uint8_t[size], true);
        memcpy(this->data.get(), other.data.get(), size);

        return *this;
    }

    const DwarfSection64& DwarfContext64::operator[](SectionType type) const
    {
        static const DwarfSection64 invalidSection;

        for (auto& section : sections) {
            if (section.type == type) return section;
        }
        return invalidSection;
    }


    // Returns -1 upon error, will advance valueData ptr to start of value
    // 'addressSize', 'dwarfWidth' should be 4 or 8
    std::size_t attributeSize(const AttributeSpecification& attr, 
        std::size_t addressSize, uint8_t dwarfWidth, const uint8_t*& value)
    {
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
            case AttributeForm::Ref1: return 1; 
            case AttributeForm::Ref2: return 2;
            case AttributeForm::Ref4: return 4;
            case AttributeForm::Ref8: return 8;
            case AttributeForm::RefUData: { uint64_t _; return uleb_read(value, _); }
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


    static class DebugEntryParser
    {
    public:
        static uint32_t nextAbbreviation(const uint8_t* buffer, std::size_t length, 
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
            DIEType& type_out, const char*& name_out, bool& hasChildren_out)
        {
            const uint8_t* origBuffer = buffer;

            uint32_t tag = 0;
            hasChildren_out = false;
            name_out = nullptr;

            auto& debug_abbrev = context[SectionType::debug_abbrev];
            if (!debug_abbrev) return 0;

            // Read header
            buffer += dwarf::uleb_read(buffer, abbrevID_out);
            // Terminate if null entry
            if (abbrevID_out == 0) return buffer - origBuffer;

            // Get abbreviation data from index
            auto index = context.abbreviationIndex.at(abbrevID_out);
            const uint8_t* origAbbrevData = debug_abbrev.data.get() + index;
            const uint8_t* abbrevData = origAbbrevData;

            // Read abbreviation header
            uint64_t _;
            abbrevData += dwarf::uleb_read(abbrevData, _);
            abbrevData += dwarf::uleb_read(abbrevData, tag);
            type_out = static_cast<DIEType>(tag);

            hasChildren_out = abbrevData[0];
            abbrevData++;

            // Handle attributes
            while (true)
            {
                // Read abbreviation attribute specifications
                AttributeSpecification attr;
                abbrevData += AttributeSpecification::parse(abbrevData,
                    debug_abbrev.size - (abbrevData - origAbbrevData), attr);

                // Break upon NULL specification
                if (attr.name == AttributeName::None &&
                    attr.form == AttributeForm::None) break;

                // Get attribute size 
                std::size_t size = attributeSize(attr, context.unitHeader().addressSize,
                    DwarfContext::bits/8, buffer);

                if (size == static_cast<std::size_t>(-1)) return static_cast<uint32_t>(-1);

                // Pull out 'name' attribute value
                if (attr.name == AttributeName::Name)
                {
                    if (attr.form == AttributeForm::String) {
                        name_out = reinterpret_cast<const char*>(buffer);
                    }
                    else if (attr.form == AttributeForm::Strp)
                    {
                        auto& debug_str = context[SectionType::debug_str];
                        if (debug_str)
                        {
                            uint64_t offset = 0;
                            memcpy(&offset, buffer, size);
                            name_out = reinterpret_cast<const char*>(debug_str.data.get() + offset);
                        }
                    }
                }
                // Advance buffer past value
                buffer += size;
            }

            return buffer - origBuffer;
        }



        template<typename DwarfContext>
        static std::size_t parseDIEChain(const uint8_t* buffer, std::size_t bufferSize, 
            DwarfContext& context, const uint8_t* sectionStart, uint64_t parentDIE)
        {
            const uint8_t* bufferStart = buffer;
            while (true)
            {
                // Parse next DIE
                uint64_t abbrevID; DIEType dietype; const char* name; bool hasChildren;
                auto size = nextDIE(buffer, bufferSize, context, abbrevID, dietype, name, hasChildren);

                // Update buffer view
                bufferSize -= size;
                buffer += size;

                // Break upon NULL entry
                if (abbrevID == 0) break;

                // Add DIE to index
                auto index = context.entryIndex.size();
                context.entryIndex.emplace_back(
                    dietype, parentDIE, name, buffer - sectionStart);

                // Process children if present
                if (hasChildren)
                {
                    auto offset = parseDIEChain(buffer, bufferSize, context, sectionStart, index);
                    bufferSize -= offset;
                    buffer += offset;
                }
            }
            return buffer - bufferStart;
        }


        template<typename DwarfContext>
        static error_t buildIndexes(DwarfContext& context)
        {
            auto& debug_abbrev = context[SectionType::debug_abbrev];
            auto& debug_info = context[SectionType::debug_info];

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
                    context.abbreviationIndex[abbrevID] = buffer - debug_abbrev.data.get();

                    // Update buffer view
                    bufferSize -= size;
                    buffer += size;
                }
            }

            // Index debug info entry table
            {
                const uint8_t* buffer = debug_info.data.get();
                uint32_t bufferSize = debug_info.size;

                // Skip past program header
                buffer += sizeof(context.header);
                bufferSize -= sizeof(context.header);

                parseDIEChain(buffer, bufferSize, context, debug_info.data.get(), 0);
            }
            return 0;
        }

        template<typename DwarfContext>
        static DebugInfoEntry dieFromId(uint64_t id, DwarfContext& context)
        {
            auto& index = context.entryIndex[id];
            auto offset = std::get<3>(index);

            auto& debug_info = context[SectionType::debug_info];
            auto& debug_abbrev = context[SectionType::debug_abbrev];

            const uint8_t* origBuffer = debug_info.data.get() + offset;
            const uint8_t* buffer = origBuffer;
            std::size_t length = debug_info.size - offset;

            const uint8_t* origAbbrevData = debug_abbrev.data.get();
            const uint8_t* abbrevData = origAbbrevData;


            // Parse abbreviation no.
            uint64_t abbrevId;
            buffer += uleb_read(buffer, abbrevId);
        
            // Get offset into abbreviation table
            abbrevData += context.abbreviationIndex[abbrevId];

            // Skip abbreviation header
            uint64_t _;
            abbrevData += dwarf::uleb_read(abbrevData, _);
            abbrevData += dwarf::uleb_read(abbrevData, _);
            abbrevData++;

            // Count attributes
            uint32_t attrCount = 0;
            const uint8_t* tmpBuff = abbrevData;
            while (true)
            {
                uint32_t name, form;

                tmpBuff += dwarf::uleb_read(tmpBuff, name);
                tmpBuff += dwarf::uleb_read(tmpBuff, form);

                if (name == 0 && form == 0) break;
                else attrCount++;
            }

            // Create entry
            DebugInfoEntry entry;
            entry.id = id;
            entry.abbreviationId = abbrevId;
            entry.type = std::get<0>(index);
            entry.attributeCount = attrCount;
            entry.attributes = std::unique_ptr<Attribute[]>(new Attribute[attrCount]);

            // Handle attributes
            uint32_t attrIndex = 0;
            while (true)
            {
                // Read abbreviation attribute specifications
                AttributeSpecification attr;
                abbrevData += AttributeSpecification::parse(abbrevData,
                    debug_abbrev.size - (abbrevData - origAbbrevData), attr);

                // Break upon NULL specification
                if (attr.name == AttributeName::None &&
                    attr.form == AttributeForm::None) break;

                // Get attribute size 
                std::size_t size = attributeSize(attr, context.unitHeader().addressSize,
                    DwarfContext::bits / 8, buffer);

                // If invalid size, return early
                if (size == static_cast<std::size_t>(-1)) 
                    return DebugInfoEntry();

                // Store attribute definition
                entry.attributes[attrIndex++] = Attribute(attr, buffer, size);

                // Advance buffer past value
                buffer += size;
            }
            return entry;
        }
    };

    error_t DwarfContext32::buildIndexes()
    {
        auto res = DebugEntryParser::buildIndexes(*this);
        dieIndex._begin = entryIndex.cbegin();
        dieIndex._end = entryIndex.cend();
        return res;
    }

    DebugInfoEntry DwarfContext32::dieFromId(uint64_t id)
    {
        return DebugEntryParser::dieFromId(id, *this);
    }


    error_t DwarfContext64::buildIndexes()
    {
        auto res = DebugEntryParser::buildIndexes(*this);
        dieIndex._begin = entryIndex.cbegin();
        dieIndex._end = entryIndex.cend();
        return res;
    }

    DebugInfoEntry DwarfContext64::dieFromId(uint64_t id)
    {
        return DebugEntryParser::dieFromId(id, *this);
    }


}
