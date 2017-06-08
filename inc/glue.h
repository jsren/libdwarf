#pragma once
#include "platform.h"

namespace glue
{
    template<class T>
    class IList
    {
    public:
        virtual void add(T item) = 0;
		
        virtual size_t count() const = 0;

        virtual T& operator[](size_t index) = 0;

		virtual const T& operator[](size_t index) = 0;

		static IList newInstance(size_t sizeHint);
	};
}

#include <glue-impl.h>
