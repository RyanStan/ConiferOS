#include "disk_stream.h"
#include "memory/heap/kernel_heap.h"
#include "disk/disk.h"


struct disk_stream *get_disk_stream(int disk_index) 
{
        struct disk *disk = disk_get(disk_index);
        if (!disk)
                return 0;

        struct disk_stream *disk_stream = kzalloc(sizeof(struct disk_stream));
        disk_stream->pos = 0;
        disk_stream->disk = disk;
        return disk_stream;
}

int disk_stream_seek(struct disk_stream *disk_stream, int pos)
{
        disk_stream->pos = pos;
        return 0;
}

int disk_stream_read(struct disk_stream *disk_stream, void *out, int total)
{
        int sector = disk_stream->pos / DISK_SECTOR_SIZE;
        int offset = disk_stream->pos % DISK_SECTOR_SIZE;
        char buf[DISK_SECTOR_SIZE];

        int rc = disk_read_block(disk_stream->disk, sector, 1, buf);
        if (rc < 0)
                return rc;
        
        if (offset + total > DISK_SECTOR_SIZE) {
                /* Read in until the end of the sector and recall the function */
                for (int i = offset; i < DISK_SECTOR_SIZE; i++) {
                        *(char *)out = buf[i];
                        (char *)out++;
                }
                disk_stream->pos += DISK_SECTOR_SIZE - offset;
                rc = disk_stream_read(disk_stream, out, total - (DISK_SECTOR_SIZE - offset));           // TODO: find a way to do this without recursion.  Uses too much stack space
                if (rc < 0)
                        return rc;
        } else {
                for (int i = offset; i < offset + total; i++) {
                        *(char *)out = buf[i];
                        (char *)out++;
                }
                disk_stream-> pos += total;
        }
        return 0;

}

/* Free the memory associated with disk_stream */
void disk_stream_close (struct disk_stream *disk_stream)
{
        kfree(disk_stream);
}