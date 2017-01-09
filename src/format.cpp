/* format.cpp - (c) James S Renwick 2015-16 */
#include "format.h"
#include "platform.h"
#include "dwarf.h"

#include <vector>


namespace dwarf4
{

	uint32_t parseAttribute(const uint8_t* buffer, std::size_t length, AttributeDefinition att_out)
	{
		const uint8_t* bufferStart = buffer;

		uint32_t name, form;
		buffer += dwarf::uleb_read(buffer, name);
		buffer += dwarf::uleb_read(buffer, form);

		// Found end-of-attributes
		if (name == 0 && form == 0) goto end;

		att_out.name = (AttributeName)name;
		att_out.form = (AttributeForm)form;

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

     end:
		return buffer - bufferStart;
	}




    // TODO: Fix
    uint32_t DebugInfoEntry::parse(const uint8_t* buffer, std::size_t length, DebugInfoEntry& entry_out)
    {
		entry_out = DebugInfoEntry{};
		const uint8_t* bufferEnd = buffer + length;

        uint32_t tag = 0;
        bool hasChildren = false;

		buffer += dwarf::uleb_read(buffer, entry_out.id);
		buffer += dwarf::uleb_read(buffer, tag);
		
		entry_out.tag = static_cast<DIEType>(tag);
		hasChildren = buffer[0] != 0;
		buffer++;
        
        auto atts = std::vector<AttributeDefinition>();

        AttributeDefinition att;

        // Assign attributes
        if ((entry_out.attributeCount = atts.size()) != 0)
		{
			auto copy = std::unique_ptr<AttributeDefinition[]>(new AttributeDefinition[atts.size()]);
			std::memcpy(copy.get(), atts.data(), atts.size() * sizeof(AttributeDefinition));
			entry_out.attributes = std::move(copy);
        }

        // Handle child definitions
        if (hasChildren)
        {
			std::vector<DebugInfoEntry> children{};

            while (true)
            {
				DebugInfoEntry child;
				buffer += parse(buffer, bufferEnd - buffer, child);

                if (child.id != 0 || child.tag != DIEType::None) {
                    children.emplace_back(std::move(child));
                }
				else break; // Found NULL entry, end sibling chain
            }

			if ((entry_out.childCount = children.size()) != 0)
			{
				auto copy = std::unique_ptr<DebugInfoEntry[]>(new DebugInfoEntry[atts.size()]);
				std::memcpy(copy.get(), atts.data(), atts.size() * sizeof(DebugInfoEntry));
				entry_out.children = std::move(copy);
			}
        }

        return length - (bufferEnd - buffer);
    }

}
