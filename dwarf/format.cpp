/* format.cpp - (c) 2015-17, 2020 James S Renwick */
#include <vector>
#include "format.hpp"
#include "dwarf.hpp"

namespace dwarf
{
    std::size_t readHeader(const std::uint8_t* buffer, std::size_t length,
        std::uint64_t& id_out, std::uint32_t& type_out)
    {
        const std::uint8_t* origBuffer = buffer;

        // Read header
        auto size = dwarf::uleb_read(buffer, length, id_out);
		buffer += size; length -= size;

		if (id_out != 0) dwarf::uleb_read(buffer, length, type_out);

        // Terminate if null entry
        else return buffer - origBuffer;
    }


    std::size_t skipChildren(const std::uint8_t* buffer, std::size_t length)
    {
        const std::uint8_t* origBuffer = buffer;

        while (true)
        {
            // Skip header
            std::uint64_t id; std::uint32_t type;
            buffer += readHeader(buffer, length, id, type);
            if (id == 0) break;

            // Skip 'hasChildren'
            bool hasChildren = *(buffer++);

            // Skip attributes
            while (true)
            {
                std::uint32_t name, form;
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


    std::size_t countChildren(const std::uint8_t* buffer, std::size_t length, std::uint32_t& count_out)
    {
        const std::uint8_t* origBuffer = buffer;

        while (true)
        {
            // Skip header
            std::uint64_t id; std::uint32_t type;
            buffer += readHeader(buffer, length, id, type);
            if (id == 0) break;

            // Skip 'hasChildren'
            bool hasChildren = *(buffer++);

            // Skip attributes
            while (true)
            {
                std::uint32_t name, form;
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


    std::uint32_t DebugInfoAbbreviation::parse(const std::uint8_t* buffer, std::size_t length,
        DebugInfoAbbreviation& entry_out)
    {
		entry_out = DebugInfoAbbreviation{};
		const std::uint8_t* origBuffer = buffer;

        // Read header
        std::uint64_t id; std::uint32_t type;
        auto size = readHeader(buffer, length, entry_out.id, type);
		buffer += size; length -= size;

        if (entry_out.id == 0) return buffer - origBuffer;
		else entry_out.tag = static_cast<DIEType>(type);

        bool hasChildren = *(buffer++);

        entry_out.attributeCount = 0;
        entry_out.attributeStart = buffer;

        // Skip attributes
        while (true)
        {
			AttributeSpecification _;
			auto size = AttributeSpecification::parse(buffer, length, _);
			buffer += size; length -= size;

            if (_.name == AttributeName::None && _.form == AttributeForm::None) break;
            entry_out.attributeCount++;
        }

        // Handle child definitions
        if (hasChildren)
        {
            std::uint32_t childCount;
            auto size = countChildren(buffer, length, childCount);

            entry_out.childCount = childCount;
            entry_out.childIDs = std::unique_ptr<std::uint64_t[]>(new std::uint64_t[childCount]);

            std::uint32_t i = 0;
            while (true)
            {
                std::uint64_t id; std::uint32_t type;
                auto size = readHeader(buffer, length, id, type);
				buffer += size; length -= size;

                if (id == 0 && type == 0) break; // NULL entry - end of children
                else entry_out.childIDs[i++] = id;
            }
        }
        return buffer - origBuffer;
    }



    std::uint32_t AttributeSpecification::parse(const std::uint8_t* buffer, std::size_t length, AttributeSpecification& att_out)
	{
		const std::uint8_t* bufferStart = buffer;

		std::uint32_t name, form;
		auto size = dwarf::uleb_read(buffer, length, name);
		buffer += size; length -= size;

		size = dwarf::uleb_read(buffer, length, form);
		buffer += size; length -= size;

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


    std::uint32_t uleb_read(const std::uint8_t data[], std::size_t length, std::uint32_t &value_out)
    {
		if (length == 0) { value_out = 0; return 0; }

        // Perform manual unrolling
        value_out = data[0];
        if ((data[0] & 0b10000000) == 0 || length == 1) return 1;
        else value_out &= 0b01111111;

        value_out |= ((std::uint32_t)data[1] << 7);
        if ((data[1] & 0b10000000) == 0 || length == 2) return 2;
        else value_out &= ((0b01111111 << 7) | 0xFF);

        value_out |= ((std::uint32_t)data[2] << 14);
        if ((data[2] & 0b10000000) == 0 || length == 3) return 3;
        else value_out &= ((0b01111111 << 14) | 0xFFFF);

        value_out |= ((std::uint32_t)data[3] << 21);
        if ((data[3] & 0b10000000) == 0 || length == 4) return 4;
        else value_out &= ((0b01111111 << 21) | 0xFFFFFF);

        value_out |= ((std::uint32_t)data[4] << 28);
        if ((data[4] & 0b10000000) == 0 || length == 5) return 5;

        // Consume any extra bytes
        for (int i = sizeof(value_out) + 1; i < length; i++) {
            if ((data[i] & 0b10000000) == 0) return i + 1;
        }
		return 0; // This should never happen...
    }

    std::uint32_t uleb_read(const std::uint8_t data[], std::size_t length, /*out*/ std::uint64_t &value_out)
    {
        std::int32_t i = 0;
        value_out = 0; // Zero

        for (std::uint8_t shift = 0; i <= (std::uint8_t)sizeof(value_out) && i < length; shift += 7, i++)
        {
            value_out |= ((std::uint64_t)data[i] << shift);
            if ((data[i] & 0b10000000) == 0) return i + 1;
            else value_out &= ~((std::uint64_t)(0b10000000 << shift));
        }

        // Consume any extra bytes
        while (i < length && data[i++] & 0b10000000) { }
        return i;
    }


    std::uint32_t sleb_read(const std::uint8_t data[], std::int32_t& value_out)
    {
		throw "NotImplemented";
    }

    std::uint32_t sleb_read(const std::uint8_t data[], /*out*/ std::int64_t& value_out)
    {
		throw "NotImplemented";
    }
}
