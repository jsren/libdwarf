#include <stdio.h>

#include "elf.h"

namespace elf
{
    bool loadHeader(FILE *file, Header32 *header)
    {
        size_t s = fread(header, sizeof(header[0]), 1, file);
        return s == sizeof(header[0]);
    }
}
