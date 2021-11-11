#include "fs/fat/fat16.h"
#include "status.h"
#include "string/string.h"
#include "disk/disk_stream.h"
#include "memory/memory.h"
#include "memory/heap/kernel_heap.h"
#include "kernel.h"
#include "config.h"
#include <stdint.h>
#include <stddef.h>

#define FAT16_SIGNATURE         0x29
#define FAT16_FAT_ENTRY_SIZE    0x02                    // In bytes
#define FAT16_BAD_SECTOR        0xFFF7
#define FAT16_UNUSED            0x00

/* FAT directory entry attribute byte bitmasks */
#define FAT_FILE_READ_ONLY      0x01                    // If this bit is set, the operating system will not allow a file to be opened for modification. 
#define FAT_FILE_HIDDEN         0x02                    // Hides files or directories from normal directory views. 
#define FAT_FILE_SYSTEM         0x04                    // Indicates that the file belongs to the system and must not be physically moved because there may be references into the file using absolute addressing (bypassing the fs)
#define FAT_FILE_VOLUME_LABEL   0x08                    // Indicates an optional directory volume label, normally only residing in a volume's root directory
#define FAT_FILE_SUBDIRECTORY   0x10                    // Indicates that the cluster-chain associated with this entry gets intepreted as a subdirectory instead of as a file
#define FAT_FILE_ARCHIVED       0x20                    // Typically set by the OS as soon as the file is created or modified to mark the file as "dirty" and reset by backup software once the file has been backed up to indicate "pure" state
#define FAT_FILE_DEVICE         0x40                    // Internally set for character device names found in filespecs, never found on disk, must not be changed by disk tools
#define FAT_FILE_RESERVED       0x80                    // Reserved, must not be changed by disk tools


/* Will be used to quickly identify whether a directory entry represents a directory or a file.
 * This is used for our own internal data structure and is not written to disk.
 */
enum fat_dir_entry_type {
        FAT_ITEM_TYPE_DIRECTORY,
        FAT_ITEM_TYPE_FILE
};

/* Extended BIOS Parameter block */
struct fat_header_extended {
        uint8_t drive_number;
        uint8_t win_nt_bit;
        uint8_t extended_boot_signature;
        uint32_t volume_id;
        uint8_t volume_id_string[11];                   // Partition Volume Label, padded with blanks
        uint8_t system_id_string[8];                    // File system type, padded with blanks
} __attribute__((packed));

struct fat_header_primary {
        uint8_t short_jmp_ins[3];                       // Instructions to jump to the bootstrap code
        uint8_t oem_id[8];                              // Name of the formatting OS
        /* BIOS Parameter Block starts here */
        uint16_t bytes_per_sector;                      // This value is the number of bytes in each physical sector. The allowed values are: 512, 1024, 2048 or 4096 bytes
        uint8_t sectors_per_cluster;                    // This is the number of sectors per cluster. The allowed values are: 1, 2, 4, 8, 16, 32 or 128
        uint16_t reserved_sectors;                      // Since the reserved region always contain the boot sector a zero value in this field is not allowed. The usual setting of this value is 1. The value is used to calculate the location for the first sector containing the FAT.
        uint8_t fat_copies;                             // # of file allocation tables on the file system
        uint16_t root_dir_entries;                      // This value contain the number of possible (max) entries in the root directory. Its recommended that the number of entries is an even multiple of the BytesPerSector values. The recommended value for FAT16 volumes is 512 entries (compatibility reasons).
        uint16_t number_of_sectors;                     // This field states the total number of sectors in the volume
        uint8_t media_type;                             // Media Descriptor.  Describes the pysical format of the volume (e.g. 3.5-inc, 2-sided, 36-sector, fixed disk, etc...)
        uint16_t sectors_per_fat;                       // # of sectors occupied by one copy of the FAT
        uint16_t sectors_per_track;                     // This value is used when the volume is on a media which has a geometry - the LBA is broken down into a CHS address.  This field represents the multiple of the max. Head and Sector value used when the volume was formatted
        uint16_t number_of_heads;                       // This value is used when the volume is on a media which has a geometry - the LBA is broken down into a CHS address.   This field represents the Head value used when the volume was formatted
        uint32_t hidden_sectors;                        // When the volume is on a media that is partitioned, this value contains the number of sectors preceeding the first sector of the volume.
        uint32_t sectors_big;                           // This field states the total number of sectors in the volume
} __attribute__((packed));

struct fat_header {
        struct fat_header_primary fat_header_primary;
        struct fat_header_extended fat_header_extended;
} __attribute__((packed));

struct fat_directory_entry {
        uint8_t filename[8];                            // Must be trailing padded.  Allowed characters are alphanumeric only
        uint8_t ext[3];                                 // Same requirements as filename
        uint8_t attribute;                              // Defines a set of flags which can be set for directories, volume name, hiden files, system files, etc.
        uint8_t reserved;                               // Reserved for Windows NT
        uint8_t creation_time_millisecond;              // This field (1 byte) only contains the millisecond stamp in counts of 10 milliseconds. Therefore valid values are between 0 and 199 inclusive. 
        uint16_t creation_time;                         // Bits 15 - 11 = Hours (0-23).  Bits 10 - 5 = Minutes (0-59).  Bits 4 - 0 = Seconds (0-29)
        uint16_t creation_date;                         // Bits 15 - 9 = Years from 1980.  Bits 10 - 5 = Month of year (1-12).  Bits 4 - 0 = Day of month (1-31).
        uint16_t last_access;                           // This 16 bit field contain the date when the entry was last read or written to. In case of writes this field of cause contain the same value as the Last Write Date field. 
        uint16_t high_16_bits_first_cluster;            // High two bytes of the starting cluster that contains this entry's data
        uint16_t last_mod_time;                         // This 16 bit field is set to the time when the last write occured. When the entry is create this field and the Creation Time field should contain the same values.
        uint16_t last_mod_date;                         // This 16 bit field is set to the date when the last write occured. When the entry is create this field and the Creation Date field should contain the same values. 
        uint16_t low_16_bits_first_cluster;             // Low two bytes of the starting cluster that contains this entry's data
        uint32_t filesize;                              // This 32-bit field count the total file size in bytes. For this reason the file system driver must not allow more than 4 Gb to be allocated to a file. For other entries than files then file size field should be set to 0.
} __attribute__((packed));

/* fat_directory corresponds to a directory cluster and represents a directory in our filesystem
 * It doesn't directly correspond to an on-disk data structure.  This struct just makes it easier for us to manage things internally
 */
struct fat_directory {
        struct fat_directory_entry *entry;              // Points to the first directory entry in the directory that this struct represents
        int total;                                      // Total number of entries in the directory
        int sector;                                     // The first disk sector where this directory cluster is located
        int end_sector;                                 // The last disk sector that contains this directory cluster
};

/* This structure is meant to make dealing with directory entries simpler.  In the case where the entry we're working with
 * represents a directory, we can just access a fat_directory structure instead of the fat_directory_entry.
 * Similar to fat_directory, this structure does not directly correspond to on-disk data.
 * 
 * TODO: don't really like how this struct works.  Seems like we would want access to the raw entry AND the fat_directory instance (if the entry represents a directory)
 */
struct fat_easy_directory_entry {
        union {
                struct fat_directory_entry *entry;              // If the entry represents a file, we still want to access the fat_directory_entry structure
                struct fat_directory *directory;                // If the entry represents a directory, we can utilize our fat_directory structure instead of fat_directory_entry
        };

        enum fat_dir_entry_type type;
};

/* Represents an open file */
struct fat_file_descriptor {
        struct fat_easy_directory_entry *easy_directory_entry;  // The directory entry corresponding to the open file                 
        uint32_t pos;                                           // Current stream offset into file
};

/* Private data for internal use by FAT filesystem 
 * (This data structure is just to help make things easier)
 */
struct fat_private {
        struct fat_header header;
        struct fat_directory root_directory;

        /* Three separate stream structures but all point to the same disk 
         * that is associated with this fat filesystem.
         */
        struct disk_stream *cluster_read_stream;                // Used to stream file data from data clusters
        struct disk_stream *fat_read_stream;                    // Used to stream the file allocation table (FAT)
        struct disk_stream *directory_stream;                   // Used to stream directory clusters
};

int fat16_resolve(struct disk *disk);
void *fat16_open(struct disk *disk, struct path_part *path_part, enum file_mode mode);
size_t fat16_fread(struct disk *disk, void *descriptor, size_t size, size_t nmemb, char *out);
int fat16_fseek(void *private, size_t offset, enum file_seek_mode whence);

struct filesystem fat16_fs = {
        .resolve = fat16_resolve,
        .fs_open = fat16_fopen,
        .fs_fread = fat16_fread,
        .fs_fseek = fat16_fseek
};

struct filesystem *fat16_init()
{
        strcpy(fat16_fs.name, "FAT16");
        return &fat16_fs;
}

/* Initializes the filesystem's internal private data structures */
static void fat16_init_private(struct disk *disk, struct fat_private *fat_private)
{
        memset(fat_private, 0, sizeof(struct fat_private));
        fat_private->cluster_read_stream = get_disk_stream(disk->id);
        fat_private->fat_read_stream = get_disk_stream(disk->id);
        fat_private->directory_stream = get_disk_stream(disk->id);
}

/* Returns the absolute position of sector on disk 
 * "Absolute position" refers to the byte offset of sector from the start of the disk
 * Sector is a 0-based index.  I.e. sector 0 starts at byte 0, sector 1 starts at byte 512, etc.
 */
static int fat16_sector_to_absolute_pos(struct disk *disk, int sector)
{
        return sector * disk->sector_size;
}

/* Return the number of directory entries in the directory cluster that starts at directory_start_sector 
 * Returns < 0 if an error occurs
 */
static int fat16_get_total_items_for_directory(struct disk *disk, uint32_t directory_start_sector)
{
        int rc = 0;

        struct fat_directory_entry entry;
        memset(&entry, 0, sizeof(struct fat_directory_entry));

        int directory_start_pos = fat16_sector_to_absolute_pos(disk, directory_start_sector);
        struct fat_private *fat_private = disk->fs_private;
        struct disk_stream *directory_stream = fat_private->directory_stream;
        if ((rc = disk_stream_seek(directory_stream, directory_start_pos)) < 0) {
                return rc;
        }
        
        /* Read each entry in the directory 
         * TODO: if all the entries have been read from the cluster, we should see if there is another cluster following this one in the cluster chain or if this is the last cluster in the chain 
         * Given we're currently not implementing the above, the max # of directory entries we can have in a directory is 2048.
         * 128 sectors per cluster * 512 bytes per sector / 32 bytes per root directory entry = 2048 directory entries in a single cluster max
         * Actually, the max we can have is 2047 directory entries so that we have a blank entry denoting the end of the directory
         */
        int count = 0;
        while (1) {
                if ((rc = disk_stream_read(directory_stream, &entry, sizeof(struct fat_directory_entry))) < 0) {
                        return rc;
                }

                /* If the first byte of the entry is 0, then there are no more entries in this directory */
                if (entry.filename[0] == 0x00)
                        break;

                /* If the first byte of the entry is 0xE5, then the entry is unused (free) */
                if (entry.filename[0] == 0xE5)
                        continue;

                count++;
        }

        return count;
}

/* 
 * Load the root directory into the fat_directory structure out_root_dir
 * Returns 0 on success or < 0 on failure
 */
static int fat16_get_root_directory(struct disk *disk, struct fat_private *fat_private, struct fat_directory *out_root_dir)
{
        int rc;

        struct fat_header_primary *primary_header = &fat_private->header.fat_header_primary;
        int root_dir_sector = primary_header->fat_copies * primary_header->sectors_per_fat + primary_header->reserved_sectors;
        int root_dir_size = primary_header->root_dir_entries * sizeof(struct fat_directory_entry);      //It looks like I'm assuming that 
        int root_dir_total_sectors = root_dir_size / disk->sector_size;
        if (root_dir_size % disk->sector_size != 0) {
                root_dir_total_sectors++;
        }

        /* Since we are not setting root_dir_entries in the primary header (via our boot.asm file), we need to dynamically resolve this value */
        int root_dir_total_items = fat16_get_total_items_for_directory(disk, root_dir_sector);

        struct fat_directory_entry *root_dir = kzalloc(root_dir_size);
        if (!root_dir)
                return -ENOMEM;

        struct disk_stream *stream = fat_private->directory_stream;
        if ((rc = disk_stream_seek(stream, fat16_sector_to_absolute_pos(disk, root_dir_sector))) < 0)
                return rc;

        if ((rc = disk_stream_read(stream, root_dir, root_dir_size)) < 0)
                return rc;

        out_root_dir->entry = root_dir;
        out_root_dir->total = root_dir_total_items;
        out_root_dir->sector = root_dir_sector;
        out_root_dir->end_sector = root_dir_sector + root_dir_size / disk->sector_size;

        return 0;
        
}

/* fat16_resolve
*
* Reads boot sector of disk and returns 0 if disk is formatted to FAT16 or < 0 otherwise
*/
int fat16_resolve(struct disk *disk)
{
        int rc = 0;

        struct fat_private *fat_private = kzalloc(sizeof(struct fat_private));
        fat16_init_private(disk, fat_private);

        disk->fs_private = fat_private;

        struct disk_stream *stream = get_disk_stream(disk->id);
        if (!stream) {
                rc = -ENOMEM;
                goto out;
        }    
        
        if ((rc = disk_stream_read(stream, &fat_private->header, sizeof(fat_private->header))) < 0)
                goto out;
        
        if (fat_private->header.fat_header_extended.extended_boot_signature != 0x29) {
                rc = -EFSNOTUS;
                goto out;
        }
                
        if ((rc = fat16_get_root_directory(disk, fat_private, &fat_private->root_directory)) < 0)
                goto out;

        disk->filesystem = &fat16_fs;
        
out:
        if (stream)
                disk_stream_close(stream);

        if (rc < 0) {
                kfree(fat_private);
                disk->fs_private = 0;
        }

        return rc;
}

/*
 * Filenames in FAT16 must be trailing padded with space bytes (ASCII: 0x20).  This function accepts a 
 * string at in and returns a new string at out that has null chars in the place of spaces
 * 
 * Returns 0 on success or < 0 on failure
 * 
 * Side effect: increments the char pointer that out points to
 * TODO: seems like this function would be more versatile if it didn't have the above side effect.  Would have to rework how it's used in get filename if this was the case
 */
static int fat16_convert_spaces_to_null(char **out, const char *in)
{
        // CHECK: he uses a pointer to a pointer for out.  Do I need to do that as well?
        while (*in != 0x00 && *in != 0x20) {
                **out = *in;
                *out += 1;
                in += 1;
        }

        if (*in == 0x20)
                **out = 0x00;

        return 0;
}

/* Writes the filename associated with raw_entry to the buffer at out.  
 * At max will write maxlen chars (bytes) to out.
 * Returns 0 on success or < 0 on failure
 */
static int fat16_get_filename(struct fat_directory_entry *raw_entry, char *out, int maxlen)
{
        memset(out, 0x00, maxlen);
        fat16_convert_spaces_to_null(&out, (const char *)raw_entry->filename);

        /* Check to see if the file has an extension */
        if (raw_entry->ext[0] != 0x00 && raw_entry->ext[0] != 0x20) {
                *out = '.';
                out++;
                fat16_convert_spaces_to_null(&out, (const char *)raw_entry->ext);
        }

        return 0;
}

/* Returns a new fat_directory_entry which is created by copying n bytes from raw_entry.  
 * n must be larger than sizeof(struct fat_directory_entry)
 * TODO: why include n?? can't we just assume n =  sizeof(struct fat_directory_entry)??
 */
static struct fat_directory_entry *fat16_clone_raw_dir_entry(struct fat_directory_entry *raw_entry, size_t n)
{
        if (n < sizeof(struct fat_directory_entry))
                return 0;

        struct fat_directory_entry *entry_copy = kzalloc(n);
        if (!entry_copy)
                return 0;

        memcpy(entry_copy, raw_entry, n);
        
        return entry_copy;
}

/* Returns the index # (CHS indexing starts at 1) of the sector that corresponds with the start of cluster */
static int fat16_cluster_to_start_sector(struct fat_private *fat_private, int cluster)
{
        /* To understand this function, remember that the data region starts after the root directory and
         * is composed of cluster, each of which are made of multiple sectors.
         * The first two FAT entries (which theoretically correspond to clusters 0 and 1) store special data, which is why we multiply (cluster - 2) with the sectors per cluster.  Thus, the data region
         * starts at cluster 2.  
         */
        return fat_private->root_directory.end_sector + (cluster - 2) * fat_private->header.fat_header_primary.sectors_per_cluster;
}

/* Returns the index of the initial cluster that contains raw_entry's data 
 * If raw_entry represents a directory, then the first cluster will contain directory entries (directory cluster).
 * If raw_entry represents a file, then the first cluster will contain file data (data cluster).
 */
static uint32_t fat16_get_first_cluster(struct fat_directory_entry *raw_entry)
{
        return raw_entry->low_16_bits_first_cluster | raw_entry->high_16_bits_first_cluster;
}

/* Returns the sector that the first FAT (file allocation table) starts at */
static uint32_t fat16_get_first_fat_sector(struct fat_private *fat_private)
{
        /* The first FAT comes directly after the reserved region (which contains our boot and kernel code) */
        return fat_private->header.fat_header_primary.reserved_sectors;
}

/* Returns the file allocation table (fat) entry that corresponds with cluster 
 * Returns < 0 on error
 */
static uint16_t fat16_get_fat_entry(struct disk *disk, int cluster)
{
        /* Each fat entry is 16 bits, or two bytes */
        int rc = 0;
        struct fat_private *fat_private = disk->fs_private;
        struct disk_stream *fat_read_stream = fat_private->fat_read_stream;

        uint32_t fat_table_position_bytes = fat16_get_first_fat_sector(fat_private) * disk->sector_size;
        if ((rc = disk_stream_seek(fat_read_stream, fat_table_position_bytes + cluster * FAT16_FAT_ENTRY_SIZE)) < 0)
                return rc;

        uint16_t entry = 0;
        entry = disk_stream_read(fat_read_stream, &entry, FAT16_FAT_ENTRY_SIZE);

        return entry;
}

/* Returns the cluster in the chain that is offset bytes from the beginning of the cluster chain_start_cluster */
static int fat16_get_cluster_in_chain(struct disk *disk, int chain_start_cluster, int offset)
{
        struct fat_private *fat_private = disk->fs_private;
        int cluster = chain_start_cluster;
        int cluster_size_bytes = fat_private->header.fat_header_primary.sectors_per_cluster * disk->sector_size;
        int clusters_ahead = offset / cluster_size_bytes;
        for (int i = 0; i < clusters_ahead; i++) {
                int entry = fat16_get_fat_entry(disk, cluster);
                if (entry >= 0xFFF8 && entry <= 0xFFFF) {
                        /* 0xFFF8 - 0xFFFF represent End-of-file 
                         * If we've entered this if statement, then it means that the offset provided is out of bounds with regards to the cluster chain at chain_start_cluster
                         */
                        return -EIO;
                }

                if (entry == FAT16_BAD_SECTOR)
                        return -EIO;
                
                /* Reserved entry (first and second fat entries are reserved to hold special data) */
                if (entry >= 0xFFF0 && entry <= 0xFFF6)
                        return -EIO;

                /* If the entry is 0, then there is not an associated cluster */
                if (entry == 0x0000)
                        return -EIO;

                cluster = (int)entry;
        }
        return cluster;
}

/* Reads n bytes from the cluster chain beginning at chain_start_cluster.
 * Starts at offset bytes from chain_start_cluster.  If offset is large enough, this may mean that the read will start in a cluster further along the chain than chain_start_cluster.
 * Stores the read data at out.
 * Returns 0 on success or < 0 on failure
 */
static int fat16_read_internal(struct disk *disk, int chain_start_cluster, int offset, int n, void *out)
{
        struct fat_private *fat_private = disk->fs_private;
        struct disk_stream *cluster_read_stream = fat_private->cluster_read_stream;
        int cluster_size_bytes = fat_private->header.fat_header_primary.sectors_per_cluster * disk->sector_size;
        int start_cluster = fat16_get_cluster_in_chain(disk, chain_start_cluster, offset);
        if (start_cluster < 0)
                return start_cluster;

        int offset_from_cluster = offset % cluster_size_bytes;
        int start_sector = fat16_cluster_to_start_sector(fat_private, start_cluster);
        int start_pos_bytes = start_sector * disk->sector_size + offset_from_cluster;
        /* We can only read up to the end of a cluster.  Then we have to find the next cluster in the chain
         *
         * TODO: This has the same error that Daniel made elsewhere in the stream code.  If offset is partially into the cluster already,
         * then n does not necessarily have to be larger than cluster_size_bytes for the read to overrun into the next cluster.
         * This code incorrectly assumes that we're starting a read from the beginning of a cluster.
         */
        int bytes_to_read = n > cluster_size_bytes ? cluster_size_bytes : n;

        int rc = 0;
        if ((rc = disk_stream_seek(cluster_read_stream, start_pos_bytes)) < 0)
                return rc;
        if ((rc = disk_stream_read(cluster_read_stream, out, bytes_to_read)) < 0)
                return rc;
        
        n -= bytes_to_read;
        if (n > 0) {
                /* There is still more to read 
                 * TODO: this could be bad for stack space.
                 */
                rc = fat16_read_internal(disk, chain_start_cluster, offset + bytes_to_read, n, out + bytes_to_read);
                if (rc < 0)
                        return rc;
        }

        return rc;
}

/* Frees the allocated memory associated with directory */
static void fat16_free_directory(struct fat_directory *directory)
{
        if (!directory)
                return;

        if (directory->entry)
                kfree(directory->entry);

        kfree(directory);
}

/* Free the allocated memory associated with easy_dir_entry */
static void fat16_easy_dir_entry_free(struct fat_easy_directory_entry *easy_dir_entry)
{
        if (easy_dir_entry->type == FAT_ITEM_TYPE_DIRECTORY) {
                fat16_free_directory(easy_dir_entry->directory);
        } else if (easy_dir_entry->type == FAT_ITEM_TYPE_FILE) {
                kfree(easy_dir_entry->entry);
        } else {
                // panic() --> have to implement this
        }

        kfree(easy_dir_entry);
}

/* Creates and initializes a fat_directory instance which corresponds to the directory represented by raw_entry 
 * Returns 0 (NULL) on error.
 */
static struct fat_directory *fat16_load_fat_directory(struct disk *disk, struct fat_directory_entry *raw_entry)
{
        struct fat_directory *directory = 0;
        struct fat_private *fat_private = disk->fs_private;
        if (!(raw_entry->attribute & FAT_FILE_SUBDIRECTORY))
                return NULL;

        directory = kzalloc(sizeof (struct fat_directory));
        if (!directory)
                return NULL;

        int cluster = fat16_get_first_cluster(raw_entry);
        int cluster_sector = fat16_cluster_to_start_sector(fat_private, cluster);
        directory->total = fat16_get_total_items_for_directory(disk, cluster_sector);
        int directory_size = directory->total * sizeof(struct fat_directory_entry);
        directory->entry = kzalloc(directory_size);
        if (!directory->entry) {
                kfree(directory);
                return NULL;
        }

        int rc = fat16_read_internal(disk, cluster, 0x00, directory_size, directory->entry);
        if (rc < 0) {
                fat16_free_directory(directory);
                return NULL;
        }

        return directory;
}

/* Creates and returns a fat_easy_directory_entry instance corresponding with the fat_directory_entry raw_entry 
 * Returns 0 on failure
*/
static struct fat_easy_directory_entry *fat16_get_easy_entry(struct disk *disk, struct fat_directory_entry *raw_entry)
{
        struct fat_easy_directory_entry *easy_entry = kzalloc(sizeof(struct fat_easy_directory_entry));
        if (!easy_entry)
                return 0;

        if (raw_entry->attribute & FAT_FILE_SUBDIRECTORY) {
                easy_entry->directory = fat16_load_fat_directory(disk, raw_entry);
                easy_entry->type = FAT_ITEM_TYPE_DIRECTORY;
        } else {
                easy_entry->type = FAT_ITEM_TYPE_FILE;
                /* Since raw_entry might be deleted after this function is called, we need to make sure we store a copy of it */
                easy_entry->entry = fat16_clone_raw_dir_entry(raw_entry, sizeof(struct fat_directory_entry));                                            
        }

        return easy_entry;
}

/* Returns the directory entry in directory that is associated with name
 * Returns 0 if the entry name does not exist in directory
 */
static struct fat_easy_directory_entry *fat16_find_entry_in_directory(struct disk *disk, struct fat_directory *directory,  const char *name)
{
        struct fat_easy_directory_entry *entry = 0;
        char tmp_filename[MAX_FILE_PATH_CHARS];

        for (int i = 0; i < directory->total; i++) {
                fat16_get_filename(&directory->entry[i], tmp_filename, sizeof(tmp_filename));
                if (strnicmp(tmp_filename, name, sizeof(tmp_filename)) == 0) {
                       entry = fat16_get_easy_entry(disk, &directory->entry[i]);
                }
        }
        
        return entry;
}

/* Returns the directory entry at the given path on disk.  Returns NULL (0) on error */
static struct fat_easy_directory_entry *fat16_get_directory_entry(struct disk *disk, struct path_part *path)
{
        struct fat_private *fat_private = disk->fs_private;
        struct fat_easy_directory_entry *first_part_entry = fat16_find_entry_in_directory(disk, &fat_private->root_directory, path->part);
                                                                                                                                        
        if (!first_part_entry)
                return 0;

        struct fat_easy_directory_entry *curr_part_entry = first_part_entry;
        struct path_part *next_path_part = path->next;
        while (next_path_part) {
                if (curr_part_entry->type != FAT_ITEM_TYPE_DIRECTORY) {
                        /* This indicates the path is invalid.  Since next_path_part has a value,
                         * the type of this entry must be a directory
                         */
                        return NULL;
                }

                struct fat_easy_directory_entry *tmp_entry = fat16_find_entry_in_directory(disk, curr_part_entry->directory, next_path_part->part);
                /* Since fat16_find_entry_in_directory dynamically creates the fat16 structures in memory on the fly, we should release 
                 * the corresponding memory as we continue to traverse the path.
                 */
                fat16_easy_dir_entry_free(curr_part_entry);
                curr_part_entry = tmp_entry;
                next_path_part = next_path_part->next;
        }

        return curr_part_entry;
}

/* Creates a file descriptor corresponding to the file at path on disk 
 * Right now this filesystem implementation only supports file reads, so the only
 * allowed mode is READ.
 * Returns an initialized fat_file_descriptor instance on success, or < 0 on failure
 */
void *fat16_fopen(struct disk *disk, struct path_part *path, enum file_mode mode)
{
        /* We are only implementing reads in our filesystem for now */
        if (mode != READ)
                return ERROR(-ERDONLY);

        struct fat_file_descriptor *fat_file_descriptor = kzalloc(sizeof(struct fat_file_descriptor));
        if (!fat_file_descriptor) 
                return ERROR(-ENOMEM);

        fat_file_descriptor->easy_directory_entry = fat16_get_directory_entry(disk, path);
        if (!fat_file_descriptor->easy_directory_entry) {
                kfree(fat_file_descriptor);
                return ERROR(-EIO);
        }
                

        fat_file_descriptor->pos = 0;

        return fat_file_descriptor;
}

/* fat16_fread - binary stream input
 *
 * Reads nmemb items of data, each size bytes long, 
 * from the stream associated with descriptor (a fat_file_descriptor instance),
 * storing them at the location given by out.
 * This function can only be called on a file, not a directory.
 * 
 * 
 * Returns the count of size elements that were read.
 * This may not always equal nmemb since an error or end-of-file may occur after some
 * elements have already been read.  Therefore, to check for failure, 
 * look for a short item count return value (or 0)
 */
size_t fat16_fread(struct disk *disk, void *descriptor, size_t size, size_t nmemb, char *out)
{
        struct fat_file_descriptor *desc = descriptor;
        if (desc->easy_directory_entry->type == FAT_ITEM_TYPE_DIRECTORY) {
                return -EINVARG;
        }
        struct fat_directory_entry *raw_entry = desc->easy_directory_entry->entry;
        int offset = desc->pos;
        size_t count = 0;
        for (count = 0; count < nmemb; count++) {
                if (fat16_read_internal(disk, fat16_get_first_cluster(raw_entry), offset, size, out) < 0)
                        return count;
                out += size;
                offset += size;
        }
        return count;
}

/* fat16_fseek - reposition the file stream
 *
 * Sets the file position indicator for the file associated with private (fs implementation specific data).  
 * The new position, measured in bytes, 
 * is obtained by adding offset bytes to the position specified by whence.
 * If whence is set to SEEK_SET, SEEK_CUR, or SEEK_END, the offset is relative
 * to the start of the file, the current positoin indicator, or end-of-file, respectively.
 * 
 * Returns 0 on success or < 0 on failure.
 *
 * SEEK_END isn't implemented yet
 * 
 * 
 */
int fat16_fseek(void *private, size_t offset, enum file_seek_mode whence)
{
        struct fat_file_descriptor *desc = private;
        if (desc->easy_directory_entry->type == FAT_ITEM_TYPE_DIRECTORY) {
                return -EINVARG;
        }
        struct fat_directory_entry *raw_entry = desc->easy_directory_entry->entry;
        if (offset >= raw_entry->filesize) {
                return -EIO;
        }

        switch (whence) {
                case SEEK_SET:
                        desc->pos = offset;
                        break;
                case SEEK_CUR:
                        desc->pos += offset;
                        break;
                case SEEK_END:
                        return -EUNIMP;
                default:
                        return -EINVARG;
        }
        
        return 0;
}