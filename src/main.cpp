#include "dwarf.h"
#include "format.h"
#include "elf.h"

/*
    The basic descriptive entity in DWARF is the debugging information entry (DIE). 
    A DIE has a tag that specifies what the DIE describes and a list of 
    attributes that fills in details, and further describes the entity.
*/

int main()
{
    uint8_t testdata[] = { 57+0x80, 100 };
    
    uint64_t value;

    volatile int32_t  c = dwarf::uleb_read(testdata, value);
    volatile uint32_t b = value;

    return 0;
}
