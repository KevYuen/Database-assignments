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
const int SLOT_SIZE = 1000;	// Size of a record
const int MAX_LINE_LEN = 10000;

int main(int argc, char *argv[])
{
	if (argc != 4) {
		cerr <<
		    "Usage: csv2heapfile <csv_file> <heap_file> <page_size>"
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

	while (fgets(line, MAX_LINE_LEN, csv_file)) {
		Record rec;
		// split csv and push into record
		load_csv_record(&rec, line);

		// Allocate a new page if necessary
		if (!page || fixed_len_page_freeslots(page) == 0) {
			if (page) {	// page is full
				PageID pid = alloc_page(&heapfile);
				write_page(page, &heapfile, pid);
				//Free page here
				free_page(page);
			}
			page = new Page;
			init_fixed_len_page(page, page_size, SLOT_SIZE);
		}
		// add record to page
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
	PageID pid = alloc_page(&heapfile);
	write_page(page, &heapfile, pid);
	free_page(page);
	fclose(csv_file);
	fclose(heapfd);
}
