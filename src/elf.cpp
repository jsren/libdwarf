#include <malloc.h>

#include "glue.h"
#include "elf.h"

using namespace dwarf::glue;

namespace dwarf
{
    ElfFile32::ElfFile32(IFile &file, int &error_out)
        : file(&file)
    {
        // Seek to start
        if (file.seek(0) != 0) goto error;

        // Read header
        if (file.read(&this->header, sizeof(this->header))
            != sizeof(this->header)) goto error;

        // Initialise sections
        uint32_t stringSectionCount = 0;

        this->sections = (SectionHeader32*)malloc(
            sizeof(SectionHeader32) * this->header.e_shnum);

        if (this->header.e_shoff == 0)
        {
            printf("[ERROR] Missing section table.");
            goto error;
        }

        // Read section headers
        for (uint32_t i = 0; i < header.e_shnum; i++)
        {
            file.seek(header.e_shoff + i*sizeof(sections[0]));

            if (file.read((void*)&this->sections[i], sizeof(sections[0])) 
                != sizeof(this->sections[0]))
            {
                printf("[ERROR] Invalid ELF header or unable to read file");
                goto error;
            }
            // If section header string table, load
            if (i == this->header.e_shstrndx)
            {
                if (sections[i].sh_type == SectionType::StrTab)
                {
                    this->strTable = (char*)malloc(sections[i].sh_size);

                    file.seek(sections[i].sh_offset);
                    if (file.read((void*)this->strTable, sections[i].sh_size) 
                        != sections[i].sh_size)
                    {
                        printf("[ERROR] Invalid ELF section table or unable to read file");
                        goto error;
                    }
                }
                else {
                    printf("[ERROR] Invalid ELF header or string table.");
                    goto error;
                }
            }
        }

        error_out = 0;
        return;
    error:
        error_out = 1;
        return;
    }

    const SectionHeader32 *ElfFile32::getSection(uint32_t index) const
    {
        return &this->sections[index];
    }

    const SectionHeader32 *ElfFile32::getSection(const char *name) const
    {
        for (uint32_t i = 0; i < this->header.e_shnum; i++) 
        {
            const char *str = &getSectionName(i);

            for (size_t n = 0; true; n++)
            {
                if (str[n] != name[n]) break;

                if (str[n] == '\n') {
                    return &this->sections[i];
                }
            }
        }
        return nullptr;
    }

    const char &ElfFile32::getSectionName(uint32_t index) const
    {
        return this->strTable[this->sections[index].sh_name];
    }

    size_t ElfFile32::readSectionData(uint32_t index, uint8_t *buffer, size_t bufferSize) const
    {
        const SectionHeader32 *section = this->getSection(index);

        this->file->seek(section->sh_offset);

        return this->file->read(buffer,
            (bufferSize < section->sh_size ? bufferSize : section->sh_size));
    }
    size_t ElfFile32::readSectionData(const char *name, uint8_t *buffer, size_t bufferSize) const
    {
        const SectionHeader32 *section = this->getSection(name);
        
        this->file->seek(section->sh_offset);

        return this->file->read(buffer,  
            (bufferSize < section->sh_size ? bufferSize : section->sh_size));
    }
}
