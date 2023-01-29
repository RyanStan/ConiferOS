#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stdbool.h>


/* Bitmasks for page table and page directory entries */
#define PAGING_CACHE_DISABLE    0b00010000              // PCD
#define PAGING_WRITE_THROUGH    0b00001000              // PWT
#define PAGING_USER_SUPERVISOR  0b00000100              // Privilege level required to access page. 0 = CPL must be < 3 (kernel mode).
#define PAGING_READ_WRITE       0b00000010              // Access right. 0 = can only be read. 1 = read and written.
#define PAGING_PRESENT          0b00000001              // If set, the page is in main memory.
#define PGD_ENTRY_TABLE_ADDR    0xfffff000              
#define PTE_PAGE_FRAME_ADDR     0xfffff000

/* the page directory and page tables will each have 1024 entries (covers 4 gb address space) */
#define PAGING_TABLE_ENTRIES    1024                    
#define PAGING_DIR_ENTRIES      1024

#define PAGING_PAGE_SIZE        4096

/*
 * In 32-bit x86, a two level paging scheme is used.
 * Thus, a linear address is split into 3 pieces.
 * Bits 31 - 22 (inclusive), index the page directory.
 * Bits 21 - 12 index the page table.
 * Bits 11 - 0 index the byte within the page. 
 */

/* Since our system is 32 bits without PAE, we'll only have access to a 4 gb address space 
 * 
 */
struct paging_desc {
        /* Pointer to the first entry in the page global directory. 
         * TODO: this should be page directory, not page global directory (pgd for 64 bit systems)
         */
        uint32_t* pgd;                                 
};

/* Initializes a page global directory and the corresponding page tables.
 * The page tables are initialized so that there is a linear, 1:1 correlation between
 * virtual addresses and physical addresses.
 */
struct paging_desc* init_page_tables(uint8_t flags);

/* Returns the page global directory associated with the paging descriptor */
uint32_t* get_pgd(struct paging_desc* paging);

/* Load the cr3 register with the address of the page global directory to use */
void paging_switch(struct paging_desc *paging_desc);

/* Set the paging bit in the cr0 register 
 * 
 * Prereqs: called init_paging and paging_switch
 */
void enable_paging();

/* Determines which page global directory entry and corresponding table entry are responsible for the virtual address  
 * Sets directory_index_out and table_index_out as a side effect of function call 
 */
int paging_get_indexes(void *virtual_address, uint32_t *pgd_index_out, uint32_t *table_index_out);

/* Set the virtual address's corresponding page table entry to the specified value */
int paging_set(uint32_t *pgd, void *virtual_address, uint32_t val);

/* Returns true if addr is aligned to page boundary, false otherwise */
bool paging_is_aligned(void *addr);

/* Free the memory allocated for the page tables associated with paging's page tables */
void free_page_tables(struct paging_desc *paging);

/* Create a mapping in the page tables at paging_desc.
 * The virtual address space starting at virt_addr will map to the physical address space
 * starting at phys_addr and will map num_pages pages. 
 */
int paging_map_range(struct paging_desc *paging_desc, void *virt_addr, void *phys_addr, int num_pages, int pg_prot);

/* Create a mapping in the page tables at paging_desc.
 * The virtual address space starting at virt_addr will map to the physical address phys_addr to phys_end_addr.
 * The page protection flags, pg_prot, describe the flags that are applied to the page table entries for this mapping.
 * This function expects all the arguments to be properly page aligned. 
 */
int paging_create_mapping(struct paging_desc *paging_desc, void *virt_addr, void *phys_addr, void *phys_end_addr, int pg_prot);

/* Rounds addr up so that it is page aligned */
void *paging_align_address(void *addr);

#endif