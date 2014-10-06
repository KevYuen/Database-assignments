#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include "record.h"
#include "page.h"
#include "heapfile.h"

#include <sys/timeb.h>
#include <vector>

using namespace std;
const int MAX_LINE_LEN = 10000;

/*
 * Split line on ',' and insert tokens into record
 */
void load_record(Record * record, char line[])
{
	char *token;
	char *attribute;
	token = strtok(line, ",");
	while (token != NULL) {
		//printf ("%s\n",token);
		attribute = new char[ATTRIBUTE_SIZE + 1];

		strncpy(attribute, token, ATTRIBUTE_SIZE);
		attribute[ATTRIBUTE_SIZE] = '\0';
		record->push_back(attribute);

		token = strtok(NULL, ",");
	}
	assert(record->size() == 100);	// I think...
}

int main(int argc, char *argv[])
{
	if (argc != 4) {
		cerr <<
		    "Usage: insert <heapfile> <csv_file> <page_size>"
		    << endl;
		return (1);
	}

	FILE *csv_file = fopen(argv[1], "r");
	FILE *heapfd = fopen(argv[2], "w");
	int page_size = atoi(argv[3]);

	if (!csv_file || !heapfd || !(page_size > 0)) {
		cerr << "FAIL" << endl;
		return (1);
	}

	Heapfile heapfile;
	init_heapfile(&heapfile, page_size, heapfd);

	Page *page = NULL;

	char line[MAX_LINE_LEN];
	PageID pid;

	while (fgets(line, MAX_LINE_LEN, csv_file)) {
		Record rec;
		// split csv and push into record
		load_record(&rec, line);

		// Get a page with free space if necessary
		if (!page || fixed_len_page_freeslots(page) == 0) 
		{
			if (page) { // page is full
				// Write page to heap again
				write_page(page, &heapfile, pid);

				//Free page here
				delete[](char *)page->data;
				delete page;
			}
			
			// Get a page with free space
			pid = get_Free_Page(&heapfile);
			page = new Page;
			read_page(&heapfile, pid, page);
		}

		// Insert record into page
		if (add_fixed_len_page(page, &rec) == -1) {
			// Something went wrong.
		}

		//Free char arrays in record
		for (int i = 0; i < rec.size(); i++) {
			//printf("attr:%s\n", rec[i]);
			delete[]rec[i];
		}
	}

	// Write the last page to file. 
	write_page(page, &heapfile, pid);
	fclose(csv_file);
	fclose(heapfd);
}
