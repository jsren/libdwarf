#pragma once

#if __GNUC__

#define alloca __builtin_alloca
#define __pragma(x) 

#define packed_begin 
#define packed_end __attribute__((packed))

#else
#if _MSC_VER

#define _Static_assert(cond, msg) 

#define packed_begin __pragma(pack(push, 1));
#define packed_end ;__pragma(pack(pop))

#else

#error "Incompatible platform or compiler"

#endif
#endif
