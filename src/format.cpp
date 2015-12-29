#include "format.h"
#include <platform.h>

namespace dwarf
{
    int32_t uleb_read(uint8_t data[], /*out*/ uint32_t &value)
    {
        // Perform manual optimisation

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
        return -1; // This should never happen...
    }

    int32_t uleb_read(uint8_t data[], /*out*/ uint64_t &value)
    {
        int32_t i = 0;
        int shift = 0;

        value = 0; // Zero 
        
        for (int shift = 0; i <= sizeof(value); shift += 7, i++)
        {
            value |= ((uint64_t)data[i] << shift);
            if ((data[i] & 0b10000000) == 0) return i + 1;
            else value &= ~((uint64_t)(0b10000000 << shift));
        }

        // Consume any extra bytes
        while (data[i++] & 0b10000000) { }
        return i;
    }


    DIEDefinition DIEDefinition::parse(glue::IFile &file)
    {
        uint32_t tag = 0;
        DIEDefinition def{};
        bool hasChildren = false;
        uint8_t buffer[16] = { 0 };

        file.read(buffer, sizeof(id));
        uleb_read(buffer, def.id);

        file.read(buffer, sizeof(tag));
        uleb_read(buffer, tag);
        def.tag = (DIEType)tag;

        file.read(buffer, 1);
        hasChildren = buffer[0] != 0;
        
        auto atts = glue::new_IList<AttributeDefinition>();

        AttributeDefinition att;
        while (true)
        {
            uint32_t name, form;
            file.read(buffer, sizeof(name));
            uleb_read(buffer, name);

            file.read(buffer, sizeof(form));
            uleb_read(buffer, form);

            // Found end-of-attributes
            if (name == 0 && form == 0) break;

            att.name = (AttributeName)name;
            att.form = (AttributeForm)form;

            switch (att.form)
            {
            case AttributeForm::Address:
                att.class_ = AttributeClass::Address; break;
            case AttributeForm::Block2:
            case AttributeForm::Block4:
            case AttributeForm::Block:
            case AttributeForm::Block1:
                att.class_ = AttributeClass::Block; break;
            case AttributeForm::Data2:
            case AttributeForm::Data4:
            case AttributeForm::Data8:
            case AttributeForm::Data1:
            case AttributeForm::SData:
            case AttributeForm::UData:
                att.class_ = AttributeClass::Constant; break;
            case AttributeForm::String:
            case AttributeForm::Strp:
                att.class_ = AttributeClass::String; break;
            case AttributeForm::Flag:
            case AttributeForm::FlagPresent:
                att.class_ = AttributeClass::Flag; break;
            case AttributeForm::RefAddr:
                att.class_ = AttributeClass::Reference; break;
            case AttributeForm::Ref1:
            case AttributeForm::Ref2:
            case AttributeForm::Ref4:
            case AttributeForm::Ref8:
            case AttributeForm::RefUData:
            case AttributeForm::RefSig8:
                att.class_ = AttributeClass::UnitReference; break;
            case AttributeForm::Indirect:
                att.class_ = AttributeClass::None; break;
            case AttributeForm::SecOffset:
                att.class_ = AttributeClass::SectionPointer; break;
            case AttributeForm::ExprLoc:
                att.class_ = AttributeClass::ExprLoc; break;
            default:
                att.class_ = AttributeClass::None; break;
            }
            atts->add(att);
        }

        // Assign attributes
        if ((def.attributeCount = atts->count()) != 0) {
            def.attributes = atts->toArray();
        }

        // Handle child definitions
        if (hasChildren)
        {
            auto children = glue::new_IList<DIEDefinition>();

            while (true)
            {
                DIEDefinition child = parse(file);
                if (child.id != 0 || child.tag != DIEType::None) {
                    children->add(child);
                }
            }

            if ((def.childCount = children->count()) != 0) {
                def.children = children->toArray();
            }

            delete children;
        }
        return def;
    }

}
