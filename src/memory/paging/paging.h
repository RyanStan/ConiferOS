#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>


/* Bitmasks for page table and page directory entries*/
#define PAGING_CACHE_DISABLE    0b00010000              // PCD
#define PAGING_WRITE_THROUGH    0b00001000              // PWT
#define PAGING_USER_SUPERVISOR  0b00000100
#define PAGING_READ_WRITE       0b00000010
#define PAGING_PRESENT          0b00000001

/* the page directory and page tables will each have 1024 entries (covers 4 gb address space) */
#define PAGING_TABLE_ENTRIES    1024                    
#define PAGING_DIR_ENTRIES      1024

#define PAGING_PAGE_SIZE        4096

/* Since our system is 32 bits without PAE, we'll only have access to a 4 gb address space 
 * TODO: bad code smell, don't really like this struct.
 */
struct paging_desc {
        /* page global directory.  each directory entry points to a page table */
        uint32_t* pgd;                                 
};

/* Initializes a page global directory and the corresponding page tables.
 * The page tables are initialized so that there is a linear, 1:1 correlation between
 * virtual addresses and physical addresses.
 */
struct paging_desc* init_page_tables(uint8_t flags);

/* Returns the page global directory associated with the paging descriptor */
uint32_t* get_pgd(struct paging_desc* paging);

/* Load the cr3 register with the address of the page global directory to use*/
void paging_switch(uint32_t* pgd);

/* Set the paging bit in the cr0 register 
 * 
 * Prereqs: called init_paging and paging_switch
 */
void enable_paging();

#endif