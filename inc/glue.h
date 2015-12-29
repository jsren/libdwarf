#pragma once
#include <stdint.h>

namespace dwarf
{
    namespace glue
    {
        void printf(const char *string);

        template <typename ...Args>
        void printf(const char *format, Args... items);

        enum class SeekMode
        {
            Absolute,
            Relative
        };

        template<class T>
        class IList
        {
        public:
            virtual void add(T item) = 0;
            virtual size_t count() = 0;

            virtual T *toArray() = 0;
            virtual T &operator [](size_t index) = 0;
        };

        template<class T>
        IList<T> *new_IList();

        class IFile
        {
        public:
            virtual size_t getPosition() = 0;
            virtual int seek(size_t offset) {
                return this->seek(offset, SeekMode::Absolute);
            }
            virtual int seek(size_t offset, SeekMode mode) = 0;
            virtual size_t read(void *buffer, size_t length) = 0;
        };
    }
}