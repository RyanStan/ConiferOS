/* file.h
 *
 * Our VFS interface
 * The actual concrete filesystems must implement these functions
 * 
 * We have significantly fewer levels of abstraction in our vfs than the Linux kernel 
 * (i.e. no dentry, inode, etc...)
 */

#ifndef FILE_H
#define FILE_H

#include "fs/pparser.h"
#include <stddef.h>
#include <stdint.h>

#define FS_NAME_MAX     20

enum file_seek_mode {
        SEEK_SET,
        SEEK_CUR,
        SEEK_END
};

enum file_mode {
        READ,           //O_RDONLY
        WRITE,          //O_WRONLY
        APPEND,         //O_APPEND
        INVALID         //??
};

/* File stat flags (bitmasks) */
#define FILE_STAT_READ_ONLY 0b00000001

struct file_stat {
        unsigned int flags;
        uint32_t filesize;
};

/* We need to forward declare disk since disk.h includes this file */
struct disk;            

/* Each concrete file system driver implementation in our OS will have an associated filesystem struct instance */
struct filesystem {

        char name[FS_NAME_MAX];

        /* fs_open - stream open function 
         *
         * Opens the file whose path is the value contained in list beginning with path_part and associates
         * a stream with it.  This will return filesystem implementation specific private data.
         */
        void *(*fs_open)(struct disk *disk, struct path_part *path_part, enum file_mode mode);

        /* resolve
         *
         * returns true if the disk is formatted
         * to the specification of this function's implementation filesystem's format
         * 
         * E.g. if the function/filesystem implementing this function pointer is part of our
         * FAT filesystem driver, then it will return true if disk is formatted as a FAT filesystem
         */
        int (*resolve)(struct disk *disk); 

        /* fs_fread - binary stream input
         *
         * Reads nmemb items of data, each size bytes long, 
         * from the stream associated with private (fs implementation specific data),
         * storing them at the location given by out.
         * This function can only be called on a file, not a directory.
         * 
         * Returns the count of size elements that were read.
         * This may not always equal nmemb since an error or end-of-file may occur after some
         * elements have already been read.
         * Therefore, to check for failure, look for a short item count return value (or 0)
         */
        size_t (*fs_fread)(struct disk *disk, void *private, size_t size, size_t nmemb, char *out);


        /* fs_fseek - reposition the file stream
        *
        * Sets the file position indicator for the file associated with private (fs implementation specific data).  
        * The new position, measured in bytes, 
        * is obtained by adding offset bytes to the position specified by whence.
        * If whence is set to SEEK_SET, SEEK_CUR, or SEEK_END, the offset is relative
        * to the start of the file, the current positoin indicator, or end-of-file, respectively.
        * 
        * Returns 0 on success or < 0 on failure.
        * 
        */
        int (*fs_fseek)(void *private, size_t offset, enum file_seek_mode whence);

        /* fs_fstat - get file status
         *
         * Returns information about the file associated with private (fs implementation specific data).
         * stat will be set by function and will point to returned file_stat instance.
         * 
         * Integer return value of 0 on success or < 0 on failure.
         */
        int (*fs_fstat)(struct disk *disk, void *private, struct file_stat *stat);

        /* fs_fclose - close a stream
         *
         * Closes the file associated with private (fs implementation specific data).
         * Returns 0 on success or < 0 on failure.
         */
        int (*fs_fclose)(void *private);
};

/* File descriptor that represents an open file */
struct file_descriptor {

        /* Corresponds to the index of the file descriptor within the array of all file descriptors (file_descriptors) */
        int index;
        
        /* In the Linux kernel, a file descriptor is tied to a dentry object which is tied to an inode 
         * which is tied to a super block which is tied to a filesystem_type.  For our kernel,
         * we're linking the file directly to the filesystem_type (or just filesystem in our case).
         */
        struct filesystem *filesystem;

        struct disk *disk;

        /* private will point to driver implementation specific data
         * which will be used by other functions for finding the file on disk.
         * This field will be set by fs_open.
         */
        void *private;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Our VFS layer functions */

/* Initializes all file systems.
 * Loads compile time filesystem drivers.
 */
void fs_init();

/* Dynamically inserts a filesystem.  This loads our OS with the file system driver so that
 * future filesystems of the corresponding type can be mounted to our OS.  
 */
void fs_insert_filesystem(struct filesystem *filesystem);

/* Responsible for locating the correct filesystem to open a file with.
 * Calls the fopen function corresponding with the filesystem that owns the file.
 * 
 * mode_str - string that specifies the mode the file should be opened in
 *               'r' = read, 'w' = write, 'a' = append
 * filename - absolute path to the file
 * 
 * Returns the file descriptor index associated with filename (non-negative integer) on success
 * Returns < 0 on failure
 */
int fopen(const char *filename, const char *mode_str);

/* Finds a filesystem that can manage the disk.  Calls that respective filesystem's resolve
 * function on the disk.  
 * Returns a pointer to the filesystem on success or 0 on failure.
 * TODO: the way we're using this right now is odd.  We're calling it within disk_search_and_init to assign the disk's filesystem.
 *       However, it seems like since the filesystem's resolve method is passed the disk, it should just set the disk's filesystem itself
 */
struct filesystem *fs_resolve(struct disk *disk);

/* fread - binary stream input
 *
 * Reads nmemb items of data, each size bytes long, 
 * from the file stream associated with the file descriptor fd.
 * Stores the read data at the location given by ptr.
 * This function can only be called on a file, not a directory.
 * 
 * 
 * Returns the count of size elements that were read.
 * This may not always equal nmemb since an error or end-of-file may occur after some
 * elements have already been read.
 * Therefore, to check for failure, look for a short item count return value (or < 0)
 */
size_t fread(void *ptr, size_t size, size_t nmemb, int fd);

/* fseek - reposition the file stream
 *
 * Sets the file position indicator for the file represented by the 
 * file descriptor fd.  The new position, measured in bytes, 
 * is obtained by adding offset bytes to the position specified by whence.
 * If whence is set to SEEK_SET, SEEK_CUR, or SEEK_END, the offset is relative
 * to the start of the file, the current positoin indicator, or end-of-file, respectively.
 * 
 * Returns 0 on success or < 0 on failure.
 * 
 */
int fseek(int fd, size_t offset, enum file_seek_mode whence);


/* fstat - get file status
 *
 * Returns information about the file associated with the file descriptor fd.
 * stat will be set by function and will point to returned file_stat instance.
 * 
 * Integer return value of 0 on success or < 0 on failure.
 */
int fstat(int fd, struct file_stat *stat);

/* fclose - close a stream
 *
 * Closes the file associated with the file descriptor fd.
 * Returns 0 on success or < 0 on failure.
 */
int fclose(int fd);

#endif