/* array.h - (c) 2017 James S Renwick
   ----------------------------------
   Authors: James S Renwick
*/
#pragma once

#include "pointer.h"
#include <iterator>


namespace dwarf
{

    template<typename T>
    struct Array
    {
        elf::Pointer<T[]> data{};
        size_t length{};

    private:
        template<typename Y>
        using if_memcpy = std::enable_if_t<std::is_trivially_copyable<Y>::value 
            && std::is_same<T, Y>::value, int>;
        template<typename Y>
        using if_no_memcpy = std::enable_if_t<(!std::is_trivially_copyable<Y>::value 
            || !std::is_same<T, Y>::value) && std::is_copy_assignable<Y>::value, int>;
        template<typename Y>
        using if_no_copy = std::enable_if_t<!std::is_trivially_copyable<Y>::value 
            && !std::is_copy_assignable<Y>::value, int>;

        // memcpy if trivially copyable
        template<class U = T, if_memcpy<U> = 0>
        inline void copyData(U data[], size_t length) {
            memcpy(const_cast<std::remove_const_t<T>*>(this->data.get()), data, length * sizeof(T));
        }
        // Perform item-by-item copy if not trivially copyable
        template<class U = T, if_no_memcpy<U> = 0>
        inline void copyData(U data[], size_t length) {
            for (size_t i = 0; i < length; i++) { this->data[i] = data[i]; }
        }
        // Throw static assertion if not copyable at all
        template<class U = T, if_no_copy<U> = 0>
        inline void copyData(U data[], size_t length) {
            static_assert(false, "Cannot create array: given element type is not "
                "trivially copyable or copy-assignable.");
        }

    public:
        inline constexpr Array() = default;

        inline explicit Array(size_t size) 
            : data(size != 0 ? new T[size]() : nullptr, size != 0), length(size) { }

        inline constexpr Array(elf::Pointer<T[]>&& array, size_t length)
            : data(std::move(array)), length(length) { }

        inline explicit Array(T data[], size_t length, bool copyData = true)
            : data(copyData ? new T[length] : data, copyData)
        {
            if (copyData) this->copyData(data, length);
        }
        
        inline T& operator[] (size_t index) {
            return data[index];
        }
        inline const T& operator[] (size_t index) const {
            return data[index];
        }



    private:
        struct Iterator
        {
            friend Array<T>;

        private:
            T* array = nullptr;
            size_t count = 0;

            inline Iterator(const T* array, size_t count)
                : array(array), count(count) { }

        public:
            inline T& operator *() { return *array; }
            inline T& operator ->() { return *array; }
            inline Iterator& operator ++() { 
                array++; count--; if (count == 0) array = nullptr; return *this; 
            }

            inline bool operator ==(const Iterator& other) {
                return this->array == other.array;
            }
            inline bool operator !=(const Iterator& other) {
                return this->array != other.array;
            }

            using iterator_category = std::input_iterator_tag;
            using value_type        = T;
            using difference_type   = decltype(nullptr);
            using pointer           = T*;
            using reference         = T&;
        };

        struct ConstIterator
        {
            friend Array<T>;

        private:
            const T* array = nullptr;
            size_t count = 0;

            inline ConstIterator(const T* array, size_t count) 
                : array(array), count(count) { }

        public:
            inline const T& operator *() { return *array; }
            inline const T& operator ->() { return *array; }

            inline ConstIterator& operator ++() {
                array++; count--; if (count == 0) array = nullptr; return *this;
            }
            inline bool operator ==(const ConstIterator& other) {
                return this->array == other.array;
            }
            inline bool operator !=(const ConstIterator& other) {
                return this->array != other.array;
            }

            using iterator_category = std::input_iterator_tag;
            using value_type        = const T;
            using difference_type   = std::nullptr_t;
            using pointer           = const T*;
            using reference         = const T&;
        };

    public:
        inline Iterator begin() {
            return Iterator{ this->data.get(), this->length };
        }
        inline Iterator end() {
            return Iterator{ nullptr, 0 };
        }
        inline ConstIterator begin() const {
            return ConstIterator{ this->data.get(), this->length };
        }
        inline ConstIterator end() const {
            return ConstIterator{ nullptr, 0 };
        }
    };
}
