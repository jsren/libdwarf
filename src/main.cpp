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

template<class T>
glue::IList<T> *glue::new_IList<T>()
{
    return new List<T>();
}

template glue::IList<DIEDefinition> *glue::new_IList
    <DIEDefinition>();

template glue::IList<AttributeDefinition> *glue::new_IList
<AttributeDefinition>();


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

int main(int argc, const char** args)
{
    FILE *file = fopen(args[1], "rb");
    FILEWrapper wrapper(file);

    FILE *outfile = fopen("../output_abbrev.bin", "wb+");

    int error = 0;
    ElfFile32* elfFile = new ElfFile32(wrapper, error);

    for (int i = 1; i < elfFile->header.e_shnum; i++)
    {
        puts(&elfFile->getSectionName(i));
    }

    uint8_t buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    size_t res = elfFile->readSectionData(".debug_abbrev", buffer, sizeof(buffer));
    
    if (res != 0)
    {
        volatile size_t sz = fwrite(buffer, 1, res, outfile);
        if (sz != res) return 1;
    }
    else return 1;

    return 0;
}
