#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "fshashmap.h"

#define PAGE_SIZE 128 // NOTE I store page size in char so this cant get bigger than 256
#define NUM_BUCKETS 100000
#define BUCKETS_PER_EXPANSION 100000

// NOTE: I can only terminate with a 0 byte because no character is 0
// but it will be problematic for storing arbitrary integers 

// NOTE: Store list number in addition to pointer so I know which list the pointer refers to.
// It's not any more efficient to store them all in one list because (assuming equal list size) the pointer would then be bigger by the same number of bits as the extra byte stored.

// Improvement: compress string by turning it into base 27 (with no 0) instead of base 256

// NOTE: a bucket for the hashmap is the page size.
// Bucket structure: size key ptr key ptr
// keys are dynamic size so \0 end them

// TODO might be slow if I'm editing off-alignment

static void read_bucket(FSHashMap *hm, int bucket_num, char* bucket) {
	lseek(hm->file, bucket_num * PAGE_SIZE, SEEK_SET);
	read(hm->file, bucket, PAGE_SIZE);
}

static void maybe_expand_file(FSHashMap *hm, int bucket_num) {
	/*if (bucket_num * PAGE_SIZE >= hm->file_size ) {
		lseek(hm->file, hm->file_size + PAGE_SIZE * BUCKETS_PER_EXPANSION, SEEK_SET);
		char zero = 0;
		write(hm->file, &zero, 1);
		hm->file_size += PAGE_SIZE * BUCKETS_PER_EXPANSION;
		printf("hashmap: expanded\n");
	}*/
}

static void write_bucket(FSHashMap *hm, int bucket_num, char* bucket) {
	maybe_expand_file(hm, bucket_num);
	lseek(hm->file, bucket_num * PAGE_SIZE, SEEK_SET);
	write(hm->file, bucket, PAGE_SIZE);
}

static void add_key_value_to_bucket(FSHashMap *hm, int bucket_num, char* key, int value) {
	int write_bucket_num = hm->hash2write_bucket[bucket_num]; // in case we should write to overflow
	size_t key_len = strlen(key);
	char* bucket = (char*)malloc(PAGE_SIZE * sizeof(char));
	read_bucket(hm, write_bucket_num, bucket);
	char size = bucket[0];
	
	// if we have just overflowed the bucket
	if (size + key_len + 1 + 4 > PAGE_SIZE) {
		// write in the old bucket the pointer to the new bucket
		int * overflow_ptr_location = (int*)(bucket+1);
		*overflow_ptr_location = hm->next_overflow_bucket;
		write_bucket(hm, write_bucket_num, bucket);

		// set the bucket to write to the new bucket
		bucket[0] = 5;
		int i;
		for (i = 0; i < PAGE_SIZE; i++) {
			bucket[i] = 0;
		}

		write_bucket_num = hm->next_overflow_bucket;
		hm->next_overflow_bucket += 1;
		hm->hash2write_bucket[bucket_num] = write_bucket_num;
		size = 5;
	}

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
	bucket[0] += key_len+4+1;

	write_bucket(hm, write_bucket_num, bucket);
	free(bucket);
}

static PosAndVal* get_value_from_key(FSHashMap *hm, int bucket_num, char* key) {
	char matching = 1;
	char* bucket = (char*)malloc(PAGE_SIZE * sizeof(char));

	while (1) {
		//printf("yo %i\n", bucket_num);
		read_bucket(hm, bucket_num, bucket);
		char size = bucket[0];

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

static int key2hash(char* key) {
	int sm = 0;
	int i;
	for (i = 0; i < strlen(key); i++) {
		sm += key[i];
		sm = (27 * sm) % NUM_BUCKETS;
	}
	int hash = sm % NUM_BUCKETS;
	return hash;
}


FSHashMap* hm_init() {
	int file = open("hm.storage", O_RDWR | O_TRUNC);
	FSHashMap *hm = (FSHashMap*)malloc(sizeof(FSHashMap));
	hm->file = file;
	hm->file_size = NUM_BUCKETS * PAGE_SIZE;
	hm->hash2write_bucket = (int*)malloc(NUM_BUCKETS * sizeof(int));
	int i;
	for(i = 0; i < NUM_BUCKETS; i++) {
		hm->hash2write_bucket[i] = i;
	}
  hm->next_overflow_bucket = NUM_BUCKETS;

  char bucket[PAGE_SIZE];
	bucket[0] = 5;
	for (i = 1; i < PAGE_SIZE; i++) {
		bucket[i] = 0;
	}
	for (i = 0; i < NUM_BUCKETS; i++) {
		write_bucket(hm, i, bucket);
	}

	return hm;
}

void hm_add(FSHashMap *hm, char* key, int value) {
	int bucket_num = key2hash(key);
	add_key_value_to_bucket(hm, bucket_num, key, value);
}

void hm_set(FSHashMap *hm, int pos, int value) {
	lseek(hm->file, pos, SEEK_SET);
	write(hm->file, &value, 4);
}

PosAndVal* hm_get(FSHashMap *hm, char* key) {
	int bucket_num = key2hash(key);
	PosAndVal *answer = get_value_from_key(hm, bucket_num, key);
	return answer;
}



static void test() {
	
	FSHashMap *hm = hm_init();
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






