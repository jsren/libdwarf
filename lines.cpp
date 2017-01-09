/* lines.cpp - (c) 2017 James S Renwick */
#include "lines.h"
#include "inc/format.h"
#include "inc/dwarf.h"

namespace dwarf2
{
    static size_t parseString(elf::Pointer<const char[]>& string, const uint8_t* buffer, bool copyData)
    {
        static_assert(sizeof(char) == sizeof(uint8_t), "Incompatible char width. Must be 1.");
        size_t length = strlen(reinterpret_cast<const char*>(buffer)) + 1;

        if (copyData)
        {
            auto* data = new char[length];
            memcpy(data, buffer, length);
            string.reset(data, true);
        }
        else string.reset(reinterpret_cast<const char*>(buffer), false);

        return length;
    }


    static size_t parseFileEntry(FileEntry& entry, const uint8_t* buffer, size_t bufferSize, bool copyData)
    {
        auto* bufferEnd = buffer + bufferSize;
        auto* cursor = buffer;

        cursor += parseString(entry.filepath, cursor, copyData); if (cursor >= bufferEnd) return 0;
        cursor += uleb_read(cursor, entry.includeDirIndex);      if (cursor >= bufferEnd) return 0;
        cursor += uleb_read(cursor, entry.lastModificationTime); if (cursor >= bufferEnd) return 0;
        cursor += uleb_read(cursor, entry.filesize);             if (cursor > bufferEnd) return 0;
        
        return static_cast<size_t>(cursor - buffer);
    }


    error_t LineNumberProgramHeader32::parse(const uint8_t* buffer, size_t bufferSize, 
        LineNumberProgramHeader32& program_out, size_t& length_out, bool copyData)
    {
        const uint8_t* cursor = buffer;
        const auto* bufferEnd = buffer + bufferSize;
        length_out = 0;

        // Check minimum buffer size
        if (bufferSize < 15) return -1;

        // The first few fields require nothing but a block copy
        memcpy(&program_out, cursor, 15);

        // Make unit length the size of entire unit
        if (program_out.unitLength <= (static_cast<uint32_t>(-1) - 4)) {
            program_out.unitLength += 4;
        } else return -2;

        // Make header length the size of entire header
        if (program_out.headerLength <= (static_cast<uint32_t>(-1) - 10)) {
            program_out.headerLength += 10;
        } else return -2; 

        length_out += 15;

        // Check size
        if (program_out.unitLength > bufferSize) return -1;

        // Parse the opcode length array
        cursor = buffer + length_out;
        program_out.opcodeLengths.length = program_out.specialOpcodeBase - 1;

        if (copyData)
        {
            auto* data = new uint8_t[program_out.opcodeLengths.length];
            memcpy(data, cursor, program_out.opcodeLengths.length);
            program_out.opcodeLengths.data.reset(data, true);
        }
        else program_out.opcodeLengths.data.reset(cursor, true);

        // Check buffer length
        length_out += program_out.opcodeLengths.length;
        cursor     += program_out.opcodeLengths.length;
        if (length_out >= bufferSize) return -1;

        // Count the number of include dirs
        size_t entryCount = 0;
        uint8_t prevChar = 0;

        size_t i;
        for (i = 0; cursor + i < bufferEnd; i++)
        {
            if (cursor[i] == '\0') {
                if (prevChar == '\0') break; else entryCount++;
            }
            prevChar = cursor[i];
        }
        if (cursor + i >= bufferEnd) return -1;

        // Parse the include dirs
        auto* data = new LineNumberProgramHeader32::String[entryCount];
        
        for (size_t i = 0; i < entryCount; i++) {
            cursor += parseString(data[i], cursor, copyData);
        }
        cursor += 1; // Skip list null terminator
        program_out.includeDirs.data.reset(data, true);
        program_out.includeDirs.length = entryCount;

        // Update parse length
        length_out = static_cast<size_t>(cursor - buffer);

        // File entries field is optional - handle not-present case
        if (*cursor == 0) {
            program_out.fileEntries.length = 0;
            program_out.fileEntries.data.reset();
            length_out++;
        }
        else
        {
            // Count the file entries
            FileEntry finalEntry;
            size_t entryCount = 0;

            auto* tmp_cursor = cursor;
            while (tmp_cursor[0] != 0)
            {
                auto len = parseFileEntry(finalEntry, tmp_cursor, bufferEnd - tmp_cursor, false);
                if (len == 0) break; else { tmp_cursor += len; entryCount++; }
            }

            // Parse the file entries
            auto* data = new FileEntry[entryCount];

            for (size_t i = 0; i < entryCount; i++) {
                cursor += parseFileEntry(data[i], cursor, bufferEnd - cursor, copyData);
            }
            program_out.fileEntries.data.reset(data, true);
            program_out.fileEntries.length = entryCount;

            // Update parse length;
            length_out += static_cast<size_t>(tmp_cursor - cursor);
        }

        return 0;
    }
}
