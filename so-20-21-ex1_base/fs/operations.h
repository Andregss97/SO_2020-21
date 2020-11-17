#ifndef FS_H
#define FS_H
#include "state.h"

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType, int* buffer);
int delete(char *name, int* buffer);
int lookup(char *name, int *buffer);
int move(char *name1, char* name2, int* buffer);
void print_tecnicofs_tree(FILE *fp);

#endif /* FS_H */
