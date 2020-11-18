#include "operations.h"
#include "state.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 */
void split_parent_child_from_path(char * path, char ** parent, char ** child) {

	int n_slashes = 0, last_slash_location = 0;
	int len = strlen(path);

	// deal with trailing slash ( a/x vs a/x/ )
	if (path[len-1] == '/') {
		path[len-1] = '\0';
	}

	for (int i=0; i < len; ++i) {
		if (path[i] == '/' && path[i+1] != '\0') {
			last_slash_location = i;
			n_slashes++;
		}
	}

	if (n_slashes == 0) { // root directory
		*parent = "";
		*child = path;
		return;
	}

	path[last_slash_location] = '\0';
	*parent = path;
	*child = path + last_slash_location + 1;

}


/*
 * Initializes tecnicofs and creates root node.
 */
void init_fs() {
	inode_table_init();
	int i = 0;
	int root_iNode = 0;
	int* count = &i;
	int* buffer_locks = malloc(INODE_TABLE_SIZE * sizeof(int));
	pthread_rwlock_t *rwl;

	/* create root inode */
	int root = inode_create(T_DIRECTORY, buffer_locks, count);

	
	if (root != FS_ROOT) {
		printf("failed to create node for tecnicofs root\n");
		exit(EXIT_FAILURE);
	}
	inode_get_lock(buffer_locks[root_iNode], &rwl);
	pthread_rwlock_unlock(rwl);
	printf(" UnLock no iNode: %d (INIT)\n", buffer_locks[root_iNode]);
	free(buffer_locks);
}


/*
 * Destroy tecnicofs and inode table.
 */
void destroy_fs() {
	inode_table_destroy();
}


/*
 * Checks if content of directory is not empty.
 * Input:
 *  - entries: entries of directory
 * Returns: SUCCESS or FAIL
 */

int is_dir_empty(DirEntry *dirEntries) {
	if (dirEntries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		if (dirEntries[i].inumber != FREE_INODE) {
			return FAIL;
		}
	}
	return SUCCESS;
}


/*
 * Looks for node in directory entry from name.
 * Input:
 *  - name: path of node
 *  - entries: entries of directory
 * Returns:
 *  - inumber: found node's inumber
 *  - FAIL: if not found
 */
int lookup_sub_node(char *name, DirEntry *entries) {
	if (entries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		if (entries[i].inumber != FREE_INODE && strcmp(entries[i].name, name) == 0) {
			return entries[i].inumber;
		}
	}
	return FAIL;
}


/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: SUCCESS or FAIL
 */
int create(char *name, type nodeType, int* buffer){

	int parent_inumber, child_inumber;
	int i = 0;
	int* count = &i;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType;
	union Data pdata;
	pthread_rwlock_t *rwl;

	printf("1\n");
	strcpy(name_copy, name);
	printf("2\n");
	split_parent_child_from_path(name_copy, &parent_name, &child_name);
	printf("Parent name: %s // Child name: %s\n", parent_name, child_name);

	printf("3\n");
	parent_inumber = lookup(parent_name, buffer, CREATE, count);
  printf("Parent iNumber is: %d\n", parent_inumber);

	printf("4\n");
	if (parent_inumber == FAIL) {
		//printf("failed to create %s, invalid parent dir %s\n",
		        //name, parent_name);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		//printf("failed to create %s, parent %s is not a dir\n",
		        //name, parent_name);
		return FAIL;
	}

	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
		//printf("failed to create %s, already exists in dir %s\n",
		       //child_name, parent_name);
		return FAIL;
	}

	/* create node and add entry to folder that contains new node */
	
	printf("4,5\n");
	child_inumber = inode_create(nodeType, buffer, count);
	if (child_inumber == FAIL) {
		//printf("failed to create %s in  %s, couldn't allocate inode\n",
		        //child_name, parent_name);
		return FAIL;
	}

	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		//printf("could not add entry %s in dir %s\n",
		       //child_name, parent_name);
		return FAIL;
	}

	for (int i=0; i < *count; i++){
		inode_get_lock(buffer[i], &rwl);
		pthread_rwlock_unlock(rwl);
		printf("\tBuffer index: %d // UnLock no iNode: %d\n", i, buffer[i]);
	}

	printf("DOES FREE create\n");
  free(buffer);


	return SUCCESS;
}


/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * Returns: SUCCESS or FAIL
 */
int delete(char *name, int* buffer){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	int i = 0;
	int* count = &i;
	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;
	pthread_rwlock_t *rwl;


	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name, buffer, DELETE, count);

	if (parent_inumber == FAIL) {
		//printf("failed to delete %s, invalid parent dir %s\n",
		        //child_name, parent_name);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		//printf("failed to delete %s, parent %s is not a dir\n",
		        //child_name, parent_name);
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);

	if (child_inumber == FAIL) {
		//printf("could not delete %s, does not exist in dir %s\n",
		       //name, parent_name);
		return FAIL;
	}

	inode_get(child_inumber, &cType, &cdata);

	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		//printf("could not delete %s: is a directory and not empty\n",
		       //name);
		return FAIL;
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		//printf("failed to delete %s from dir %s\n",
		       //child_name, parent_name);
		return FAIL;
	}

	if (inode_delete(child_inumber) == FAIL) {
		//printf("could not delete inode number %d from dir %s\n",
		       //child_inumber, parent_name);
		return FAIL;
	}


	for (int i=0; i < *count; i++){
		inode_get_lock(buffer[i], &rwl);
		pthread_rwlock_unlock(rwl);
		printf("\tBuffer index: %d // UnLock no iNode: %d\n", i, buffer[i]);
	}

	printf("DOES FREE delete\n");
  free(buffer);


	return SUCCESS;
}


/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(char *name, int *buffer, int flag, int* count) {
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";
	char* saveptr;

	printf("-----------\n");
	printf("1\n");
	strcpy(full_path, name);

	/* start at root node */
	printf("2\n");
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;
	pthread_rwlock_t *rwl;

	/* get root inode data */
	

	printf("3\n");
	char *path = strtok_r(full_path, delim, &saveptr);
	if (path){
		//READ LOCK
	printf("4\n");
		inode_get_lock(current_inumber, &rwl);
		pthread_rwlock_rdlock(rwl);
		printf(" Read Lock no iNode: %d\n", current_inumber);
		printf("Buffer Counter: %d\n", *count);
		buffer[(*count)++] = current_inumber;	
		printf("Buffer Counter [after]: %d\n", *count);
		inode_get(current_inumber, &nType, &data);
	}

	printf("5\n");
	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		printf("\tEntrou no ciclo WHILE com current_inumber %d\n", current_inumber);
		path = strtok_r(NULL, delim, &saveptr);
		if (path) {
			//READ LOCK
			inode_get_lock(current_inumber, &rwl);
			pthread_rwlock_rdlock(rwl);
			printf(" Read Lock no iNode: %d\n", current_inumber);
			buffer[(*count)++] = current_inumber;	
			inode_get(current_inumber, &nType, &data);
		}
	}

	printf("6\n");
	if(flag == LOOKUP){
		inode_get_lock(current_inumber, &rwl);
		pthread_rwlock_rdlock(rwl);
		printf("Read Lock no [último] iNode: %d  (LOOKUP)\n", current_inumber);
		buffer[(*count)++] = current_inumber;
	}
	else if(flag == DELETE || flag== CREATE){
		printf("7\n");
		inode_get_lock(current_inumber, &rwl);
		printf("8 - %d - %p\n", current_inumber, &rwl);
		pthread_rwlock_wrlock(rwl);
		printf("Write Lock no [último] iNode: %d  (DELETE/CREATE)\n", current_inumber);
		printf("Buffer Counter: %d\n", *count);
		buffer[(*count)++] = current_inumber;
		printf("Buffer Counter [after]: %d\n", *count);
	}

	return current_inumber;
}

int move(char* name1, char* name2, int* buffer){
	
	delete(name1, buffer);
	create(name2, T_FILE, buffer);
	return SUCCESS;
}


/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
void print_tecnicofs_tree(FILE *fp){
	inode_print_tree(fp, FS_ROOT, "");
}
