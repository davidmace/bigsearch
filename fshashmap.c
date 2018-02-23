#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include "fshashmap.h"

#define PAGE_SIZE 128 // NOTE I store page size in char so this cant get bigger than 256

// NOTE: I can only terminate with a 0 byte because no character is 0
// but it will be problematic for storing arbitrary integers 

// NOTE: Store list number in addition to pointer so I know which list the pointer refers to.
// It's not any more efficient to store them all in one list because (assuming equal list size) the pointer would then be bigger by the same number of bits as the extra byte stored.

// Improvement: compress string by turning it into base 27 (with no 0) instead of base 256

// NOTE: a bucket for the hashmap is the page size.
// Bucket structure: size key ptr key ptr
// keys are dynamic size so \0 end them

// TODO might be slow if I'm editing off-alignment

// Note: this is a duplicated function
void clear_string2(char* s, int len) {
	int i;
	for (i = 0; i < len; i++) {
		s[i] = '\0';
	}
}

int starts = 0;
int reads = 0;

static void read_bucket(int file, int bucket_num, char* bucket) {
	lseek(file, bucket_num * PAGE_SIZE, SEEK_SET);
	read(file, bucket, PAGE_SIZE);
}

static void write_bucket(int file, int bucket_num, char* bucket) {
	lseek(file, bucket_num * PAGE_SIZE, SEEK_SET);
	write(file, bucket, PAGE_SIZE);
}

static void add_key_value_to_bucket(FSHashMap *hm, int bucket_num, char* key, int value) {
	int write_bucket_num = hm->hash2write_bucket[bucket_num]; // in case we should write to overflow
	size_t key_len = strlen(key);
	char* bucket = (char*)malloc(PAGE_SIZE * sizeof(char));
	read_bucket(hm->file, write_bucket_num, bucket);
	unsigned char size = (unsigned char)bucket[0];
	
	// if we have just overflowed the bucket
	if (size + key_len + 1 + 4 > PAGE_SIZE) {
		// write in the old bucket the pointer to the new bucket
		int * overflow_ptr_location = (int*)(bucket+1);
		*overflow_ptr_location = hm->next_overflow_bucket;
		write_bucket(hm->file, write_bucket_num, bucket);

		// set the bucket to write to the new bucket
		bucket[0] = 5;
		int i;
		for (i = 1; i < PAGE_SIZE; i++) {
			bucket[i] = 0;
		}

		write_bucket_num = hm->next_overflow_bucket;
		hm->next_overflow_bucket += 1;
		hm->hash2write_bucket[bucket_num] = write_bucket_num;
		size = 5;
		if (hm->next_overflow_bucket >= hm->num_buckets * 1.3) {
			printf("%i %i\n", hm->next_overflow_bucket, hm->num_buckets);
			expand_hm(hm);
		}

		// write the word that would have overflowed the bucket but now in the proper new hm bucket
		hm_add(hm, key, value);
	}
	else {
		// write the key's characters and the 0 terminating byte
		int i;
		for (i = 0; i < key_len; i++) {
			bucket[size+i] = key[i];
		}
		bucket[size+key_len+0] = 0;

		// write the value
		int * intlocation = (int*)(bucket+size+key_len+1);
		*intlocation = value;

		// append the size
		size += key_len+4+1;
		bucket[0] = (char)size;

		write_bucket(hm->file, write_bucket_num, bucket);
		free(bucket);
	}
}


static PosAndVal* get_value_from_key(FSHashMap *hm, int bucket_num, char* key) {
	char matching = 1;
	char* bucket = (char*)malloc(PAGE_SIZE * sizeof(char));
	//starts += 1;

	while (1) {
		//printf("yo %i\n", bucket_num);
		//reads += 1;
		read_bucket(hm->file, bucket_num, bucket);
		unsigned char size = (unsigned char)bucket[0];

		int key_i = 0;
		int i;
		for (i = 5; i < size; i++) {
			//printf("%c %c\n",bucket[i],key[key_i]);
			if (bucket[i] == 0) { // finished key

				// found the key and next int is the value
				if (matching == 1 && key_i == strlen(key)) {
					int *answer_ptr = (int*)(bucket+i+1);
					PosAndVal *pos_and_val = (PosAndVal*)malloc(sizeof(PosAndVal));
					pos_and_val->hm_pos = bucket_num * PAGE_SIZE + i + 1;
					pos_and_val->ls_file_and_pos = *answer_ptr;
					free(bucket);
					return pos_and_val;
				}

				// finished the key but it isn't a match
				else {
					matching = 1;
					key_i = -1;
					i += 4; // skip the value in the next 4 bits
				}

			}

			// bucket's character doesn't match to key's character
			else if (key_i >= strlen(key) || bucket[i] != key[key_i]) {
				matching = 0;
			}
			key_i += 1;
		}

		// reached end of last bucket
		int overflow_ptr = *( (int*)(bucket+1) );
		if (overflow_ptr == 0) {
			break;
		}

		bucket_num = overflow_ptr;
	}
	free(bucket);
	return NULL;
}

static int key2hash_old(char* key, int num_buckets) {
	//printf("%i\n",strlen(key));
	int sm = 0;
	int i;
	for (i = 0; i < strlen(key); i++) {
		sm += key[i];
		sm = (27 * sm) % num_buckets;
		//printf("%i %i %i\n",sm,key[i],(27 * sm) % num_buckets);
	}
	//printf("\n",sm);
	int hash = sm % num_buckets;
	return hash;
}

static int key2hash(char *str, int num_buckets)
{
    unsigned long hash = 5381;
    int c;

    while ( (c = *str++) )
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    //printf("%i\n",hash % num_buckets);
    return (int)(hash % num_buckets);
}

void clear_hm(FSHashMap *hm, int hm_num, int num_buckets) {
	hm->num_buckets = num_buckets;
	hm->file_size = num_buckets * PAGE_SIZE;
	hm->hm_num = hm_num;

	char str[30];
	sprintf(str, "storage/hm%i", hm_num);
	int file = open(str, O_RDWR | O_TRUNC | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	hm->file = file;

	// all hashes initially write to their main bucket rather than overflow
	hm->hash2write_bucket = (int*)malloc(num_buckets * sizeof(int));
	int i;
	for(i = 0; i < num_buckets; i++) {
		hm->hash2write_bucket[i] = i;
	}
	hm->next_overflow_bucket = num_buckets;

	// init all buckets
	char bucket[PAGE_SIZE];
	bucket[0] = 5;
	for (i = 1; i < PAGE_SIZE; i++) {
		bucket[i] = 0;
	}
	for (i = num_buckets * 1.3 - 1; i >= 0; i--) {
		write_bucket(hm->file, i, bucket);
	}

}

void expand_hm(FSHashMap *hm) {
	printf("EXPAND HM\n");
	//int file_new = open("storage/hm"+str(hm->hm_num+1), O_RDWR | O_TRUNC | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	
	// expand the file to full size
	//lseek(file_new, hm->num_buckets * PAGE_SIZE, SEEK_SET);
	//char zero = 0;
	//write(file_new, &zero, 1);
	int old_file = hm->file;
	int old_num_buckets = hm->num_buckets;
	clear_hm(hm, hm->hm_num + 1, hm->num_buckets * 3);

	char* bucket = (char*)malloc(PAGE_SIZE * sizeof(char));
	int bucket_num;
	for (bucket_num = 0; bucket_num < floor(old_num_buckets * 1.3); bucket_num++) {
		read_bucket(old_file, bucket_num, bucket);

		// read key,val in buckets
		char* key = (char*)malloc(PAGE_SIZE * sizeof(char));
		clear_string2(key, PAGE_SIZE);
		int key_i = 0;
		int bucket_i;
		unsigned char this_bucket_size = (unsigned char)bucket[0];
		//printf("START BUCKET %i %i\n", bucket_num, this_bucket_size);
		for (bucket_i = 5; bucket_i < this_bucket_size; bucket_i++) {
			if (bucket[bucket_i] == '\0') {
				int val = *((int*)(bucket + bucket_i + 1));
				hm_add(hm, key, val);
				//if (strncmp(key,"third",5) == 0)
				//if (strncmp(key,"twentysixth",11) == 0)
					//return;
				//printf("%s %i\n", key, val);
				key_i = 0;
				bucket_i += 4;
				clear_string2(key, PAGE_SIZE);
			}
			else {
				key[key_i] = bucket[bucket_i];
				key_i += 1;
			}
		}
	}
	//printf("DONE EXPAND\n");

}

void print_hm(FSHashMap *hm) {
	printf("\nPRINT_HM\n");
	//int file_new = open("storage/hm"+str(hm->hm_num+1), O_RDWR | O_TRUNC | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	
	// expand the file to full size
	//lseek(file_new, hm->num_buckets * PAGE_SIZE, SEEK_SET);
	//char zero = 0;
	//write(file_new, &zero, 1);
	int old_file = hm->file;
	int old_num_buckets = hm->num_buckets;
	//clear_hm(hm, hm->hm_num + 1, hm->num_buckets * 3);

	char* bucket = (char*)malloc(PAGE_SIZE * sizeof(char));
	int bucket_num;
	for (bucket_num = 0; bucket_num < floor(old_num_buckets * 1.3); bucket_num++) {
		read_bucket(old_file, bucket_num, bucket);

		// read key,val in buckets
		char* key = (char*)malloc(PAGE_SIZE * sizeof(char));
		clear_string2(key, PAGE_SIZE);
		int key_i = 0;
		int bucket_i;
		unsigned char this_bucket_size = (unsigned char)bucket[0];
		//printf("START BUCKET %i %i\n", bucket_num, this_bucket_size);
		for (bucket_i = 5; bucket_i < this_bucket_size; bucket_i++) {
			if (bucket[bucket_i] == '\0') {
				int val = *((int*)(bucket + bucket_i + 1));
				//hm_add(hm, key, val);
				printf("%s %i\n", key, val);
				key_i = 0;
				bucket_i += 4;
				clear_string2(key, PAGE_SIZE);
			}
			else {
				key[key_i] = bucket[bucket_i];
				key_i += 1;
			}
		}
	}

}

FSHashMap* hm_init(int num_buckets) {
	FSHashMap *hm = (FSHashMap*)malloc(sizeof(FSHashMap));
	clear_hm(hm, 0, num_buckets);
	return hm;
}

void hm_add(FSHashMap *hm, char* key, int value) {
	//printf("hm_add, %s %i\n", key, key2hash(key, hm->num_buckets));
	int bucket_num = key2hash(key, hm->num_buckets);
	add_key_value_to_bucket(hm, bucket_num, key, value);
}

void hm_set(FSHashMap *hm, int pos, int value) {
	lseek(hm->file, pos, SEEK_SET);
	write(hm->file, &value, 4);
}

PosAndVal* hm_get(FSHashMap *hm, char* key) {
	int bucket_num = key2hash(key, hm->num_buckets);
	PosAndVal *answer = get_value_from_key(hm, bucket_num, key);
	return answer;
}

void print_stats() {
	printf("%f\n",1.0*reads/starts);
	starts = 0;
	reads = 0;
}



static void test() {
	
	FSHashMap *hm = hm_init(10);
	//hm_add(hm,"sunnyk",255);

	hm_add(hm,"cat",255);
	hm_add(hm,"car",254);
	hm_add(hm,"cas",253);
	hm_add(hm,"ca",252);
	hm_add(hm,"cata",251);
	hm_add(hm,"catb",250);
	hm_add(hm,"catc",249);

	PosAndVal *answer;
	answer = hm_get(hm,"cat");
	printf("%i\n",answer->ls_file_and_pos);
	answer = hm_get(hm,"car");
	printf("%i\n",answer->ls_file_and_pos);
	answer = hm_get(hm,"cas");
	printf("%i\n",answer->ls_file_and_pos);
	answer = hm_get(hm,"ca");
	printf("%i\n",answer->ls_file_and_pos);
	answer = hm_get(hm,"cata");
	printf("%i\n",answer->ls_file_and_pos);
	answer = hm_get(hm,"catb");
	printf("%i\n",answer->ls_file_and_pos);
	answer = hm_get(hm,"catc");
	printf("%i\n",answer->ls_file_and_pos);

	


	// put key values in
	/*char *key;
	int offset;

	key = "sunnyk";
	offset = key2hash(key);
	printf("offset %i\n",offset);
	add_key_value_to_bucket(file, offset, key, 1234560);

	key = "abc";
	offset = key2hash(key);
	printf("offset %i\n",offset);
	add_key_value_to_bucket(file, offset, key, 1234561);

	key = "inferno";
	offset = key2hash(key);
	printf("offset %i\n",offset);
	add_key_value_to_bucket(file, offset, key, 1234562);

	key = "s";
	offset = key2hash(key);
	printf("offset %i\n",offset);
	add_key_value_to_bucket(file, offset, key, 1234563);

	// get 
	key = "inferno";
	offset = key2hash(key);
	printf("offset %i\n",offset);
	int value = get_value_from_key(file, offset, key);
	printf("%i\n",value);*/

}

//int main() {
//	test();
//}






