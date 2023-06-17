#ifndef ELF_FILE_H
#define ELF_FILE_H

#include "config.h"

/*
 * The Linux Kernel code that loads ELF files is at linux/fs/binfmt_elf.c.
 */

struct elf_file {
    char filename[MAX_FILE_PATH_CHARS];

    // Size of the ELF file in memory
    int in_memory_size;

    // Once the ELF file is loaded into the kernel task's memory as a contiguous set of bytes, 
    // this will point to the file in memory.
    void *elf_file_buffer;

    /* Currently, our ELF Loader only supports executable files (ET_ELF)
     * and only parses loadable segments (PT_LOAD).
     * It also expects that the loadable segments are contiguous, and that 
     * the first loadable segment is executable and contains the program's code/text.
     * 
     * These variables hold the virtual address range where the loadable segments will be mapped into a user process's 
     * address space.
     * 
     * Much like how the stack starts at a high address and grows downward, the end of this range has a lower
     * address than the base.
     */
    void *elf_virtual_addr_base;
    void *elf_virtual_addr_end;

    /* These variables hold the physical address range that the loadable
     * segments have been mapped into. The Kernel Page Tables have a 1:1 address mapping with physical memory,
     * so kernel processes can use these addresses to touch the loadable segments.
     * 
     * We don't set these values to the physical addresses given to us
     * in the ELF file because our operating system makes it's own decision about
     * which physical memory address to load these ELF files into.
     */
    void *elf_phys_addr_base;
    void *elf_phys_addr_end;
};

/* 
 * Initializes an in-memory elf_file data structure that corresponds to an ELF file from the filesystem.
 *
 * This function allocates the elf_file structure and sets the value pointed to by elf_file_out
 * to the address of the elf_file structure.
 * 
 * Note: this function will not load the ELF file's loadable segments into the address space of the current process.
 */
int elf_file_init(const char *elf_filename, struct elf_file **elf_file_out);

// Free the memory associated with the elf_file structure
void elf_file_close(struct elf_file *elf_file);

/* Returns a pointer to the ELF header of the ELF file.
 * Prerequisite: The ELF file must have been loaded into memory (elf_file_buffer).
 */
struct elf32_ehdr *elf_get_ehdr(struct elf_file *elf_file);

// Returns a pointer to the program header table
struct elf32_phdr *elf_get_phdr_table(struct elf32_ehdr *header);

// Returns the program header at the specified index in the program header table.
struct elf32_phdr *elf_get_phdr(struct elf32_ehdr *header, unsigned int index);

// Returns the physical address of the segment associated with the program header elf32_phdr.
void *elf_file_get_segment_phys_addr(struct elf_file *elf_file, struct elf32_phdr *elf32_phdr);

#endif