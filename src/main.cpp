/* main.cpp - (c) James S Renwick 
   ------------------------------
   Test file for libdwarf.
*/
#include <stdio.h>
#include <assert.h>
#include <cstring>
#include <memory>


#include "dwarf.h"
#include "format.h"
#include "elf.h"

using namespace dwarf;

template<class T>
class List : public glue::IList<T>
{
private:
    T *array;
    size_t capacity;
    size_t index;

public:
    List()
    {
        capacity = 0;
        index    = 0;
    }

    void add(T item) override
    {
        // Perform expansion as necessary
        if (index == capacity) 
        {
            // Perform initial alloc
            if (capacity == 0) 
            {
                capacity = 10;
                this->array = (T*)malloc(capacity * sizeof(T));
            }
            else
            {
                this->array = (T*)realloc(this->array,
                    (capacity = (capacity + capacity / 2)));
            }
        }
        array[index++] = item;
    }

    size_t count() override {
        return this->index;
    }
    T &operator [](size_t index) override {
        return this->array[index];
    }

    T *toArray() override 
    {
        T *newArray = (T*)malloc(index * sizeof(T));
        memcpy(newArray, this->array, index * sizeof(T));
        return newArray;
    }
};


class FILEWrapper : public glue::IFile
{
private:
    FILE *file;

public:
    FILEWrapper(FILE *file) : file(file) { }

public:
    int seek(size_t offset, glue::SeekMode mode) override 
    {
        return fseek(this->file, (long)offset, 
            mode == glue::SeekMode::Absolute ? SEEK_SET : SEEK_CUR);
    }
    size_t read(void *buffer, size_t length) override {
        return fread(buffer, 1, (long)length, this->file);
    }
    size_t getPosition() override {
        return ftell(this->file);
    }
};


/*
    The basic descriptive entity in DWARF is the debugging information entry (DIE). 
    A DIE has a tag that specifies what the DIE describes and a list of 
    attributes that fills in details, and further describes the entity.
*/

elf::Header32 fileHeader;
elf::SectionHeader32* sectionTable;

int main(int argc, const char** args)
{
    FILE* file = fopen("DwarfExample/dwarfexample.bin", "rb");
    assert(file != NULL);

    // Big allocation
    uint8_t* buffer = new uint8_t[54000];

    // Read file
    volatile size_t len = fread(buffer, 1, 53296, file);
    assert(len == 53296);

    // Attempt to parse file
    volatile auto error = elf::parseElfHeader(buffer, len, fileHeader);
    assert(error == sizeof(elf::Header32));
    
    // Parse section table
    assert(len > fileHeader.e_shoff);
    error = elf::parseSectionTable(buffer + fileHeader.e_shoff, 
        len - fileHeader.e_shoff, fileHeader.e_shnum, sectionTable);
    assert(error == fileHeader.e_shnum);

    // Get the string table
    const elf::SectionHeader32* strtab = nullptr;
    for (int i = 0; i < fileHeader.e_shnum; i++)
    {
        if (sectionTable[i].sh_type == elf::SectionType::StrTab) {
            strtab = &sectionTable[i]; break;
        }
    }
    assert(strtab != nullptr);


    dwarf::DwarfSection<>* sections = new dwarf::DwarfSection<>[6]();

    // Print section info
    for (int i = 0; i < fileHeader.e_shnum; i++)
    {
        const char* name = sectionTable[i].getName(
            reinterpret_cast<const char*>(buffer + strtab->sh_offset), 
            strtab->sh_size);

        printf("{Section '%-24s'; type %03d; length %08d}\n", name,
            sectionTable[i].sh_type, sectionTable[i].sh_size);


        dwarf::SectionType type;

        if (strcmp(name, ".debug_info") == 0) {
            type = dwarf::SectionType::debug_info;
        }
        else if (strcmp(name, ".debug_abbrev") == 0) {
            type = dwarf::SectionType::debug_abbrev;
        }
        else if (strcmp(name, ".debug_aranges") == 0) {
            type = dwarf::SectionType::debug_aranges;
        }
        else if (strcmp(name, ".debug_ranges") == 0) {
            type = dwarf::SectionType::debug_ranges;
        }
        else if (strcmp(name, ".debug_line") == 0) {
            type = dwarf::SectionType::debug_line;
        }
        else if (strcmp(name, ".debug_str") == 0) {
            type = dwarf::SectionType::debug_str;
        }
        else continue;

        // Check for duplicates
        assert(sections[(uint8_t)type-1].type == dwarf::SectionType::invalid);

        // Copy section
        sections[(uint8_t)type-1] = dwarf::DwarfSection<>(type, 
            buffer + sectionTable[i].sh_offset, sectionTable[i].sh_size);
    }

    auto context = dwarf::DwarfContext<>(sections);


    // Delete buffer
    delete[] buffer;

    return 0;
}
