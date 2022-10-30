#ifndef TSS_H
#define TSS_H

/* We'll be using task state segments to enter ring 0 (kernel mode) after
 * an interrupt.  
 */

#include <stdint.h>
struct tss {
    /* TODO: make note of which ones of these are unused by us */
    uint32_t prev_task_link;    /* Segment selector for the TSS of the previous task. Used by IRET. */
    uint32_t esp0;              /* Kernel stack pointer.  This is loaded into esp when a switch occurs. */
    uint32_t ss0;               /* Kernel stack segment selector (index into gdt) */
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt_seg_selector;
    uint32_t io_map_base_addr;
    uint32_t ssp;
} __attribute__((packed));

/* Load tss_seg_selector into segment selector field of task register.
 * tss_seg_selector is offset into gdt of a tss descriptor.
 */
void tss_load(int tss_seg_selector);

#endif