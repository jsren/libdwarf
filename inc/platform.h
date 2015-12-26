#pragma once

#if __GNUC__

#define alloca __builtin_alloca
#define __pragma(x) 

#else
#if _MSC_VER

#define inline __inline
#define _Static_assert(cond, msg) 

#else

#error "Incompatible platform or compiler"

#endif
#endif
