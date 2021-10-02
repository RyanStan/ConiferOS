#include "fs/file.h"
#include "config.h"
#include "print/print.h"
#include "memory/memory.h"
#include "memory/heap/kernel_heap.h"
#include "status.h"
#include "fs/fat/fat16.h"

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
        //TODO: implement a panic for the case when a valid filesystem is not passed in or there are no free fs slots

        if (!filesystem) {
                print("fs_insert_filesystem passed invalid argument"); 
                while (1) {}
        }

        struct filesystem **fs = get_free_fs_slot();
        if (!fs) {
                print("Problem inserting filesystem"); 
                while (1) {}
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

/* Searches for an open slot in file_descriptors.  If it finds one, it will
 * allocate a new file descriptor and assign desc_out the address of the pointer to that new file descriptor.
 * Returns 0 on success or -ENOMEM on failure
 *
 * TODO: is there a way to make this comment less wordy?
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

int file_open(const char *filename, enum file_mode mode)
{
        /* Not implemented yet */
        return -EIO;
}