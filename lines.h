/* lines.h - (c) 2017 James S Renwick 
   ----------------------------------
   Authors: James S Renwick
*/
#pragma once

#include "pointer.h"
#include "array.h"
#include "inc/platform.h"

namespace dwarf2
{
    using namespace dwarf;
    typedef signed long int error_t;


    struct FileEntry
    {
        elf::Pointer<const char[]> filepath; // Full or relative path to the file
        uint64_t includeDirIndex;            // Index of file's include dir for relative paths
        uint64_t lastModificationTime;       // Implementation-defined last modification time or 0 if unknown
        uint64_t filesize;                   // Length in bytes of file, or 0 if unknown
    };

    struct LineNumberProgramHeader32
    {
        using String = elf::Pointer<const char[]>;

        _pack_start struct {
            uint32_t unitLength;                  // Size (bytes) of line no. unit
            uint16_t version;                     // Line number version number
            uint32_t headerLength;                // Size (bytes) of this header
            uint8_t  minInstructionLength;        // Size (bytes) of the smallest target machine instruction
            uint8_t  defaultIsStmt;               // Initial value of isStatement 'register'
            int8_t   lineBase;                    // Mimimum line number delta value
            uint8_t  lineRange;                   // Line number delta value range
            uint8_t  specialOpcodeBase;           // Numeric value of first special opcode
        } _pack_end;

        Array<const uint8_t>   opcodeLengths; // Number of operands for each standard opcode
        Array<String>          includeDirs;   // Directories searched for includes
        Array<const FileEntry> fileEntries;   // Source files which contribute line number info.
                                              // Points to nullptr if values given by DW_LNE_define_file instead.

        /* Parses the line number program header from the given buffer.

             program_out - the value to which to write the data
             length_out  - value to hold the number of bytes parsed
             copyData    - If true, assigns copies of original data to pointer fields.
                           Otherwise pointers point to original data.

           Returns the number of bytes consumed from the buffer.
        */
        static error_t parse(const uint8_t* buffer, size_t bufferSize,
            LineNumberProgramHeader32& program_out, size_t& length_out, bool copyData = true);

    };



    class LineNumberState32
    {
        uint32_t address;       // Current program instruction address
        uint32_t opIndex;       // Index of operation within Very Long Instruction Word instruction
        uint32_t fileIndex;     // Index of source file within file entry table
        uint32_t lineNumber;    // Current source file line (first index is 1)
        uint32_t columnNumber;  // Current source file column (first index is 1)
        bool     isStatement;   // Whether current instruction is a recommended breakpoint location
        bool     isBlockStart;  // Whether current instruction is the beginning of a basic block
        bool     isSequenceEnd; // Whether the current instruction follows the end of sequence of instructions
        bool     isBodyStart;   // Whether the current instruction is entry into a function body
        bool     isBodyEnd;     // Whether the current instruction is exit from a function body
        uint32_t isa;           // Implementation-defined value indicating specific Instruction Set Architecture
        uint32_t _;
    };

}

namespace dwarf4
{
    using namespace dwarf;
    typedef signed long int error_t;


    struct LineNumberProgramHeader32
    {
        using String = elf::Pointer<const char[]>;

        _pack_start struct {
            uint32_t unitLength;                  // Size (bytes) of line no. unit
            uint16_t version;                     // Line number version number
            uint32_t headerLength;                // Size (bytes) of this header
            uint8_t  minInstructionLength;        // Size (bytes) of the smallest target machine instruction
            uint8_t  maxOpsPerInstruction;        // Maximum number of individual operations in an instruction
            uint8_t  defaultIsStmt;               // Initial value of isStatement 'register'
            int8_t   lineBase;                    // Mimimum line number delta value
            uint8_t  lineRange;                   // Line number delta value range
            uint8_t  specialOpcodeBase;           // Numeric value of first special opcode
        } _pack_end;

        Array<const uint8_t>   opcodeLengths; // Number of operands for each standard opcode
        Array<String>          includeDirs;   // Directories searched for includes
        Array<const dwarf2::FileEntry> fileEntries;   // Source files which contribute line number info.
                                              // Points to nullptr if values given by DW_LNE_define_file instead.

        /* Parses the line number program header from the given buffer.

             program_out - the value to which to write the data
             length_out  - value to hold the number of bytes parsed
             copyData    - If true, assigns copies of original data to pointer fields.
                           Otherwise pointers point to original data.

           Returns the number of bytes consumed from the buffer.
        */
        static error_t parse(const uint8_t* buffer, size_t bufferSize,
            LineNumberProgramHeader32& program_out, size_t& length_out, bool copyData = true);

    };
}
