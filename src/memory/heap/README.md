# Heap Algorithm

Taken from the Udemy Course "Developing a Multithreaded Kernel From Scratch" by Daniel McCarthy.
Down the road I plan on reimplementing this algorithm myself to be more efficient (reduce memory fragmentation issues)

## Entry Table
Array of one byte values that represent each entry in our heap data pool.

### The entry structure


Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0
----- | ----- | ----- | ----- | ----- | ----- | ----- | -----
HAS\_N | IS\_FIRST |  0 | 0 | ET\_3 | ET\_2 | ET\_1 | ET\_0

* HAS\_N = Set if the entry to the right is part of this allocation
* IS\_FIRST = Set if this is the first entry of this allocation
* ET\_3 - ET\_0 = Entry type

**Each entry in the table describes 4096 bytes of data in the heap data pool**

### Entry types

* HEAP\_BLOCK\_TABLE\_ENTRY\_TAKEN = The entry is taken and the address cannot be used
* HEAP\_BLOCK\_TABLE\_ENTRY\_FREE = The entry is free and may be used

## Basic Algorithm Overview
1. Take allocation size from malloc and calculate how many blocks we need to allocate
2. Find the first entry in the entry table that has a type of HEAP\_BLOCK\_TABLE\_ENTRY\_FREE
3. Ensure that, if the allocation size requires, the next n blocks are free as well.  Otherwise, keep looking
4. If string of blocks is found that fits requested allocation, set types to taken.  Set first block IS\_FIRST.  Set HAS\_N on intermediate blocks within allocation
