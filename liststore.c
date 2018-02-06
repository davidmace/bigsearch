
// NOTE: memory is [size,elem,elem,garbage,garbage]
// and file1 is just elem,elem,elem,elem

// TODO: I should write to a buffer instead of direct file for adding small lists to end.

// TODO: maintain a queue of pointers to clear locations in addition to end pointer so I can recycle places in the file.

// NOTE: Preset all bytes to 0 then if you're ever overwriting a non-zero byte then you know 

// NOTE: it's only not a problem to check at mod == 0 as end of list because there are never size 0 lists within any file (since they get loaded into the special one element file)

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include "liststore.h"

#define LIST_EXPANSION_BYTES (1 << 22)

int compress(int position, int filenum) {
	return (position << 4) + filenum;
}

void maybe_expand_file(ListStore *ls, int filenum, int addition_bytes) {
	/*if (ls->file_vals_stored[filenum] * 4 + addition_bytes >= ls->file_size[filenum] ) {
		lseek(ls->files[filenum], ls->file_size[filenum] + LIST_EXPANSION_BYTES, SEEK_SET);
		char zero = 0;
		write(ls->files[filenum], &zero, 1);
		ls->file_size[filenum] += LIST_EXPANSION_BYTES;
		printf("list: expanded\n");
	}*/
}

int ls_start_list(ListStore *ls, int value) {
	maybe_expand_file(ls, 0, 4);
	lseek(ls->files[0], ls->file_vals_stored[0]*4, SEEK_SET);
	write(ls->files[0], &value, 4);
	ls->file_vals_stored[0] += 1;
	int npos = ls->file_vals_stored[0];
	return compress(npos,0);
}

int move_and_write(ListStore *ls, int filenum, int position, int max_list_len, int value) {
	// get bytes from old list
	lseek(ls->files[filenum], (position-max_list_len)*4, SEEK_SET);
	char* l = (char*)malloc( (max_list_len+1) * 4);
	read(ls->files[filenum], l, max_list_len*4);

	// add new value
	int *ptr = (int*)(l+max_list_len*4);
	*ptr = value;

	// write bytes to new list
	maybe_expand_file(ls, filenum + 1, (max_list_len << 1) * 4);
	lseek(ls->files[filenum+1], ls->file_vals_stored[filenum+1]*4, SEEK_SET);
	write(ls->files[filenum+1], l, (max_list_len+1)*4);
	int nposition = ls->file_vals_stored[filenum+1] + max_list_len + 1;
	ls->file_vals_stored[filenum+1] += max_list_len << 1;
	free(l);
	return nposition;
}

int ls_add_element(ListStore *ls, int filenum_and_position, int value) {
	int filenum = filenum_and_position & 0x0f;
	int position = filenum_and_position >> 4;

	int max_list_len = 1 << filenum;
	int list_len = position % max_list_len;
	if (list_len == 0) { // list is full
		int nposition = move_and_write(ls, filenum, position, max_list_len, value);
		return compress(nposition, filenum+1);
	}
	else {
		maybe_expand_file(ls, filenum, 4);
		lseek(ls->files[filenum], position*4, SEEK_SET);
		write(ls->files[filenum], &value, 4);
		return compress(position+1, filenum);
	}
}

int* ls_get_list(ListStore *ls, int filenum_and_position) {
	int filenum = filenum_and_position & 0x0f;
	int position = filenum_and_position >> 4;

	int start = ( (position-1) >> filenum) << filenum;
	lseek(ls->files[filenum], start*4, SEEK_SET);
	char* list_bytes = (char*)malloc( (position-start)*4 + 4 ); // last 4 bytes for end delimeter
	read(ls->files[filenum], list_bytes, (position-start)*4);
	int* list = (int*)list_bytes;
	list[position-start] = -1; // end delimiter
	return list;
}

ListStore* ls_init() {
	int num_lists = 20;

	ListStore *ls = (ListStore*)malloc(sizeof(ListStore));
	ls->files = (int*)malloc(num_lists * sizeof(int));
	ls->file_vals_stored = (int*)malloc(num_lists * sizeof(int));
	ls->file_size = (int*)malloc(num_lists * sizeof(int));
	char bucket[LIST_EXPANSION_BYTES];
	int i;
	for (i = 0; i < LIST_EXPANSION_BYTES; i++) {
		bucket[i] = 0;
	}
	for (i = 0; i < num_lists; i++) {
		char str_i[5];
		sprintf(str_i, "%d", i);
		char fn[20] = "list";
		strcat(fn, str_i);
		//strcat(fn,".storage");
		ls->files[i] = open(fn, O_RDWR | O_TRUNC | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
		//printf("%i %s\n", ls->files[i], strerror(errno));
		int res = write(ls->files[i], bucket, LIST_EXPANSION_BYTES);
		//printf("%i %s\n", res, strerror(errno));
		ls->file_vals_stored[i] = 0;
		ls->file_size[i] = LIST_EXPANSION_BYTES;
		fsync(ls->files[i]); // TODO do I need this?
	}
	return ls;
}


static void test() {

	ListStore *ls = ls_init();
	printf("%i %i\n",ls->files[0],ls->file_vals_stored[0]);

	int nfile_and_npos;

	nfile_and_npos = ls_start_list(ls,26);
	nfile_and_npos = ls_add_element(ls,nfile_and_npos,27);
	nfile_and_npos = ls_add_element(ls,nfile_and_npos,28);
	nfile_and_npos = ls_add_element(ls,nfile_and_npos,29);
	nfile_and_npos = ls_add_element(ls,nfile_and_npos,30);
	nfile_and_npos = ls_add_element(ls,nfile_and_npos,31);
	nfile_and_npos = ls_add_element(ls,nfile_and_npos,32);
	nfile_and_npos = ls_add_element(ls,nfile_and_npos,33);

	int* l = ls_get_list(ls,nfile_and_npos);
	printf("%i\n",l[0]);
	printf("%i\n",l[1]);
	printf("%i\n",l[2]);
	printf("%i\n",l[3]);
	printf("%i\n",l[4]);
	printf("%i\n",l[5]);
	printf("%i\n",l[6]);
	printf("%i\n",l[7]);

	//int answer = add_element(nfile,npos,28);
	//nfile = 
	//npos = 
	//(nfile,npos) = add_element(nfile,npos,29);
	//(nfile,npos) = add_element(nfile,npos,30);
	//(nfile,npos) = add_element(nfile,npos,31);


}


//int main() {
//	test();
//}








