#ifndef GDT_H
#define GDT_H

#include <stdint.h>
#include "config.h"

struct segment_descriptor_raw {
    uint16_t segment_limit;     // The size of the segment. 
    uint16_t base_first;        // The location of byte 0 of the segment within the 4 GB linear address space
    uint8_t base;               
    uint8_t access;             // P (1 bit, msb). DPL (2 bits). S (1 bit). Type (4 bits, lsb).
                                // P: Indicates whether the segment is present in memory (set) or not present (clear).
                                // DPL: Specifies the privilege level of the segment and is used to control access.
                                // S: Specifies whether the segment descriptor is for a system segment (S flag is clear) or a code or data segment (S flag is set).
                                // Type: Indicates the segment or gate type and specifies the kinds of access that can be made to the segment and the direction of growth.
    uint8_t high_flags;         // G (1 bit, msb). D/B (1 bit). L (1 bit). AVL (1 bit). Seg Limit (4 bits). 
                                // G: Granularity. If flag is clear, the segment limit is interpreted in byte units; when flag is set, the segment limit is interpreted in 4-KByte units
                                // D/B: Performs different functions depending on whether the segment descriptor is an executable code segment, an expand-down data segment, or a stack segment.
                                // L: 64-bit code segment flag
                                // AVL: reserved for system software
                                // Seg Limit: continuation of first 16 bits.
    uint8_t base_24_31_bits;
} __attribute__((packed));

struct segment_descriptor {
    /* These fields are byte granular */
    /* Thus, base and limit can be max 4 GB. */
    uint32_t base;
    uint32_t limit;         
    uint8_t type;               // Corresponds to Access byte
};

/* TODO: this is a good idea eventually - just have to figure out the assembly intepretation
struct gdt {
    struct segment_descriptor_raw gdt_entry[TOTAL_GDT_SEGMENTS];
    int num_entries;
};
*/

/* Converts a list of segment_descriptor into a list of segment_descriptor_raw 
 * total_entries is the length of each list
 */
void segment_descriptor_to_raw(struct segment_descriptor_raw* raw, struct segment_descriptor *desc, int total_entries);

/* Load gdt by updating gdtr to reference it. Size is the number of entries in the gdt 
 * TODO: this function should take gdt and do gdt_raw conversion itself
 */
void gdt_load(struct segment_descriptor_raw *gdt, uint16_t size);

#endif