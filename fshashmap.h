typedef struct FSHashMap {
   int file;
   int* hash2write_bucket;
   int next_overflow_bucket;
   int file_size;
   int num_buckets;
   int hm_num;
} FSHashMap;

typedef struct PosAndVal {
   int hm_pos;
   int ls_file_and_pos;
} PosAndVal;

FSHashMap* hm_init(int num_buckets);
void hm_set(FSHashMap *hm, int pos, int value);
PosAndVal* hm_get(FSHashMap *hm, char* key);
void hm_add(FSHashMap *hm, char* key, int value);
void print_stats();
void expand_hm(FSHashMap *hm);
void print_hm(FSHashMap *hm);