typedef struct FSHashMap {
   int file;
   int* hash2write_bucket;
   int next_overflow_bucket;
   int file_size;
} FSHashMap;

typedef struct PosAndVal {
   int hm_pos;
   int ls_file_and_pos;
} PosAndVal;

FSHashMap* hm_init();
void hm_set(FSHashMap *hm, int pos, int value);
PosAndVal* hm_get(FSHashMap *hm, char* key);
void hm_add(FSHashMap *hm, char* key, int value);