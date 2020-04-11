/**
 * Copyright (c) 2020 James Renwick
 */
#include <vector>
#include <assert.h>
#include <cstring>
#include <cstdio>
#include <memory>
#include <algorithm>
#include "elf/elf.hpp"


const std::uint8_t* read_file(const char* filepath, std::size_t& size_out)
{
    std::FILE* file = std::fopen(filepath, "rb");
    assert(file != NULL);

    std::fseek(file, 0, SEEK_END);
    size_out = static_cast<std::size_t>(std::ftell(file));
    std::rewind(file);

    // Read file
    std::uint8_t* buffer = new std::uint8_t[size_out];
    std::size_t len = std::fread(buffer, 1, size_out, file);
    assert(len == size_out);

	// Close file
	std::fclose(file);
    return buffer;
}


int main(int argc, const char** args)
{
    using namespace elf;

    assert(argc == 2);

    std::size_t len;
    const std::uint8_t* buffer = read_file(args[1], len);

    // Read header
    elf::Header header;
    assert(decodeElfHeader(buffer, len, header));

    std::printf("Elf class: %s\n", header.el_class() == elf::ElfClass::class64 ? "64-bit" : "32-bit");
    std::printf("Section count: %d\n", header.e_shnum);

    // Get section name string table
    elf::SectionHeader shstrSection = *std::next(iter_section_headers(buffer, header).begin(), header.e_shstrndx);
    auto shstrtab = reinterpret_cast<const char*>(buffer + shstrSection.sh_offset);

    // Print section info
    std::printf("Sections:\n");
    elf::SectionHeader prevSection{};

    std::size_t index = 0;
    for (const elf::SectionHeader& section : iter_section_headers(buffer, header))
    {
        // Print section information
        const char* name = &shstrtab[section.sh_name];
        std::printf("  Section type 0x%08x @0x%012x (0x%06x bytes) \"%s\"\n",
                    section.sh_type, section.sh_addr, section.sh_size, name);

        // Store symbol table to match with string table
        if (section.sh_type == elf::SectionType::SymTab)
        {
            prevSection = section;
        }
        // Once corresponding strtab found, print symbols with name
        else if (section.sh_type == elf::SectionType::StrTab && prevSection.sh_type != elf::SectionType::Null)
        {
            for (auto& symbol : iter_symbols(buffer, prevSection, header.el_class()))
            {
                const char* strtab = reinterpret_cast<const char*>(buffer + section.sh_offset);
                const char* name = &strtab[symbol.st_name];

                std::printf("    Symbol @0x%012x section %05d (0x%06x bytes) \"%s\"\n",
                            symbol.st_value, symbol.st_shndx, symbol.st_size, name);
            }
            prevSection.sh_type = elf::SectionType::Null;
        }
        else
        {
            prevSection.sh_type = elf::SectionType::Null;
        }
        index++;
    }
}
