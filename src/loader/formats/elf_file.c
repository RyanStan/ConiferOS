#include "elf_file.h"
#include "elf.h"
#include "fs/file.h"
#include <stdbool.h>
#include "memory/memory.h"
#include "memory/heap/kernel_heap.h"
#include "status.h"
#include "print/print.h"

// Every ELF file starts with these four bytes.
const char elf_signature[] = {0x7F, 'E', 'L', 'F'};

///////////////////////////////////////////////////////////////////
// UTILITY FUNCTIONS FOR PARSING THE ELF FILE

// Returns true if the data in buffer starts with the ELF signature
static bool elf_valid_signature(void *buffer)
{
    return memcmp(buffer, (void *)elf_signature, sizeof(elf_signature)) == 0;
}

// Returns true if e_ident[EI_CLASS] is ELFCLASSNONE or ELFCLASS32. False otherwise.
static bool elf_valid_class(struct elf32_ehdr *header)
{
    return header->e_ident[EI_CLASS] == ELFCLASSNONE || header->e_ident[EI_CLASS] == ELFCLASS32;
}

// Returns true if e_ident[EI_DATA] is ELFDATANONE or ELFDATA2LSB. False otherwise.
// x86 processor's use little-endian byte ordering.
static bool elf_valid_encoding(struct elf32_ehdr *header)
{
    return header->e_ident[EI_DATA] == ELFDATANONE || header->e_ident[EI_DATA] == ELFDATA2LSB;
}

static bool elf_is_executable(struct elf32_ehdr *header)
{
    return header->e_type == ET_EXEC;
}

/* Returns true if the entry point virtual address
 * equals the virtual memory address that we load task executable code into.
 *
 * It's not required that we fix the virtual address entry point of ELF programs (Linux kernel doesn't do this).
 * However, it makes life easier for now, in case the kernel needs to make certain data available to user land and
 * maps it into the user process's virtual address space.
 */
static bool elf_valid_entry(struct elf32_ehdr *header)
{
    return header->e_entry == TASK_LOAD_VIRTUAL_ADDRESS;
}

struct elf32_ehdr *elf_get_ehdr(struct elf_file *elf_file)
{
    if (!elf_file->elf_file_buffer)
        return 0;
    
    return (struct elf32_ehdr *)elf_file->elf_file_buffer;
}

static bool elf_has_program_header_table(struct elf32_ehdr *header)
{
    return header->e_phoff != 0;
}

// Return true if the ELF file can be loaded by our ELF loader.
bool valid_elf_file(struct elf32_ehdr *header)
{
    return elf_valid_signature(header) && elf_valid_class(header) && elf_valid_encoding(header) && elf_is_executable(header)
        && elf_has_program_header_table(header);
}

// Returns a pointer to the section header table
struct elf32_shdr *elf_get_shdr_table(struct elf32_ehdr *header)
{
    struct elf32_shdr *shdr_table = (struct elf32_shdr *)((char *)header + header->e_shoff);
    return shdr_table;
}

// Returns the section header at the specified index in the section header table.
struct elf32_shdr *elf_get_shdr(struct elf32_ehdr *header, unsigned int index)
{
    struct elf32_shdr *shdr = elf_get_shdr_table(header);
    return &shdr[index];
}

struct elf32_phdr *elf_get_phdr_table(struct elf32_ehdr *header)
{
    if (!elf_has_program_header_table(header))
        return 0;

    struct elf32_phdr *phdr_table = (struct elf32_phdr *)((char *)header + header->e_phoff);
    return phdr_table;
}

struct elf32_phdr *elf_get_phdr(struct elf32_ehdr *header, unsigned int index)
{
    struct elf32_phdr *phdr = elf_get_phdr_table(header);
    return &phdr[index];
}

// Returns a character pointer to the string table.
char *elf_get_string_table(struct elf32_ehdr *header)
{
    struct elf32_shdr *string_table_section = elf_get_shdr(header, header->e_shstrndx);     // SHT_STRTAB
    return (char *)header + string_table_section->sh_offset;
}

///////////////////////////////////////////////////////////////////

void *elf_file_get_segment_phys_addr(struct elf_file *elf_file, struct elf32_phdr *elf32_phdr)
{
    return elf_file->elf_file_buffer + elf32_phdr->p_offset;
}

/* Process a program header of type PT_LOAD (loadable segment)
 * Expects that PT_LOAD segments are contiguous and that the first segment is executable + contains the code section.
 * 
 * This function calculates the virtual address and physical addresses that the ELF file's loadable segments will be loaded into.
 * and updates the elf_file structure accordingly.
 */
int elf_process_pheader_pt_load(struct elf_file *elf_file, struct elf32_phdr *elf32_phdr)
{
    // In Linux, loadable segments are mapped into a process's memory here: https://elixir.bootlin.com/linux/v3.18/source/fs/binfmt_elf.c#L816

    if (elf32_phdr->p_type != PT_LOAD)
        return -EINVARG;

    if (elf_file->elf_virtual_addr_base >= (void *)elf32_phdr->p_vaddr || elf_file->elf_virtual_addr_base == 0x00) {
        elf_file->elf_virtual_addr_base = (void *)elf32_phdr->p_vaddr;
        elf_file->elf_phys_addr_base = elf_file->elf_file_buffer + elf32_phdr->p_offset;
    }

    unsigned int end_virt_addr = elf32_phdr->p_vaddr + elf32_phdr->p_filesz;
    if (elf_file->elf_virtual_addr_end <= (void *)end_virt_addr || elf_file->elf_virtual_addr_end == 0x00) {
        elf_file->elf_virtual_addr_end = (void *)end_virt_addr;
        elf_file->elf_phys_addr_end = elf_file->elf_file_buffer + elf32_phdr->p_offset + elf32_phdr->p_filesz;
    }

    return 0;
}

int elf_process_pheader(struct elf_file *elf_file, struct elf32_phdr *elf32_phdr)
{
    int rc = 0;

    // For now, we are only processing loadable segments
    switch (elf32_phdr->p_type) {
    case PT_LOAD:
        rc = elf_process_pheader_pt_load(elf_file, elf32_phdr);
        if (rc < 0) {
            print("ERROR: elf_process_pheader: Error processing PT_LOAD segment\n");
            return rc;
        }
        break;
    default:
        break;
    }

    return rc;
}

int elf_process_pheaders(struct elf_file *elf_file)
{
    struct elf32_ehdr *elf32_ehdr = elf_get_ehdr(elf_file);
    for (int i = 0; i < elf32_ehdr->e_phnum; i++) {
        struct elf32_phdr *elf32_phdr = elf_get_phdr_table(elf32_ehdr);
        int rc = elf_process_pheader(elf_file, elf32_phdr);
        if (rc < 0) {
            print("ERROR: elf_process_pheaders: Failed to process a program header\n");
        }
    }
    return 0;
}

int process_elf_file(struct elf_file *elf_file)
{
    // Validate that the elf file has been loaded into a buffer
    // so that we can parse it.
    if (!elf_file->elf_file_buffer)
        return -EINVARG;

    struct elf32_ehdr *elf32_ehdr = elf_get_ehdr(elf_file);
    if (!valid_elf_file(elf32_ehdr)) {
        print("ERROR: process_elf_file: Invalid ELF file\n");
        return -EIFORMAT;
    }

    int rc = elf_process_pheaders(elf_file);
    if (rc < 0) {
        return rc;
    }
    
    return 0;
}

int elf_file_init(const char *elf_filename, struct elf_file **elf_file_out)
{
    // Life would be much easier if we had a `mmap` function which maps files into memory

    struct elf_file *elf_file = kzalloc(sizeof(struct elf_file));
    if (!elf_file)
        return -ENOMEM;

    int fd = fopen(elf_filename, "r");
    if (fd < 0) {
        kfree(elf_file);
        return fd;
    }

    struct file_stat stat;
    int rc = fstat(fd, &stat);
    if (rc < 0) {
        kfree(elf_file);
        return rc;
    }

    elf_file->elf_file_buffer = kzalloc(stat.filesize);
    rc = fread(elf_file->elf_file_buffer, stat.filesize, 1, fd);
    if (rc != 1) {
        print("elf_file_init: error reading elf file into elf_file_buffer\n");
        kfree(elf_file->elf_file_buffer);
        kfree(elf_file);
        return -EINVARG;
    }

    rc = process_elf_file(elf_file);
    if (rc < 0) {
        kfree(elf_file->elf_file_buffer);
        kfree(elf_file);
        fclose(fd);
        return rc;
    }

    *elf_file_out = elf_file;
    fclose(fd);
    return 0;
}

void elf_file_close(struct elf_file *elf_file)
{
    if (!elf_file)
        return;

    kfree(elf_file->elf_file_buffer);
    kfree(elf_file);
}