#include "memory/paging/paging.h"
#include "memory/heap/kernel_heap.h"
#include "status.h"

/* Size of memory that we can reach via paging:
 * We have one pgd with 1024 entries and each of those entries points to a page table.  Thus, we have 1024 * 1024 page table entries, or 1,048,576 pages.
 * We're using 4 KB pages, so this indicates we have access to 4,294,967,296 bytes, or 0x1 0000 0000.  This adds up to 4 GiB.
 */

#define PAGE_TABLE_ENTRY_FLAGS_MASK 0x00000FFF  /* The last 12 bits of a page table entry are just flags, since pages are aligned at 4 MB, or 2^12 */

static uint32_t* current_pgd = 0;

void paging_load_pgd(uint32_t* pgd);

/* Create a direct mapping between linear and physical addresses */
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

void paging_switch(struct paging_desc *paging_desc)
{
        paging_load_pgd(paging_desc->pgd);
        current_pgd = paging_desc->pgd;
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

void free_page_tables(struct paging_desc *paging)
{
        for (int i = 0; i < PAGING_DIR_ENTRIES; i++) {
                uint32_t pgd_entry = paging->pgd[i];
                uint32_t *page_table = (uint32_t*)(pgd_entry * ~PAGE_TABLE_ENTRY_FLAGS_MASK);
                kfree(page_table);
        }

        kfree(paging->pgd);
        kfree(paging);
}

int paging_map_range(struct paging_desc *paging_desc, void *virt_addr, void *phys_addr, int num_pages, int pg_prot)
{
    if (!paging_is_aligned(virt_addr) || !paging_is_aligned(phys_addr))
        return -EINVARG;

    void *virt_addr_copy = virt_addr;
    void *phys_addr_copy = phys_addr;

    int rc = 0;
    for (int i = 0; i < num_pages; i++) {
        rc = paging_set(paging_desc->pgd, virt_addr_copy, (uint32_t) phys_addr_copy | pg_prot);
        if (rc < 0)
            break;
        
        virt_addr_copy += PAGING_PAGE_SIZE;
        phys_addr_copy += PAGING_PAGE_SIZE;
    }

    return rc;
}

int paging_create_mapping(struct paging_desc *paging_desc, void *virt_addr, void *phys_addr, void *phys_end_addr, int pg_prot)
{
    if (!paging_is_aligned(virt_addr) || !paging_is_aligned(phys_addr) || !paging_is_aligned(phys_end_addr))
        return -EINVARG;

    if ((uint32_t)phys_end_addr < (uint32_t)phys_addr)
        return -EINVARG;

    uint32_t total_bytes = (uint32_t)phys_end_addr - (uint32_t)phys_addr;
    int total_pages = total_bytes / PAGING_PAGE_SIZE;
    return paging_map_range(paging_desc, virt_addr, phys_addr, total_pages, pg_prot);
}

void *paging_align_address(void *addr)
{
    if (paging_is_aligned(addr))
        return addr;

    return (void*)((uint32_t)addr + PAGING_PAGE_SIZE - ((uint32_t)addr % PAGING_PAGE_SIZE));
}

uint32_t paging_get_pte(uint32_t *task_page_directory, void *virt_addr)
{
        uint32_t directory_index = 0;
        uint32_t table_index = 0;
        int rc = paging_get_indexes(virt_addr, &directory_index, &table_index);
        if (rc < 0) {
                return rc;
        }

        uint32_t page_directory_entry = task_page_directory[directory_index];
        uint32_t *page_table = (uint32_t*)(page_directory_entry & PGD_ENTRY_TABLE_ADDR);
        return page_table[table_index];
}