#include <malloc.h>
#include <string.h>
#include <stdio.h>

#include "dwarf.h"
#include "format.h"
#include "elf.h"

using namespace dwarf;

/*
    The basic descriptive entity in DWARF is the debugging information entry (DIE). 
    A DIE has a tag that specifies what the DIE describes and a list of 
    attributes that fills in details, and further describes the entity.
*/

int main(int argc, const char** args)
{
    FILE *file = fopen(args[1], "rb");

    dwarf::Header32 header;
    dwarf::loadHeader(file, &header);

    const char *strings;
    
    if (header.e_shoff == 0 || fseek(file, header.e_shoff, 0) != 0) {
        printf("[ERROR] Missing section table or unable to seek.");
    }

    
    const char **strTables = NULL;

    dwarf::SectionHeader32 *sections = (SectionHeader32*)
        malloc(sizeof(SectionHeader32) * header.e_shnum);

    uint32_t stringSectionCount = 0;

    for (uint32_t i = 0; i < header.e_shnum; i++)
    {
        if (fread(&sections[i], 1, sizeof(sections[0]), file) != sizeof(sections[0]))
        {
            printf("[ERROR] Invalid ELF header or unable to read file");
            break;
        }
        if (sections[i].sh_type == SectionType::StrTab) {
            stringSectionCount++;
        }
    }
    strTables = (const char **)malloc(sizeof(strTables[0]) * stringSectionCount);

    for (uint32_t i = 0, n = 0; i < header.e_shnum, n < stringSectionCount; i++)
    {
        if (sections[i].sh_type == SectionType::StrTab) 
        {
            char *data = (char*)malloc(header.e_ehsize);

            if (fread(data, 1, header.e_ehsize, file) != header.e_ehsize) {
                printf("[ERROR] Invalid ELF section table or unable to read file");
            }
            strTables[n++] = data;
        }
    }

    SectionHeader32 *dbgSection = NULL;

    uint32_t i = 0;
    for (; i < header.e_shnum; i++)
    {
        if (sections[i].sh_type != dwarf::SectionType::ProgBits) continue;

        strings = (const char *)strTables[sections[i].sh_link];
        const char *name = &strings[sections[i].sh_name];

        if (strcmp(name, ".debug-info") == 0) 
        {
            dbgSection = &sections[i];
            break;
        }
    }
    if (dbgSection == NULL)
    {
        printf("[ERROR] No debug section found.");
        return 1;
    }



    return 0;
}
