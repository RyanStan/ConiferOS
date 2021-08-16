#include "memory/paging/paging.h"
#include "memory/heap/kernel_heap.h"
#include "status.h"

// instead of paging_new_4gb it seems much cleaner to just have an initialize paging function 
// paging_4gb_chunk seems like a weird way of doing it
// maybe I can refer back to how Linux does it.. (each process struct has a pointer to pgd)

// since we're not using pae, we'll only be able to address 4gb physical memory
// want to look at linux kernel code to see how page tables are initialized there... they use internal heap allocation functions too?

static uint32_t* current_pgd = 0;

void paging_load_pgd(uint32_t* pgd);

struct paging_desc* init_page_tables(uint8_t flags)
{
        /* allocate the page global directory */
        uint32_t* pgd = kzalloc(sizeof(uint32_t) * PAGING_DIR_ENTRIES);

        int offset = 0;
        for (int i = 0; i < PAGING_DIR_ENTRIES; i++) {

                /* allocate a page table*/
                uint32_t* pte = kzalloc(sizeof(uint32_t) * PAGING_TABLE_ENTRIES);

                /* Fill each entry in the page table with an address to somewhere in our 4 gb space */
                for (int b = 0; b < PAGING_TABLE_ENTRIES; b++) {
                        pte[b] = (offset + (b * PAGING_PAGE_SIZE)) | flags;
                }
                offset += PAGING_TABLE_ENTRIES * PAGING_PAGE_SIZE;

                /* Fill in the pgd entry corresponding with the page table */
                pgd[i] = (uint32_t)pte | flags | PAGING_READ_WRITE;
        }

        struct paging_desc* paging = kzalloc(sizeof(struct paging_desc));
        paging->pgd = pgd;
        return paging;
}

uint32_t* get_pgd(struct paging_desc* paging)
{
        return paging->pgd;
}

void paging_switch(uint32_t* pgd)
{
        paging_load_pgd(pgd);
        current_pgd = pgd;
}

bool paging_is_aligned(void *addr)
{
        return (uint32_t)addr % PAGING_PAGE_SIZE == 0;
}

int paging_get_indexes(void *virtual_address, uint32_t *pgd_index_out, uint32_t *table_index_out)
{
        if (!paging_is_aligned(virtual_address)) {
                return -EINVARG;
        }

        /* TODO: Could also get the following two values by looking at the relevant 10 bits of virtual address (I think this would be better code) */
        *pgd_index_out = (uint32_t)virtual_address / (PAGING_TABLE_ENTRIES * PAGING_PAGE_SIZE);           
        *table_index_out = (uint32_t)virtual_address % (PAGING_TABLE_ENTRIES * PAGING_PAGE_SIZE) / PAGING_PAGE_SIZE;

        return 0;
}

int paging_set(uint32_t *pgd, void *virtual_address, uint32_t val)
{
        if (!paging_is_aligned(virtual_address)) {
                return -EINVARG;
        }

        uint32_t pgd_index = 0;
        uint32_t table_index = 0;
        int rc = paging_get_indexes(virtual_address, &pgd_index, &table_index);
        if (rc < 0) {
                return rc;
        }

        uint32_t pgd_entry = pgd[pgd_index];
        uint32_t *table = (uint32_t*)(pgd_entry & PGD_ENTRY_TABLE_ADDR);
        table[table_index] = val;

        return 0;
}


// need to implement past 18 minute mark
// my question: we have one pgd with 1024 entries and each of those entries points to a page table.  Thus, we have 1024 * 1024 page table entries, or 1,048,576 pages.
//              This indicates we have access to 4,294,967,296 bytes, or 0x1 0000 0000.  This adds up to 4 gb, all is good
// need to review setting up the stack frame in assembly. forgot what ebp and esp are.  e.g. what is the point of push ebp and then mov ebp, esp