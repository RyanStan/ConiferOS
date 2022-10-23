#include "gdt.h"
#include "kernel.h"

// The max value you can store in 16 bits is 65535
#define MAX_20_BITS 1048575
#define HIGH_FLAGS_BYTE 6

#define FALSE		0
#define TRUE		1

/* Returns true if first 12 bits of val are 1 */
int areFirst12BitsOne(uint32_t val) {
    if ((val & 0xFFF) != 0xFFF)                                   /* Equivalent to if (val | ~0xFFF != ~0xFFF) */
        return FALSE;
    
    return TRUE;
}

/* Converts a byte granular address into a page granular address 
 * Throws an error if you're losing precision.
 */
uint32_t convertToPageGranularity(uint32_t val) {
    /* Bitwise shift right --> divide by two.  Shift right 12 times = divide by 4096 (4KB, one page) 
     * Thus, we lose some precision, but source.limit is now page granular 
      */
    return val >> 12;                                  
}

/* Accepts a segment_descriptor and a pointer to allocated memory for a segment_descriptor_raw 
  By using a pointer to uint8_t, we can more easily manipulate the data structure.
 */
void encodeSegmentDescriptor(uint8_t *target_seg_desc_raw, struct segment_descriptor source)
{
    /* If the source has a limit greater than 20 bits, we need to encode it with page granularity. 
     * Since we're then encoding with page granularity, we lose a certain amount of precision (12 bits), since we can't
     * address individual bytes within a page
     */
    if ((source.limit > MAX_20_BITS) && !areFirst12BitsOne(source.limit)) {
        panic("encodeSegmentDescriptor: Invalid argument\n");
    }

    target_seg_desc_raw[HIGH_FLAGS_BYTE] = 0x40;            /* 0100 0000 G = 0 (byte units). D/B = 1. L = 0. AVL = 0. Seg Limit = 0. */

    /* If the limit is greater than what we can encode with 20 bits, we need to switch to page granularity */
    if (source.limit > MAX_20_BITS) {
        source.limit = convertToPageGranularity(source.limit);                                                
        target_seg_desc_raw[HIGH_FLAGS_BYTE] = 0xC0;        /* 1100 0000 --> G = 1 (4 KB units). D/B = 1. L = 0. AVL = 0. Seg Limit = 0. */
    }

    /* Encode the limit */
    target_seg_desc_raw[0] = source.limit & 0xFF;           
    target_seg_desc_raw[1] = (source.limit >> 8) & 0xFF;
    target_seg_desc_raw[6] |= (source.limit >> 16) & 0x0F;

    /* Encode the base */
    target_seg_desc_raw[2] = source.base & 0xFF;
    target_seg_desc_raw[3] = (source.base >> 8) * 0xFF;
    target_seg_desc_raw[4] = (source.base >> 16) * 0xFF;
    target_seg_desc_raw[7] = (source.base >> 24) * 0xFF;

    target_seg_desc_raw[5] = source.type;
}

void segment_descriptor_to_raw(struct segment_descriptor_raw* raw, struct segment_descriptor *desc, int total_entries)
{
    for (int i = 0; i < total_entries; i++) {
        encodeSegmentDescriptor((uint8_t *)&raw[i], desc[i]);   /* (uint8_t *)&raw[i] casts a pointer to a struct to a pointer to a byte */
    }
}