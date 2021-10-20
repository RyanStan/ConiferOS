#include "pparser.h"
#include "config.h"
#include "string/string.h"
#include "memory/memory.h"
#include "status.h"
#include "memory/heap/kernel_heap.h"

/* Returns 0 if path is a valid format, or -1 otherwise
 * A valid path starts with a driver number like '0:/'
 * Drives can only be numeric values 0-9
 */
static int path_valid_form(const char *filename)
{
        int len = strnlen(filename, MAX_FILE_PATH_CHARS);
        return len > 2 && is_digit(filename[0]) && memcmp((void *)&filename[1], ":/", 2) == 0;
}

/* Returns the drive number associated with the path
 * Side effect: modifies path pointer by pushing it past drive number (which is why this function takes a double pointer)
 */
static int get_drive_by_path(const char **path)
{
        if (!path_valid_form(*path))
                return -EBADPATH;

        int drive_no = ctoi(*path[0]);

        /* Add 3 bytes to skp drive number 0:/ 1:/ 2:/ */
        *path += 3;
        
        return drive_no;
}

/* Returns an unlinked path_root struct that has the drive_no field filled */
static struct path_root *pparser_create_root(int drive_no)
{
        struct path_root *path_r = kzalloc(sizeof(struct path_root));
        path_r->drive_no = drive_no;
        path_r->first = 0;
        return path_r; 
}

/* Returns next path part at start of path (i.e. a dir or file)
 * Side effect: increments path to next part after each call
 * Example: the passed in path is java/test.class.
 *          the first call will return 'java' and the second call will
 *          return 'test.class'
 */
static const char *pparser_get_path_part(const char **path)
{
        char *result_path_part = kzalloc(MAX_FILE_PATH_CHARS);
        int i = 0;

        while (**path != '/' && **path != 0) {
                result_path_part[i] = **path;
                *path += 1;
                i++;
        }

        if (**path == '/')
                *path += 1;

        if (i == 0) {
                /* There was no path part */
                kfree(result_path_part);
                result_path_part = 0;
        }

        return result_path_part;
        
}

/* Parses and returns a path_part struct corresponding to the next path part in path.
 * last_part is the last returned path_part struct that we will link to this one
 */                
struct path_part *parse_path_part(struct path_part *last_part, const char **path)
{
        const char *path_part_str = pparser_get_path_part(path);
        if (!path_part_str)
                return 0;

        struct path_part *part = kzalloc(sizeof(struct path_part));
        part->part = path_part_str;
        part->next = 0;

        if (last_part)
                last_part->next = part;

        return part;
}

void pparser_free(struct path_root *root)
{
        struct path_part *part = root->first;
        while (part) {
                struct path_part *next_part = part->next;
                kfree((void *)part->part);
                kfree((void *)part);
                part = next_part;
        }

        kfree((void *)root);
}

// TODO: functon would probably be better if it returned an integer indicating the response code and just took a pointer
//       to a path_root instead
// TODO: Need to implement current_dir_path 
struct path_root *pparser_parse(const char *path, const char *current_dir_path)
{
        const char *tmp_path = path;
        struct path_root *path_root;
        int drive_no;
        struct path_part *first_part;

        if (strlen(path) > MAX_FILE_PATH_CHARS)
                return 0;

        if ((drive_no = get_drive_by_path(&tmp_path)) < 0)
                return 0;

        if (!(path_root = pparser_create_root(drive_no)))
                return 0;

        if (!(first_part = parse_path_part(NULL, &tmp_path)))
                return 0;

        path_root->first = first_part;

        struct path_part *part = parse_path_part(first_part, &tmp_path);
        while (part) {
                part = parse_path_part(part, &tmp_path);
        }

        return path_root;
}