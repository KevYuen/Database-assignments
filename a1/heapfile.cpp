#include "heapfile.h"

#include <stdio.h>
#include <cassert>
#include <cstdlib>
#include <cmath>

/**
 * Initalize a heapfile to use the file and page size given.
 */
void init_heapfile(Heapfile *heapfile, int page_size) {
	assert(page_size > 1001);

	heapfile->page_size = page_size;
}

/**
 * Create the initial structure of a heapfile
 * This only needs to be used when creating new heapfile
 */
void create_heapfile(Heapfile* heapfile, char *filename) {
	//(w+) Create an empty file and open it for input and output.
	// If a file with the same name already exists its contents
	// are discarded and the file is treated as a new empty file.
	heapfile->file_ptr = fopen(filename, "w+");
	assert(heapfile->file_ptr);
	//create the directory page
	Directory *directory = new Directory;
	// First page of heapfile is a directory.
	init_directory_page(directory, heapfile->page_size, 0);
	write_block(heapfile, (char *)directory->data, 0);

	//  directories are buffered
	heapfile->directory_buffer.push_back(directory);
	//heapfile->directory.push_back(directory);
}

void open_heapfile(Heapfile *heapfile, char *filename) {
	//(r+) Open a file for input and output. The file must exist.
	heapfile->file_ptr = fopen(filename, "r+");
	assert(heapfile->file_ptr);


	int offset = 0;
	do {
	//create the directory page
	Directory *directory = new Directory;
	// First page of heapfile is a directory.
	init_directory_page(directory, heapfile->page_size, offset);
	read_block(heapfile, (char *)directory->data, offset);

	//  directories are buffered
	heapfile->directory_buffer.push_back(directory);
	offset = *directory->next_directory;
	} while (offset != 0);
}

void close_heapfile(Heapfile *heapfile) {

	//write out directory buffer
	int num_directories = heapfile->directory_buffer.size();
	for (int dir_no=0; dir_no < num_directories; dir_no++){
		Directory *dir = heapfile->directory_buffer[dir_no];
		write_block(heapfile, (char *)dir->data, dir->offset);

		delete[] (char *)dir->data;
		delete dir;
	}

	fclose(heapfile->file_ptr);
	delete heapfile;
}

void put_record(Heapfile* heapfile, Record *rec) {

	printf("Put record:\n");
	PageID pid = get_free_pid(heapfile);

	printf("   get_free_pid=%d\n", pid);
	Page *page = new Page;
	init_fixed_len_page(page, heapfile->page_size, SLOTSIZE);
	get_page(heapfile, pid, page);
	add_fixed_len_page(page, rec);
	commit_page(heapfile, page, pid);

	free_page(page);
}

void get_record(Heapfile* heapfile, RecordID *rid, Record *rec) {
	Page *page = new Page;
	init_fixed_len_page(page, heapfile->page_size, SLOTSIZE);
	get_page(heapfile, rid->page_id, page);
	read_fixed_len_page(page, rid->slot, rec);

	printf("getrecord:%d\n", rid->page_id);
	free_page(page);
}

void update_record(Heapfile* heapfile, RecordID *rid, Record *rec) {

	Page *page = new Page;
	init_fixed_len_page(page, heapfile->page_size, SLOTSIZE);
	get_page(heapfile, rid->page_id, page);
	write_fixed_len_page(page, rid->slot, rec);
	commit_page(heapfile, page, rid->page_id);

	free_page(page);
}

void delete_record(Heapfile *heapfile, RecordID *rid){

	Page *page = new Page;
	init_fixed_len_page(page, heapfile->page_size, SLOTSIZE);

	get_page(heapfile, rid->page_id, page);

	delete_fixed_len_page(page, rid->slot);
	
	commit_page(heapfile, page, rid->page_id);
	free_page(page);
}

/* Search existing pages with free space.
 * May allocate new page
 */
PageID get_free_pid(Heapfile *heapfile) {

	printf("get free pid:\n");

	// return pid with a vacant slot or -1 if none available
	PageID pid = vacant_page_id(heapfile);
	if (pid == -1) {
		//no free pages available so allocate new page.
		pid = alloc_page(heapfile);
	}

	return pid;
}

void get_page(Heapfile *heapfile, PageID pid, Page *page) {
	//if pid in cache: return page
	//else:

	Entry *entry = get_entry(heapfile, pid);

	//write out cache in buffer
	//free old page
	//load new page

	printf("get page: pid=%d  offset=%d\n", pid, entry->offset);
	read_block(heapfile, (char*)page->data, entry->offset);
	//push into cache
}
/**
 * Allocate another page in the heapfile.  This grows the file by a page.
 */
PageID alloc_page(Heapfile *heapfile){

	printf("alloc page:\n");
	Entry entry;
	Page *page = new Page; 	

	//Create the new page
	init_fixed_len_page(page, heapfile->page_size, SLOTSIZE);

	//assign entry pointers
	fseek(heapfile->file_ptr, 0, SEEK_END);
	entry.offset = ftell(heapfile->file_ptr);
    entry.free_space = fixed_len_page_freeslots(page);

    //write the page out
    write_block(heapfile, (char*)page->data, entry.offset);

	// add the new entry into entry list
    PageID pid = append_entry(heapfile, &entry);

	printf("alloc page: pid=%d  offset=%d\n", pid, entry.offset);

    heapfile->num_pages++;
    // push page to cache
    free_page(page);
	return pid;
}

/**
 * Read a page size block into memory
 */
void read_block(Heapfile *heapfile, char *buffer, int offset){
	fseek(heapfile->file_ptr, offset, SEEK_SET);

	int result = fread(buffer, 1, heapfile->page_size, heapfile->file_ptr);
	if(result != heapfile->page_size){
		perror("read_block");
		printf("panic panic, can't read block(%d)\n", result);
		exit(1);
	}

	printf("-----------------------------------read block(%d - %d)\n", offset, offset+result);

}

/**
 * Write a page size block from memory to disk
 */
void write_block(Heapfile *heapfile, char *buffer, int offset){

	fseek(heapfile->file_ptr, offset, SEEK_SET);

	int result = fwrite(buffer, 1, heapfile->page_size, heapfile->file_ptr);

	if (result != heapfile->page_size){
		perror("write block");
		printf("panic panic, can't write block(%d)\n", result);
		exit(1);
	}

	printf("----------------------------------wrote block(%d - %d)\n", offset, offset+result);
	fflush(heapfile->file_ptr);
}

void commit_page(Heapfile *heapfile, Page *page, PageID pid) {

	Entry *entry = get_entry(heapfile, pid);

	entry->free_space = fixed_len_page_freeslots(page);

	printf("Commit page(%d)\n", entry->offset);
	write_block(heapfile, (char*)page->data, entry->offset);
}


PageID append_entry(Heapfile *heapfile, Entry *entry) {

	printf("append entry:\n");
	int num_directories = heapfile->directory_buffer.size();
	int dir_no;
	for (dir_no=0; dir_no < num_directories; dir_no++){
		Directory *dir = heapfile->directory_buffer[dir_no];
		if (!is_full(dir)) {
			int slot_no = next_vacant_slot(dir);
			dir->entries[slot_no] = *entry;

			printf("appendentry ret1:(%d,%d,%d)\n", dir_no , dir->capacity , slot_no);
			return dir_no * dir->capacity + slot_no;
		}
	}

	// no capacity. must alloc new direcotry
	Directory *last_dir = heapfile->directory_buffer[num_directories-1];

	fseek(heapfile->file_ptr, 0, SEEK_END);
	int offset =  ftell(heapfile->file_ptr);
	*last_dir->next_directory = offset;
	Directory *new_dir = new Directory;
	init_directory_page(new_dir, heapfile->page_size, offset);
    write_block(heapfile, (char*)new_dir->data, offset);

    heapfile->directory_buffer.push_back(new_dir);
	int slot_no = next_vacant_slot(new_dir);
	new_dir->entries[slot_no] = *entry;

	printf("appendentry ret2:(%d)\n", (num_directories) * new_dir->capacity + slot_no);
	return (num_directories) * new_dir->capacity + slot_no;
//
}


	// get entry from pid
Entry *get_entry(Heapfile *heapfile, PageID pid) {

	printf("get entry:(%d)\n", pid);
	int capacity = heapfile->directory_buffer[0]->capacity;

	printf("  Capacity(%d)\n", capacity);

	int dir_no = pid / capacity;
	int slot_no = pid % capacity;

	Directory *dir = heapfile->directory_buffer[dir_no];
	assert(slot_no < *dir->n_entries);
	return & dir->entries[slot_no];
}

PageID vacant_page_id(Heapfile *heapfile){

	printf("vacant page id:\n");
	int num_directories = heapfile->directory_buffer.size();

	//check every directory page
	for (int dir_no=0; dir_no < num_directories; dir_no++){
		Directory *dir = heapfile->directory_buffer[dir_no];
		// check every slot
		int slot_no = find_free_page(dir);
		if (slot_no != -1) {
			return dir_no * dir->capacity + slot_no;
		}
	}
	// No vacant pages available.
	return -1;
}

Record RecordIterator::next(){
	Page *page = new Page;
	Record record(100);

	init_fixed_len_page(page, heapfile->page_size, SLOTSIZE);
	get_page(heapfile, pid, page);
	read_fixed_len_page(page, slot, &record);
	free_page(page);
	
	// Increment iterator
	int capacity = fixed_len_page_capacity(page);
	slot++;
	if (slot >= capacity) {
		slot=0;
		pid++;
	}

	int capacity2 = heapfile->directory_buffer[0]->capacity;
	
	int dir_no = pid / capacity2;
	int slot_no = pid % capacity2;
	Directory *dir = heapfile->directory_buffer[dir_no];
	
	if(!(slot_no < *dir->n_entries)) {
		has_next = false;
	} else {
		Page *page2 = new Page;
		init_fixed_len_page(page2, heapfile->page_size, SLOTSIZE);
		get_page(heapfile, pid, page2);

		int index = page2->page_size - ceil((slot+1) / 8.0);
		int valid = ((char *)page2->data)[index] & (1 << (slot % 8));
		
		has_next = (valid > 0);
		free_page(page2);
	}
	

	return record;
}

RecordID RecordIterator::currentRID(){
	RecordID rid;
	rid.page_id = pid;
	rid.slot = slot;
	return rid;
}