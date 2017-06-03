/* main.cpp - (c) James S Renwick 
   ------------------------------
   Test file for libdwarf.
*/

#include "dwarf.h"
#include "format.h"
#include "elf.h"
#include "../lines.h"

#include <stdio.h>
#include <cstring>
#include <memory>
#include <vector>
#include <assert.h>
#include <algorithm>

using namespace dwarf;
using namespace dwarf2;
using namespace dwarf4;

/*
    The basic descriptive entity in DWARF is the debugging information entry (DIE). 
    A DIE has a tag that specifies what the DIE describes and a list of 
    attributes that fills in details, and further describes the entity.
*/

elf::Header32 fileHeader{};
elf::SectionTable32 sectionTable{};

std::vector<elf::SymbolTable32> symbolTables{};


int main(int argc, const char** args)
{
    FILE* file = fopen("DwarfExample/dwarfexample.bin", "rb");
    assert(file != NULL);

    // Big allocation
    uint8_t* buffer = new uint8_t[54000];

    // Read file
    volatile size_t len = fread(buffer, 1, 53296, file);
    assert(len == 53296);

    // Attempt to parse ELF header
    volatile auto error = elf::parseElfHeader(buffer, len, fileHeader);
    assert(error == sizeof(elf::Header32));
    
    // Parse section table
    assert(len > fileHeader.e_shoff);
    error = elf::parseSectionTable(buffer + fileHeader.e_shoff, 
        len - fileHeader.e_shoff, fileHeader.e_shnum, sectionTable);
    assert(error == 0);

    // Parse program header
    elf::ProgramHeader32 programHeader{};

    error = elf::parseProgramHeader(buffer + fileHeader.e_phoff, 
        fileHeader.e_phnum * fileHeader.e_phentsize, fileHeader.e_phnum, programHeader);
    assert(error == 0);

    // Print program header
    for (uint32_t i = 0; i < programHeader.segmentCount; i++)
    {
        const auto& segment = programHeader.segments[i];

        printf("{Segment %-3d; type %03d; offset %08d; loadaddr %08d; length %08d; flags: %d}\n", i, segment.p_type, 
            segment.p_offset, segment.p_vaddr, segment.p_filesz, segment.p_flags);
    }
    printf("-------------------------------------\n");

    std::vector<elf::StringTable32> strtables{};


    // Get special sections
    for (int i = 0; i < sectionTable.headerCount; i++)
    {
        // Get string table
        if (sectionTable[i].sh_type == elf::SectionType::StrTab)
        {
            strtables.emplace_back(
                elf::Pointer<const char[]>(reinterpret_cast<char*>(buffer + sectionTable[i].sh_offset), false),
                sectionTable[i].sh_size
            );
        }
        // Parse symbol table(s)
        else if (sectionTable[i].sh_type == elf::SectionType::SymTab)
        {
            elf::SymbolTable32 symTable{};

            error = elf::parseSymbolTable(buffer + sectionTable[i].sh_offset,
                len - sectionTable[i].sh_offset,
                sectionTable[i].sh_size / sectionTable[i].sh_entsize,
                symTable);
            assert(error == 0);

            symbolTables.emplace_back(std::move(symTable));
        }
    }
    assert(strtables.size() != 0);


    // Print string table
    for (auto& strtable : strtables)
    {
        for (uint32_t i = 0; i < strtable.size; i++)
        {
            printf("%05d: \"%s\"\n", i, strtable[i]);
            i += strlen(strtable[i]);
        }
        printf("-------------------------------------\n");
    }

    // Print symbol table, skipping empty entry
    for (size_t i = 1; i < symbolTables[0].entryCount; i++)
    {
        const char* str = strtables[1][symbolTables[0].entries[i].st_name];
        printf("| Symbol '%s' type %d |\n", str, symbolTables[0].entries[i].st_info_type);
    }


    std::vector<DwarfSection32> sections{};

    // Print section info
    for (int i = 0; i < fileHeader.e_shnum; i++)
    {
        const char* name = strtables[0][sectionTable[i].sh_name];

        printf("{Section '%-24s'; type %03d; offset %08d; length %08d}\n", name,
			sectionTable[i].sh_type, sectionTable[i].sh_offset, sectionTable[i].sh_size);

        SectionType type = SectionTypeFromString(name);
        if (type == SectionType::invalid) continue;

        // Copy section
        sections.emplace_back(type, 
            elf::Pointer<uint8_t[]>(buffer + sectionTable[i].sh_offset, false), 
            sectionTable[i].sh_size);
    }

    DwarfContext32 context(Array<DwarfSection32>(sections.data(), sections.size(), false));
    context.buildIndexes();


    DwarfSection32 debug_line;
    if ((debug_line = context[SectionType::debug_line]))
    {
        // Parse header
        size_t length;
        dwarf2::LineNumberProgramHeader32 header;

        auto error = dwarf2::LineNumberProgramHeader32::parse(debug_line.data.get(),
            debug_line.size, header, length, false);
        assert(error == 0);

        for (auto& file : header.fileEntries)
        {
            if (file.includeDirIndex != 0) {
                printf("| file '%s/%s' |\n", header.includeDirs[file.includeDirIndex - 1].get(), file.filepath.get());
            }
            else printf("| file '%s/%s' |\n", ".", file.filepath.get());
        }
    }

    // Delete buffer
    delete[] buffer;

    return 0;
}
