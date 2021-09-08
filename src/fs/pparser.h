#ifndef PPARSER_H
#define PPARSER_H

#endif

struct path_root {
        int drive_no;
        struct path_part *first;
};

struct path_part {                      // TODO: better name?
        const char *part;
        struct path_part *next;
};

/* Parses the file path and returns the complete corresponding path_root struct 
 * Accepts current_dir_path as a parameter so that it can understand relative paths (not implemented yet though)
 * Returns 0 on failure
 */
struct path_root *pparser_parse(const char *path, const char *current_dir_path);

/* Frees the memory of all linked path structs associated with root */
void pparser_free(struct path_root *root);