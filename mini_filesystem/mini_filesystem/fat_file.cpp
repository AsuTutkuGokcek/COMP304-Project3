#include "fat.h"
#include "fat_file.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

// Little helper to show debug messages. Set 1 to 0 to silence.
#define DEBUG 1
inline void debug(const char * fmt, ...) {
#if DEBUG>0
	va_list args;
   va_start(args, fmt);
   vprintf(fmt, args);
   va_end(args);
#endif
}

// Delete index-th item from vector.
template<typename T>
static void vector_delete_index(std::vector<T> &vector, const int index) {
	vector.erase(vector.begin() + index);
}

// Find var and delete from vector.
template<typename T>
static bool vector_delete_value(std::vector<T> &vector, const T var) {
	for (int i=0; i<vector.size(); ++i) {
		if (vector[i] == var) {
			vector_delete_index(vector, i);
			return true;
		}
	}
	return false;
}

void mini_file_dump(const FAT_FILESYSTEM *fs, const FAT_FILE *file)
{
	printf("Filename: %s\tFilesize: %d\tBlock count: %d\n", file->name, file->size, (int)file->block_ids.size());
	printf("\tMetadata block: %d\n", file->metadata_block_id);
	printf("\tBlock list: ");
	for (int i=0; i<file->block_ids.size(); ++i) {
		printf("%d ", file->block_ids[i]);
	}
	printf("\n");

	printf("\tOpen handles: \n");
	for (int i=0; i<file->open_handles.size(); ++i) {
		printf("\t\t%d) Position: %d (Block %d, Byte %d), Is Write: %d\n", i,
			file->open_handles[i]->position,
			position_to_block_index(fs, file->open_handles[i]->position),
			position_to_byte_index(fs, file->open_handles[i]->position),
			file->open_handles[i]->is_write);
	}
}


/**
 * Find a file in loaded filesystem, or return NULL.
 */
FAT_FILE * mini_file_find(const FAT_FILESYSTEM *fs, const char *filename)
{
	for (int i=0; i<fs->files.size(); ++i) {
		if (strcmp(fs->files[i]->name, filename) == 0) // Match
			return fs->files[i];
	}
	return NULL;
}

/**
 * Create a FAT_FILE struct and set its name.
 */
FAT_FILE * mini_file_create(const char * filename)
{
	FAT_FILE * file = new FAT_FILE;
	file->size = 0;
	strcpy(file->name, filename);
	return file;
}


/**
 * Create a file and attach it to filesystem.
 * @return FAT_OPEN_FILE pointer on success, NULL on failure
 */
FAT_FILE * mini_file_create_file(FAT_FILESYSTEM *fs, const char *filename)
{
	assert(strlen(filename)< MAX_FILENAME_LENGTH);
	FAT_FILE *fd = mini_file_create(filename);

	int new_block_index = mini_fat_allocate_new_block(fs, FILE_ENTRY_BLOCK);
	if (new_block_index == -1)
	{
		fprintf(stderr, "Cannot create new file '%s': filesystem is full.\n", filename);
		return NULL;
	}
	fs->files.push_back(fd); // Add to filesystem.
	fd->metadata_block_id = new_block_index;
	return fd;
}

/**
 * Return filesize of a file.
 * @param  fs       filesystem
 * @param  filename name of file
 * @return          file size in bytes, or zero if file does not exist.
 */
int mini_file_size(FAT_FILESYSTEM *fs, const char *filename) {
	FAT_FILE * fd = mini_file_find(fs, filename);
	if (!fd) {
		fprintf(stderr, "File '%s' does not exist.\n", filename);
		return 0;
	}
	return fd->size;
}


/**
 * Opens a file in filesystem.
 * If the file does not exist, returns NULL, unless it is write mode, where
 * the file is created.
 * Adds the opened file to file's open handles.
 * @param  is_write whether it is opened in write (append) mode or read.
 * @return FAT_OPEN_FILE pointer on success, NULL on failure
 */
FAT_OPEN_FILE * mini_file_open(FAT_FILESYSTEM *fs, const char *filename, const bool is_write)
{
	FAT_FILE * fd = mini_file_find(fs, filename);
	

	if (!fd) {
		// TODO: check if it's write mode, and if so create it. Otherwise return NULL.

		if(is_write){
			fd = mini_file_create_file(fs, filename);
			if(fd==NULL){
				fprintf(stderr,"ERROR");
				return NULL;
			}
		}else{
			fprintf(stderr,"ERROR");
			return NULL;
		}
	
	}

	if (is_write) {
		// TODO: check if other write handles are open.
		for(int i = 0; i < fd->open_handles.size(); i++){
		if(fd->open_handles.at(i)->is_write==true){
			fprintf(stderr,"ERROR");

			return NULL;
		}
	}
	}

	FAT_OPEN_FILE * open_file = new FAT_OPEN_FILE;
	// TODO: assign open_file fields.

	open_file->position=0;
	open_file->file=fd;
	open_file->is_write=is_write;

	// Add to list of open handles for fd:
	fd->open_handles.push_back(open_file);
	return open_file;
}

/**
 * Close an existing open file handle.
 * @return false on failure (no open file handle), true on success.
 */
bool mini_file_close(FAT_FILESYSTEM *fs, const FAT_OPEN_FILE * open_file)
{
	if (open_file == NULL) return false;
	FAT_FILE * fd = open_file->file;
	if (vector_delete_value(fd->open_handles, open_file)) {
		return true;
	}

	fprintf(stderr, "Attempting to close file that is not open.\n");
	return false;
}

/**
 * Write size bytes from buffer to open_file, at current position.
 * @return           number of bytes written.
 */
int mini_file_write(FAT_FILESYSTEM *fs, FAT_OPEN_FILE * open_file, const int size, const void * buffer)
{

	

	int written_bytes = 0;

	// TODO: write to file.

	FAT_FILE *file = open_file->file;
	int POS = open_file->position;

	int block_index = position_to_block_index(fs, POS);
	int byte_index = position_to_byte_index(fs, POS);

	if(size + block_index <= fs->block_size){

		file->size += mini_fat_write_in_block(fs, block_index, byte_index, size, buffer);


	}
	else{

		file->size += mini_fat_write_in_block(fs, block_index, byte_index, fs->block_size - block_index, buffer);
	
		mini_file_write(fs, open_file , size - (fs->block_size - block_index), buffer + (fs->block_size - block_index));

		
	}


	return size;



}

/**
 * Read up to size bytes from open_file into buffer.
 * @return           number of bytes read.
 */
int mini_file_read(FAT_FILESYSTEM *fs, FAT_OPEN_FILE * open_file, const int size, void * buffer)
{	

	

	int read_bytes = 0;

	// TODO: read file.
	 

	FAT_FILE *file = open_file->file;
	int POS = open_file->position;
	int block_index = position_to_block_index(fs, POS);
	int byte_index = position_to_byte_index(fs, POS);

	if(size + block_index <= fs->block_size){

		read_bytes = mini_fat_read_in_block(fs, block_index, byte_index, size, buffer);


	}
	else{

		read_bytes = mini_fat_read_in_block(fs, block_index, byte_index, fs->block_size - block_index, buffer);

		

		read_bytes += mini_file_read(fs, open_file , size - (fs->block_size - block_index), buffer + (fs->block_size - block_index));

		
		
	}

	return read_bytes;
	
}


/**
 * Change the cursor position of an open file.
 * @param  offset     how much to change
 * @param  from_start whether to start from beginning of file (or current position)
 * @return            false if the new position is not available, true otherwise.
 */
bool mini_file_seek(FAT_FILESYSTEM *fs, FAT_OPEN_FILE * open_file, const int offset, const bool from_start)
{
	// TODO: seek and return true.

	int newPos = 0;
	int size = open_file->file->size;
	if (from_start){
		newPos = offset;
	}else{
		newPos = open_file->position + offset;
	}

	if (newPos > size || newPos < 0){
		return false;
	}

	open_file->position = newPos;
	
	return true;

}

/**
 * Attemps to delete a file from filesystem.
 * If the file is open, it cannot be deleted.
 * Marks the blocks of a deleted file as empty on the filesystem.
 * @return true on success, false on non-existing or open file.
 */
bool mini_file_delete(FAT_FILESYSTEM *fs, const char *filename)
{
	// TODO: delete file after checks.

	t_FAT_FILE *file = mini_file_find(fs, filename);


	if (file == NULL)
	{
		printf("No file found with name %s", filename);
		return false;
	}
	for(int i=0; i<file->open_handles.size(); i++){
	if (file->open_handles[i]->is_write==true)
	{
		printf("The file named %s is open. Therefore, it can't be deleted", filename);
		return false;
	}
	}

	vector_delete_value(fs->files, file);

	return true;
}