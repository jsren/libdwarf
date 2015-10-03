#include <dwarf.h>

int main()
{
    uint8_t testdata[] = { 57+0x80, 100 };
    
    uint64_t value;

    volatile int32_t  c = dwarf::uleb_read(testdata, value);
    volatile uint32_t b = value;

    return 0;
}
