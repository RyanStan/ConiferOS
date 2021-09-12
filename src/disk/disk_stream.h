/* disk_stream.h
 * 
 * provides an interface for streaming data from a disk.
 * allows us to work with arbitrary sizes from disk (e.g. 20 MB)
 * without having to read from and work with disk in sector block sizes (512 bytes)
 */

#ifndef DISK_STREAM_H
#define DISK_STREAM_H

#include "disk.h"

struct disk_stream {
        int pos;                // the position we're at in the stream (seeking at in the disk)
        struct disk *disk;
};

/* Returns a new disk stream for the disk associated with index disk_index 
 * Returns 0 on failure
 */
struct disk_stream *get_disk_stream(int disk_index);

/* Repositions the seeker (next byte to be read) to pos 
 * Returns 0 on success
 */
int disk_stream_seek(struct disk_stream *disk_stream, int pos);

/* Read bytes from disk stream
 * 
 * disk_stream  - the disk stream to read bytes from
 * out          - the read bytes are written to this buffer
 * total        - the number of bytes to read
 * 
 * returns 0 on success or < 0 on failure
 */
int disk_stream_read(struct disk_stream *disk_stream, void *out, int total);

/* Free the memory associated with disk_stream */
void disk_stream_close (struct disk_stream *disk_stream);

#endif