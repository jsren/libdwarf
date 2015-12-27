#pragma once
#include <stdint.h>

namespace dwarf
{
    namespace glue
    {
        void printf(const char *string);

        template <typename ...Args>
        void printf(const char *format, Args... items);

        class IFile
        {
        public:
            virtual int seek(size_t offset) = 0;
            virtual size_t read(void *buffer, size_t length) = 0;
        };
    }
}