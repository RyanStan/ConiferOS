#include "fs/file.h"
#include "config.h"
#include "print/print.h"
#include "memory/memory.h"
#include "memory/heap/kernel_heap.h"
#include "status.h"
#include "fs/fat/fat16.h"
#include "fs/pparser.h"
#include "disk/disk.h"
#include "string/string.h"
#include "kernel.h"

/* TODO: should adjust functions to fit linux kernel return value style guidelines.
 * Imperative command functions should return 0 on success or < 0 on failure.
 * Predicates should return a boolean.  I.e. 0 on failure and 1 on success.
 */

/* Array of each filesystem driver implementation in our kernel */
struct filesystem *filesystems[MAX_FILESYSTEMS];

/* Array of every open file */
struct file_descriptor *file_descriptors[MAX_OPEN_FILES];

/* Finds an open slot in the filesystems arrays (space for a new filesystem implementation) and returns a pointer to that slot 
 * Returns 0 if no free filesystem slots are available
 */
static struct filesystem **get_free_fs_slot()
{
        for (int i = 0; i < MAX_FILESYSTEMS; i++) {
                if (!filesystems[i])
                        return &filesystems[i];
        }

        return 0;
}

void fs_insert_filesystem(struct filesystem *filesystem)
{
        if (!filesystem) {
                panic("ERROR: fs_insert_filesystem passed invalid filesystem");
        }

        struct filesystem **fs = get_free_fs_slot();
        if (!fs) {
                panic("ERROR: fs_insert_filesystem no free fs slots");
        }

        *fs = filesystem;
}

/* Loads filesystems that are built into the kernel at compile time */
static void fs_static_load()
{
        memset(filesystems, 0, sizeof(filesystems));
        fs_insert_filesystem(fat16_init());
}

void fs_init()
{
        memset(file_descriptors, 0, sizeof(file_descriptors));
        fs_static_load();
}

static void free_file_descriptor(struct file_descriptor *desc)
{
      file_descriptors[desc->index] = 0;
      kfree(desc);
}

/* Searches for an open slot in file_descriptors.  If it finds one, it will
 * allocate a new file descriptor and assign desc_out the address of the pointer to that new file descriptor.
 * Returns 0 on success or -ENOMEM on failure
 */
static int file_new_descriptor(struct file_descriptor **desc_out)
{
        for (int i = 0; i < MAX_OPEN_FILES; i++) {
                if (!file_descriptors[i]) {
                        file_descriptors[i] = kzalloc(sizeof(struct file_descriptor));
                        file_descriptors[i]->index = i;
                        *desc_out = file_descriptors[i];
                        return 0;
                }

        }
        return -ENOMEM;
}

/* Searches through the open file descriptors and returns one 
 * where the index is equal to the argument fd.
 * Returns 0 on failure
 */
static struct file_descriptor *file_get_descriptor(int fd)
{
        if (fd < 0 || fd > MAX_OPEN_FILES - 1)
                return 0;

        return file_descriptors[fd];
}

struct filesystem *fs_resolve(struct disk *disk)
{
        struct filesystem *fs = 0;
        for (int i = 0; i < MAX_FILESYSTEMS; i++) {
                if (filesystems[i] != 0 && filesystems[i]->resolve(disk) == 0) {
                        fs = filesystems[i];
                        break;
                }
        }

        return fs;
}

/* Accepts a string that corresponds to a file mode
 * Returns the file_mode associated with the string
 * 'r' = FILE_MODE_READ
 * 'w' = FILE_MODE_WRITE
 * 'a' = FILE_MODE_APPEND
 * If str is something else, then returns FILE_MODE_INVALID
 */
enum file_mode file_get_mode_by_string(const char *str)
{
        if (strncmp(str, "r", 1) == 0)
                return READ;
        else if (strncmp(str, "w", 1) == 0)
                return WRITE;
        else if (strncmp(str, "a", 1) == 0)
                return APPEND;
        else 
                return INVALID;
}

int fopen(const char *filename, const char *mode_str)
{
        struct path_root *path_root = pparser_parse(filename, NULL);
        if (!path_root)
                return -EINVARG;

        /* We can't have just a root path.  E.g. '0:/'.  We need something like '0:/bin/shell.bin' */
        if (!path_root->first)
                return -EINVARG;

        /* Make sure the disk we are reading from exists */
        struct disk *disk = disk_get(path_root->drive_no);
        if (!disk)
                return -EIO;

        /* Ensure that the disk is formatted to a filesystem that we understand and have a driver implementation for */
        if (!disk->filesystem)
                return -EIO;

        enum file_mode file_mode = file_get_mode_by_string(mode_str);
        if (file_mode == INVALID)
                return -EINVARG;

        void *file_priv_data = disk->filesystem->fs_open(disk, path_root->first, file_mode);
        if (IS_ERROR(file_priv_data))
                return ERROR_I(file_priv_data);

        int rc = 0;
        struct file_descriptor *file = 0;
        rc = file_new_descriptor(&file);
        if (rc < 0)
                return rc;
        file->filesystem = disk->filesystem;
        file->private = file_priv_data;
        file->disk = disk;
        return file->index;
}

size_t fread(void *ptr, size_t size, size_t nmemb, int fd)
{
        if (size == 0 || nmemb == 0 || fd < 0)
                return -EINVARG;

        struct file_descriptor *desc = file_get_descriptor(fd);
        if (!desc)
                return -EINVARG;

        int rc = desc->filesystem->fs_fread(desc->disk, desc->private, size, nmemb, (char *)ptr);
        return rc;
}

int fseek(int fd, size_t offset, enum file_seek_mode whence)
{
        struct file_descriptor *desc = file_get_descriptor(fd);
        if (!desc)
                return -EINVARG;
        
        return desc->filesystem->fs_fseek(desc->private, offset, whence);
}

int fstat(int fd, struct file_stat *stat)
{
        struct file_descriptor *desc = file_get_descriptor(fd);
        if (!desc)
                return -EINVARG;

        return desc->filesystem->fs_fstat(desc->disk, desc->private, stat);
}

int fclose(int fd)
{
        struct file_descriptor *desc = file_get_descriptor(fd);
        if (!desc)
                return -EINVARG;

        desc->filesystem->fs_fclose(desc->private);
        free_file_descriptor(desc);
        return 0;
}