/* platform.h - (c) James S Renwick 2016 */
#pragma once

#ifdef _MSC_VER

#define _pack_start __pragma(pack(push, 1))
#define _pack_end __pragma(pack(pop))

#else
#ifdef __GNUC__

#define _pack_start 
#define _pack_end __attribute__((packed))

#else
#error Unsupported platform or compiler
#endif
#endif
