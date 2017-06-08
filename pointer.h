/* pointer.h - (c) 2017 James S Renwick
   ------------------------------------
   Authors: James S Renwick

   Custom pointer classes defining pointers 
   with optional runtime data ownership.
*/
#pragma once
#include <memory>

namespace elf
{
    
    template<typename T, typename D = std::default_delete<T>>
    class Pointer : private std::unique_ptr<T>
    {
    private:
        bool hasOwnership = false;

    public:
        using pointer = typename std::unique_ptr<T>::pointer;

        inline constexpr Pointer() = default;
        inline constexpr Pointer(nullptr_t _) : std::unique_ptr<T>(_) { }
        inline constexpr explicit Pointer(pointer p, bool owns) : std::unique_ptr<T>(p), hasOwnership(owns) { }

        inline constexpr explicit Pointer(std::unique_ptr<T>&& p) :
            std::unique_ptr<T>(std::move(p)), hasOwnership(true) { }

        inline Pointer(Pointer&& u) : std::unique_ptr<T>(std::move(u)) { }
        template<typename U>
        Pointer(Pointer<U>&& u) : std::unique_ptr<T>(std::move(u)) { };

        inline ~Pointer()
        {
            if (!hasOwnership) this->release();
            // Manual destructor call as destructor is not virtual
            std::unique_ptr<T>::~unique_ptr<T>();
        }

        inline Pointer& operator=(Pointer&& r)
        {
            if (!hasOwnership) this->release();
            std::unique_ptr<T>::operator=(std::move(r));
            hasOwnership = r.hasOwnership;
            return *this;
        }
        template<class U, class E>
        inline Pointer& operator=(Pointer<U, E>&& r)
        {
            if (!hasOwnership) this->release();
            std::unique_ptr<T>::operator=(std::move(r));
            hasOwnership = r.hasOwnership;
            return *this;
        }
        inline Pointer& operator=(std::unique_ptr<T>&& r)
        {
            if (!hasOwnership) this->release();
            std::unique_ptr<T>::operator=(std::move(r));
            hasOwnership = true;
            return *this;
        }
        template<class U, class E>
        inline Pointer& operator=(std::unique_ptr<U, E>&& r)
        {
            if (!hasOwnership) this->release();
            std::unique_ptr<T>::operator=(std::move(r));
            hasOwnership = true;
            return *this;
        }

    public:
        inline void reset()
        {
            if (!hasOwnership) std::unique_ptr<T>::release();
            std::unique_ptr<T>::reset();
        }
        inline void reset(pointer ptr, bool ownsPointer)
        {
            if (!hasOwnership) std::unique_ptr<T>::release();
            std::unique_ptr<T>::reset(ptr);
            this->hasOwnership = ownsPointer;
        }

        template<class = std::enable_if_t<std::is_copy_constructible<T>::value>>
        inline std::unique_ptr<T> to_unique_ptr() && {
            auto ptr = std::unique_ptr<T>(this->release());
            return hasOwnership ? ptr : std::unique_ptr<T>(new T(*ptr));
        }

    public:
        using std::unique_ptr<T>::get;
        using std::unique_ptr<T>::release;
        using std::unique_ptr<T>::operator bool;
        using std::unique_ptr<T>::operator *;
        using std::unique_ptr<T>::operator ->;
    };



    template<typename T, typename D>
    class Pointer<T[], D> : private std::unique_ptr<T[], D>
    {
    private:
        bool hasOwnership = false;

    public:
        using pointer = typename std::unique_ptr<T[]>::pointer;

        inline constexpr Pointer() = default;
        inline constexpr Pointer(nullptr_t _) : std::unique_ptr<T[]>(_) { }
        inline explicit Pointer(pointer p, bool owns) : std::unique_ptr<T[]>(p), hasOwnership(owns) { }

        inline explicit Pointer(std::unique_ptr<T[]>&& p) :
            std::unique_ptr<T[]>(std::move(p)), hasOwnership(true) { }

        inline Pointer(Pointer<T[], D>&& u) : std::unique_ptr<T[]>(std::move(u)) { }
        template<typename U>
        Pointer(Pointer<U[]>&& u) : std::unique_ptr<T[]>(std::move(u)) { };

        inline ~Pointer()
        {
            if (!hasOwnership) 
				this->release();
            // Manual destructor call as destructor is not virtual
            //std::unique_ptr<T[]>::~unique_ptr<T[]>();
        }

        inline Pointer& operator=(Pointer&& r)
        {
            if (!hasOwnership) this->release();
            std::unique_ptr<T[]>::operator=(std::move(r));
            hasOwnership = r.hasOwnership;
            return *this;
        }
        /*template<class U, class E>
        inline Pointer& operator=(Pointer<U[], E>&& r)
        {
            this->reset(r.release(), static_cast<Pointer<U[], E>>(r).hasOwnership);
            return *this;
        }*/
        inline Pointer& operator=(std::unique_ptr<T[]>&& r)
        {
            if (!hasOwnership) this->release();
            std::unique_ptr<T[]>::operator=(std::move(r));
            hasOwnership = true;
            return *this;
        }
        template<class U, class E>
        inline Pointer& operator=(std::unique_ptr<U[], E>&& r)
        {
            if (!hasOwnership) this->release();
            std::unique_ptr<T[]>::operator=(std::move(r));
            hasOwnership = true;
            return *this;
        }

    public:
        inline void reset()
        {
            if (!hasOwnership) std::unique_ptr<T[]>::release();
            std::unique_ptr<T[]>::reset();
        }
        inline void reset(pointer ptr, bool ownsPointer)
        {
            if (!hasOwnership) this->release();
            std::unique_ptr<T[]>::reset(ptr);
            this->hasOwnership = ownsPointer;
        }

        inline bool ownsData() const {
            return this->hasOwnership;
        }

    public:
        using std::unique_ptr<T[]>::get;
        using std::unique_ptr<T[]>::release;
        //using std::unique_ptr<T[]>::operator bool;
        using std::unique_ptr<T[]>::operator [];
    };


    template<typename T>
    inline auto make_Pointer(T* pointer) {
        return Pointer<T>(pointer, false);
    }
    template<typename T, typename ...Args>
    inline auto make_unique_Pointer(Args&&... args) {
        return Pointer<T>(new T(std::forward<Args>(args)...), true);
    }


}
