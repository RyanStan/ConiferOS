/* ELF File Specification:
 *  https://refspecs.linuxfoundation.org/elf/elf.pdf
 *
 * The comments in this file are taken straight out of the specification.
 * 
 * This file contains type definitions from the formal ELF specification.
 */
#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stddef.h>

// Segment Flag Bits, p_flags
#define PF_X            0x01
#define PF_W            0x02
#define PF_R            0x04

// Segment types, p_type
#define PT_NULL         0
#define PT_LOAD         1
#define PT_DYNAMIC      2
#define PT_INTERP       3
#define PT_NOTE         4
#define PT_SHLIB        5
#define PT_PHDR         6

// Section Types, sh_type
#define SHT_NULL        0
#define SHT_PROGBITS    1
#define SHT_SYMTAB      2
#define SHT_STRTAB      3
#define SHT_RELA        4
#define SHT_HASH        5
#define SHT_DYNAMIC     6
#define SHT_NOTE        7
#define SHT_NOBITS      8
#define SHT_REL         9
#define SHT_SHLIB       10
#define SHT_DYNSYM      11

// Object file type. These are possible values of e_type in the ELF header.
#define ET_NONE         0   // No file type
#define ET_REL          1   // Relocatable file
#define ET_EXEC         2   // Executable file
#define ET_DYN          3   // Shared object file
#define ET_CORE         4   // Core file

// e_ident[] Identification Indexes
#define EI_CLASS        4   // File class
#define EI_DATA         5   // Data encoding
#define EI_NIDENT       16  // Size of e_ident[]

// e_ident[EI_CLASS] values
#define ELFCLASSNONE    0   // Invalid claass
#define ELFCLASS32      1   // 32-bit objects
#define ELFCLASS64      2   // 64-bit objects

// e_ident[EI_DATA] values
// Specifies the data encoding of the processor-specific
// data in the object file.
#define ELFDATANONE     0   // Invalid data encoding
#define ELFDATA2LSB     1   // 2's complement values, LSB
#define ELFDATAMSB      2   // 2's complements values, MSB

/* SHN_UNDEF marks an undefined, missing,
 * irrelevant, or meaningless section reference.
 */
#define SHN_UNDEF       0

// ELF 32-bit data types
typedef uint32_t elf32_addr;    // Unsigned program address
typedef uint16_t elf32_half;    // Unsigned medium integer
typedef uint32_t elf32_off;     // Unsigned file offset
typedef int32_t elf32_sword;   // Signed large integer
typedef uint32_t elf32_word;    // Unsigned large integer

// ELF Header
struct elf32_ehdr {
    unsigned char e_ident[EI_NIDENT];
    elf32_half e_type;                  // Identifies object file type
    elf32_half e_machine;               // CPU architecture
    elf32_word e_version;               
    elf32_addr e_entry;                 // Virtual address to which the system first transfers control, thus starting the process
    elf32_off e_phoff;                  // The program header table's file offset in bytes
    elf32_off e_shoff;                  // The section header table's file offset in bytes
    elf32_word e_flags;                 // Processor specific flags associated with the file
    elf32_half e_ehsize;                // The ELF header's size in bytes
    elf32_half e_phentsize;             // The size in bytes of one entry in the file's program header table
    elf32_half e_phnum;                 // The number of entries in the program header table
    elf32_half e_shentsize;             // A section header's size in bytes
    elf32_half e_shnum;                 // The number of entries in the section header table
    elf32_half e_shstrndx;              // The section header table index of the entry associated with the section name string table
                                        // This table stores null terminated strings.
} __attribute__((packed));

/* A Program Header describes a segment. 
 * Program Headers are stored in the Program Header Table.
 * 
 * Segments contain information that is needed for the run-time execution
 * of a program.
 */
struct elf32_phdr {
    elf32_word p_type;              
    elf32_off p_offset;             // Offset from the beginning of the file at which the first byte of the segment resides
    elf32_addr p_vaddr;             // Virtual address at which the first byte of the segment should go in memory
    elf32_addr p_paddr;             // For systems where physical addressing is relevant. This is the physical address the segment will go in memory
    elf32_word p_filesz;            // Size of the segment in the ELF file
    elf32_word p_memsz;             // Size of the segment when it is loaded into memory
    elf32_word p_flags;             // Executable, writable, readable
    elf32_word p_align;             // Alignment re quires when loaded into memory
} __attribute__((packed));

/* Section Header
 * Sections contain linking information. A segment may contain sections.
 * Section headers are stored in the section header table.
 */
struct elf32_shdr {
    elf32_word sh_name;             // Name of the section. Value is an index into the section header string table
    elf32_word sh_type;             // Type of section
    elf32_word sh_flags;            // Write, Alloc, Execinstr
    elf32_addr sh_addr;             // If the section will appear in the memory image of a process, 
                                    //      this member gives the address at which the section's first byte should reside
    elf32_off sh_offset;            // The byte offset from the beginning of the file to the first byte in the section
    elf32_word sh_size;             // The section's size in bytes.
    elf32_word sh_link;             // Intepretation depends on the section type. Often used for linking to other sections
    elf32_word sh_info;             // Extra info. Depends on section type
    elf32_word sh_addralign;        // Alignment constraints
    elf32_word sh_entsize;          // For sections that hold a table of fixed-size entries, such as a symbol table,
                                    //      this member gives the size in bytes of each entry.
} __attribute__((packed));

/* If an object file participates in dynamic linking, its program header table will have an element
 * of type PT_DYNAMIC. This "segment" contains the .dynamic section.
 * 
 *  A special symbol, _DYNAMIC, labels the section, which contains an array of the following structures.
 */
struct elf32_dyn {
    elf32_sword d_tag;          // Defines the type of the array element
    union {
        elf32_word d_val;
        elf32_addr d_ptr;       // Program virtual addresses
    } d_un;
} __attribute__((packed));

/* Symbol table entry
 *
 * An object file's symbol table holds information needed to locate and relocate a program's
 * symbolic definitions and references.
 * 
 * There are two symbol tables, the main symbol table (SHT_SYMTAB section type)
 * and the dynamic symbol table (SHT_DYNSYM), which is a subset of entries in the main symbol table.
 * The dynamic symbol table contains only symbols that are needed at runtime. Thus, SHT_DYNSYM is allocable,
 * or mapped into memory and needed at runtime.
 * 
 * This structure defines an entry within the symbol table.
 */
struct elf32_sym {
    elf32_word st_name;         // Index into the object file's symbol string table, which holds
                                //      the character representations of the symbol names.
    elf32_addr st_value;        // The value of the symbol. The type of this value depend's on the type of elf file.
                                //      In relocatable files, this holds alighmnet constraints for a symbol whose section index is SHN_COMMON.
                                //      In relocatable files, st_value holds a section offset for a defined symbol. That is,
                                //          st_value is an offset from the beginning of the section that st_shndx identifies
                                //      In executable and shared object files, st_value holds a virtual address.
    elf32_word st_size;     
    unsigned char st_info;      // Specifies the symbol's type and binding attributes
    unsigned char st_other;     // Holds 0 and has no defined meaning
    elf32_half st_shndx;        // Every symbol table entry is "defined'' in relation to some section; this member holds
                                //      the relevant section header table index. 
} __attribute__((packed));

#define ELF32_ST_BIND(i) ((i)>>4)
#define ELF32_ST_TYPE(i) ((i)&0xf)
#define ELF32_ST_INFO(b,t) (((b)<<4)+((t)&0xf))

// Symbol Binding, ELF32_ST_BIND
#define STB_LOCAL   0               // Local symbols are not visible outside the object file containing their definition.
#define STB_GLOBAL  1               // Global symbols are visible to all object files being combined.
#define STB_WEAK    2               // Weak symbols resemble global symbols, but their definitions have lower precedence.
#define STB_LOPROC  13
#define STB_HIPROC  15

// Symbol types, ELF32_ST_TYPE
#define STT_NOTYPE      0           // The symbol's type is not specified.
#define STT_OBJECT      1           // The symbol is associated with a data object, such as a variable, an array, and so on.
#define STT_FUNC        2           // The symbol is associated with a function or other executable code.
#define STT_SECTION     3           // The symbol is associated with a section. Symbol table entries of this type
                                    //      exist primarily for relocation and normally have STB_LOCAL binding.
#define STT_FILE        4           // The file symbol has STB_LOCAL binding, its section index is SHN_ABS, and
                                    //      it precedes the other STB_LOCAL symbols for the file, if it is present
#define STT_LOPROC      13
#define STT_HIPROC      15

#endif