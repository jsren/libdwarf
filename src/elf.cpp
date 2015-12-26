#include <stdio.h>

#include "elf.h"

namespace dwarf
{
    bool loadHeader(FILE *file, Header32 *header)
    {
        if (fseek(file, 0, 0) != 0) return false;

        char buffer[sizeof(header[0])];

        size_t s = fread(buffer, 1, sizeof(header[0]), file);
        return s == sizeof(header[0]);
    }
}
