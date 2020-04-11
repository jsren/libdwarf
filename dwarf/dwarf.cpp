#include "dwarf.hpp"
#include "format.hpp"
#include <cstring>

namespace dwarf
{

    SectionType SectionTypeFromString(const char* name)
    {
        if (std::strcmp(name, ".debug_info") == 0) {
            return dwarf::SectionType::debug_info;
        }
        else if (std::strcmp(name, ".debug_abbrev") == 0) {
            return dwarf::SectionType::debug_abbrev;
        }
        else if (std::strcmp(name, ".debug_aranges") == 0) {
            return dwarf::SectionType::debug_aranges;
        }
        else if (std::strcmp(name, ".debug_ranges") == 0) {
            return dwarf::SectionType::debug_ranges;
        }
        else if (std::strcmp(name, ".debug_line") == 0) {
            return dwarf::SectionType::debug_line;
        }
        else if (std::strcmp(name, ".debug_str") == 0) {
            return dwarf::SectionType::debug_str;
        }
        else return SectionType::invalid;
    }





    DwarfSection::DwarfSection(const DwarfSection& other) : type(other.type), size(other.size)
    {
        if (other.data.ownsData())
        {
            data.reset(new std::uint8_t[size], true);
            std::memcpy(this->data.get(), other.data.get(), size);
        }
        else data.reset(other.data.get(), false);
    }

    DwarfSection& DwarfSection::operator=(const DwarfSection& other)
    {
        if (this == &other) return *this;

        type = other.type; size = other.size;
        data.reset(new std::uint8_t[size], true);
        std::memcpy(this->data.get(), other.data.get(), size);

        return *this;
    }

    const DwarfSection& DwarfContext::operator[](SectionType type) const
    {
        static const DwarfSection invalidSection;

        for (auto& section : sections) {
            if (section.type == type) return section;
        }
        return invalidSection;
    }




    // Returns -1 upon error, will advance valueData ptr to start of value
    // 'addressSize', 'dwarfWidth' should be 4 or 8
	std::size_t attributeSize(const AttributeSpecification& attr, std::size_t addressSize,
		std::uint8_t dwarfWidth, const std::uint8_t*& value, std::size_t valueLength)
    {
        switch (attr.form) {
            // AttributeClass::Address
            case AttributeForm::Address: return addressSize;
            // AttributeClass::Block
            case AttributeForm::Block1: return *(value++);
            case AttributeForm::Block2: { std::uint16_t size; std::memcpy(&size, value, 2); value += 2; return size; }
            case AttributeForm::Block4: { std::uint32_t size; std::memcpy(&size, value, 4); value += 4; return size; }
            case AttributeForm::Block:  { std::uint64_t size; value += uleb_read(value, valueLength, size); return size; }
            // AttributeClass::Constant
            case AttributeForm::Data1: return 1;
            case AttributeForm::Data2: return 2;
            case AttributeForm::Data4: return 4;
            case AttributeForm::Data8: return 8;
            case AttributeForm::SData: { std::uint64_t _; return uleb_read(value, valueLength, _); }
            case AttributeForm::UData: { std::uint64_t _; return uleb_read(value, valueLength, _); }
            // AttributeClass::ExprLoc
            case AttributeForm::ExprLoc:
                { std::uint64_t size; value += uleb_read(value, valueLength, size); return size; }
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
            case AttributeForm::RefUData: { std::uint64_t _; return uleb_read(value, valueLength, _); }
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


	extern std::size_t readHeader(const std::uint8_t* buffer, std::size_t length,
		std::uint64_t& id_out, std::uint32_t& type_out);


    class DebugEntryParser
    {
    public:
        static std::uint32_t nextAbbreviation(const std::uint8_t* buffer, std::size_t length,
            std::uint64_t& abbrevID_out)
        {
            const std::uint8_t* origBuffer = buffer;

            std::uint32_t tag = 0;
            bool hasChildren = false;

            // Read header
			auto size = readHeader(buffer, length, abbrevID_out, tag);
			buffer += size; length -= size;

            // Terminate if null entry
            if (abbrevID_out == 0) return buffer - origBuffer;

            // Skip over 'hasChildren'
			buffer++; length--;

            // Skip attributes
            while (true)
            {
				AttributeSpecification _;
				auto size = AttributeSpecification::parse(buffer, length, _);

				buffer += size; length -= size;
                if (_.name == AttributeName::None && _.form == AttributeForm::None) break;
            }
            return buffer - origBuffer;
        }


        static std::uint32_t nextDIE(const std::uint8_t* buffer, std::size_t length,
            const DwarfContext& context, std::uint64_t& abbrevID_out,
            DIEType& type_out, const char*& name_out, bool& hasChildren_out)
        {
            const std::uint8_t* origBuffer = buffer;

            hasChildren_out = false;
            name_out = nullptr;
			type_out = DIEType::None;

            auto& debug_abbrev = context[SectionType::debug_abbrev];
            if (!debug_abbrev) return 0;

            // Read header
            auto size = dwarf::uleb_read(buffer, length, abbrevID_out);
			buffer += size; length -= size;
            // Terminate if null entry
            if (abbrevID_out == 0) return buffer - origBuffer;

            // Get abbreviation data from index
            auto index = context.abbreviationIndex.at(abbrevID_out);
            const std::uint8_t* origAbbrevData = debug_abbrev.data.get();
            const std::uint8_t* abbrevData = origAbbrevData + index;

            // Read abbreviation header
			std::uint64_t _; std::uint32_t tag;
			abbrevData += readHeader(abbrevData, debug_abbrev.size - (abbrevData - origAbbrevData), _, tag);
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
                auto size = attributeSize(attr, context.unitHeader().addressSize(),
					context.width == DwarfWidth::Bits64 ? 8 : 4, buffer, length);

                if (size == static_cast<std::size_t>(-1)) return static_cast<std::uint32_t>(-1);

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
                            std::uint64_t offset = 0;
                            std::memcpy(&offset, buffer, size);
                            name_out = reinterpret_cast<const char*>(debug_str.data.get() + offset);
                        }
                    }
                }
                // Advance buffer past value
                buffer += size;
            }

            return buffer - origBuffer;
        }


        static std::size_t parseDIEChain(const std::uint8_t* buffer, std::size_t bufferSize,
            DwarfContext& context, const std::uint8_t* sectionStart, std::uint64_t parentDIE)
        {
            const std::uint8_t* bufferStart = buffer;
            while (true)
            {
                // Parse next DIE
                std::uint64_t abbrevID; DIEType dietype; const char* name; bool hasChildren;
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


        static error_t buildIndexes(DwarfContext& context)
        {
            auto& debug_abbrev = context[SectionType::debug_abbrev];
            auto& debug_info = context[SectionType::debug_info];

            // Index abbreviation table - this must be done first
            {
                const std::uint8_t* buffer = debug_abbrev.data.get();
                std::uint32_t bufferSize = debug_abbrev.size;

                while (true)
                {
                    // Parse next abbreviation
                    std::uint64_t abbrevID;
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
                const std::uint8_t* buffer = debug_info.data.get();
                std::uint32_t bufferSize = debug_info.size;

                // Skip past program header
				auto headerSize = context.width == DwarfWidth::Bits64 ?
					sizeof(CompilationUnitHeader64) : sizeof(CompilationUnitHeader32);

                buffer += headerSize; bufferSize -= headerSize;

                parseDIEChain(buffer, bufferSize, context, debug_info.data.get(), 0);
            }
            return 0;
        }


        static DebugInfoEntry dieFromId(std::uint64_t id, DwarfContext& context)
        {
            auto& index = context.entryIndex[id];
            auto offset = std::get<3>(index);

            auto& debug_info = context[SectionType::debug_info];
            auto& debug_abbrev = context[SectionType::debug_abbrev];

            const std::uint8_t* origBuffer = debug_info.data.get() + offset;
            const std::uint8_t* buffer = origBuffer;
            std::size_t length = debug_info.size - offset;

            const std::uint8_t* origAbbrevData = debug_abbrev.data.get();
            const std::uint8_t* abbrevData = origAbbrevData;


            // Parse abbreviation no.
            std::uint64_t abbrevId;
            auto size = uleb_read(buffer, length, abbrevId);
			buffer += size; length -= size;

            // Get offset into abbreviation table
            auto abbrev_offset = context.abbreviationIndex[abbrevId];
			auto abbrevLength = debug_abbrev.size - abbrev_offset;
			abbrevData += abbrev_offset;

            // Skip abbreviation header
			std::uint64_t _; std::uint32_t _1;
			size = readHeader(abbrevData, abbrevLength, _, _1);
			abbrevData += size; abbrevLength -= size;
            abbrevData++;

            // Count attributes
            std::uint32_t attrCount = 0;
            const std::uint8_t* tmpBuff = abbrevData;
			auto tmpLength = abbrevLength;
            while (true)
            {
                std::uint32_t name, form;

				auto size = dwarf::uleb_read(tmpBuff, tmpLength, name);
				tmpBuff += size; tmpLength -= size;

                size = dwarf::uleb_read(tmpBuff, tmpLength, form);
				tmpBuff += size; tmpLength -= size;

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
            std::uint32_t attrIndex = 0;
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
                std::size_t size = attributeSize(attr, context.unitHeader().addressSize(),
                    context.width == DwarfWidth::Bits64 ? 8 : 4, buffer, length);

                // If invalid size, return early
                if (size == static_cast<std::size_t>(-1)) return DebugInfoEntry();

                // Store attribute definition
                entry.attributes[attrIndex++] = Attribute(attr, buffer, size);

                // Advance buffer past value
                buffer += size;
            }
            return entry;
        }
    };



	DwarfContext::DwarfContext(std::vector<DwarfSection>&& sections, DwarfWidth width) :
		sections(std::move(sections)), width(width)
	{
		// Copy compilation unit header from debug_info section, if found
		for (auto& section : this->sections) if (section.type == SectionType::debug_info)
		{
			if (width == DwarfWidth::Bits32) {
				CompilationUnitHeader32 header; std::memcpy(&header, section.data.get(), sizeof(header));
				this->header.reset(new _detail::CompilationUnitHeader32(header));
			}
			else {
				CompilationUnitHeader64 header; std::memcpy(&header, section.data.get(), sizeof(header));
				this->header.reset(new _detail::CompilationUnitHeader64(header));
			}
			break;
		}
	}


    error_t DwarfContext::buildIndexes()
    {
        auto res = DebugEntryParser::buildIndexes(*this);
        dieIndex._begin = entryIndex.cbegin();
        dieIndex._end = entryIndex.cend();
        return res;
    }


    DebugInfoEntry DwarfContext::dieFromId(std::uint64_t id)
    {
        return DebugEntryParser::dieFromId(id, *this);
    }

}
