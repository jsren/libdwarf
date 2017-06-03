/* format.cpp - (c) 2015-17 James S Renwick */
#include "format.h"
#include "platform.h"
#include "dwarf.h"

#include <vector>


namespace dwarf4
{
    std::size_t readHeader(const uint8_t* buffer, std::size_t length, 
        uint64_t& id_out, uint32_t& type_out)
    {
        const uint8_t* origBuffer = buffer;

        // Read header
        buffer += dwarf::uleb_read(buffer, id_out);
        if (id_out != 0) buffer += dwarf::uleb_read(buffer, type_out);

        // Terminate if null entry
        else return buffer - origBuffer;
    }


    std::size_t skipChildren(const uint8_t* buffer, std::size_t length)
    {
        const uint8_t* origBuffer = buffer;

        while (true)
        {
            // Skip header
            uint64_t id; uint32_t type;
            buffer += readHeader(buffer, length, id, type);
            if (id == 0) break;

            // Skip 'hasChildren'
            bool hasChildren = *(buffer++);

            // Skip attributes
            while (true)
            {
                uint32_t name, form;
                buffer += dwarf::uleb_read(buffer, name);
                buffer += dwarf::uleb_read(buffer, form);
                if (name == 0 && form == 0) break;
            }

            // Skip children
            if (hasChildren) {
                buffer += skipChildren(buffer, length - (buffer - origBuffer));
            }
        }
        return buffer - origBuffer;
    }


    std::size_t countChildren(const uint8_t* buffer, std::size_t length, uint32_t& count_out)
    {
        const uint8_t* origBuffer = buffer;

        while (true)
        {
            // Skip header
            uint64_t id; uint32_t type;
            buffer += readHeader(buffer, length, id, type);
            if (id == 0) break;

            // Skip 'hasChildren'
            bool hasChildren = *(buffer++);

            // Skip attributes
            while (true)
            {
                uint32_t name, form;
                buffer += dwarf::uleb_read(buffer, name);
                buffer += dwarf::uleb_read(buffer, form);
                if (name == 0 && form == 0) break;
            }

            // Skip children
            if (hasChildren) {
                buffer += skipChildren(buffer, length - (buffer - origBuffer));
            }
            count_out++;
        }
        return buffer - origBuffer;
    }


    uint32_t DebugInfoAbbreviation::parse(const uint8_t* buffer, std::size_t length, 
        DebugInfoAbbreviation& entry_out)
    {
		entry_out = DebugInfoAbbreviation{};
		const uint8_t* origBuffer = buffer;

        

        // Read header
        uint64_t id; uint32_t type;
        buffer += readHeader(buffer, length, entry_out.id, type);
        if (entry_out.id == 0) return buffer - origBuffer;
		else entry_out.tag = static_cast<DIEType>(type);

        bool hasChildren = *(buffer++);
        
        entry_out.attributeCount = 0;
        entry_out.attributeStart = buffer;

        // Skip attributes
        while (true)
        {
            uint32_t name, form;
            buffer += dwarf::uleb_read(buffer, name);
            buffer += dwarf::uleb_read(buffer, form);
            if (name == 0 && form == 0) break;

            entry_out.attributeCount++;
        }

        // Handle child definitions
        if (hasChildren)
        {
            uint32_t childCount; 
            auto size = countChildren(buffer, length - (buffer - origBuffer), childCount);

            entry_out.childCount = childCount;
            entry_out.childIDs = std::unique_ptr<uint64_t[]>(new uint64_t[childCount]);

            uint32_t i = 0;
            while (true)
            {
                uint64_t id; uint32_t type;
                buffer += readHeader(buffer, length - (buffer - origBuffer), id, type);

                if (id == 0 && type == 0) break; // NULL entry - end of children
                else entry_out.childIDs[i++] = id;
            }
        }
        return buffer - origBuffer;
    }


    


    uint32_t AttributeSpecification::parse(const uint8_t* buffer, std::size_t length, AttributeSpecification& att_out)
	{
		const uint8_t* bufferStart = buffer;

		uint32_t name, form;
		buffer += dwarf::uleb_read(buffer, name);
		buffer += dwarf::uleb_read(buffer, form);

		att_out.name = static_cast<AttributeName>(name);
		att_out.form = static_cast<AttributeForm>(form);


		switch (att_out.form)
		{
		case AttributeForm::Address:
			att_out.class_ = AttributeClass::Address; break;
		case AttributeForm::Block2:
		case AttributeForm::Block4:
		case AttributeForm::Block:
		case AttributeForm::Block1:
			att_out.class_ = AttributeClass::Block; break;
		case AttributeForm::Data2:
		case AttributeForm::Data4:
		case AttributeForm::Data8:
		case AttributeForm::Data1:
		case AttributeForm::SData:
		case AttributeForm::UData:
			att_out.class_ = AttributeClass::Constant; break;
		case AttributeForm::String:
		case AttributeForm::Strp:
			att_out.class_ = AttributeClass::String; break;
		case AttributeForm::Flag:
		case AttributeForm::FlagPresent:
			att_out.class_ = AttributeClass::Flag; break;
		case AttributeForm::RefAddr:
			att_out.class_ = AttributeClass::Reference; break;
		case AttributeForm::Ref1:
		case AttributeForm::Ref2:
		case AttributeForm::Ref4:
		case AttributeForm::Ref8:
		case AttributeForm::RefUData:
		case AttributeForm::RefSig8:
			att_out.class_ = AttributeClass::UnitReference; break;
		case AttributeForm::Indirect:
			att_out.class_ = AttributeClass::None; break;
		case AttributeForm::SecOffset:
			att_out.class_ = AttributeClass::SectionPointer; break;
		case AttributeForm::ExprLoc:
			att_out.class_ = AttributeClass::ExprLoc; break;
		default:
			att_out.class_ = AttributeClass::None; break;
		}
		return buffer - bufferStart;
	}


    /*uint32_t DebugInfoEntry::parse(const uint8_t* buffer, std::size_t length, 
        DebugInfoEntry& entry_out, const DebugInfoAbbreviation& rootAbbreviation)
    {
        const uint8_t* bufferStart = buffer;

        uint64_t abbrevID;
        buffer += dwarf::uleb_read(buffer, abbrevID);



    }*/

}
