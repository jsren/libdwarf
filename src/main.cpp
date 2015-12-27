#include <malloc.h>
#include <string.h>
#include <stdio.h>

#include "dwarf.h"
#include "format.h"
#include "elf.h"

using namespace dwarf;

void glue::printf(const char *string) {
    ::printf(string);
}
template <typename ...Args>
void glue::printf(const char *format, Args... items)
{
    ::printf(format, items...);
}

class FILEWrapper : public glue::IFile
{
private:
    FILE *file;

public:
    FILEWrapper(FILE *file) : file(file) { }

public:
    int seek(size_t offset) override {
        return fseek(this->file, (long)offset, 0);
    }
    size_t read(void *buffer, size_t length) override {
        return fread(buffer, 1, (long)length, this->file);
    }
};

/*
    The basic descriptive entity in DWARF is the debugging information entry (DIE). 
    A DIE has a tag that specifies what the DIE describes and a list of 
    attributes that fills in details, and further describes the entity.
*/

int main(int argc, const char** args)
{
    FILE *file = fopen(args[1], "rb");
    FILEWrapper wrapper(file);

    int error = 0;
    ElfFile32* elfFile = new ElfFile32(wrapper, error);

    for (int i = 1; i < elfFile->header.e_shnum; i++)
    {
        puts(&elfFile->getSectionName(i));
    }

    return 0;
}
