#pragma once
#include <const.h>

namespace dwarf
{

#ifdef DWARF_x64
    using Int  = int64_t;
    using UInt = uint64_t;
#else
    using Int  = int32_t;
    using UInt = uint32_t;
#endif


    enum class ObjectFileSection
    {
        debug_info,
        debug_abbrev,
        debug_aranges,
        debug_ranges,
        debug_line,
        debug_str
    };

    struct ObjectFileSectionInfo
    {
    public:
        dwarf::UInt offset;
        dwarf::UInt size;
    };


    class ObjectFile
    {
    public:
        ObjectFile() = default;
        virtual ~ObjectFile() = default;

        ObjectFile(ObjectFile &&)      = delete;
        ObjectFile(const ObjectFile &) = delete;

    public:
        virtual dwarf::Int read(uint8_t *buffer, dwarf::UInt startIndex, dwarf::UInt length) noexcept(false) = 0;
        virtual ObjectFileSectionInfo getSectionInfo(ObjectFileSection section) noexcept(false) = 0;
    };

    struct Context
    {
    public:

    };
}