#include "fshashmap.h"
#include "liststore.h"

typedef struct Indexer {
  FSHashMap *hm; 
	ListStore *ls;
} Indexer;

int add_element(Indexer *indexer, char* key, int element);
int* get_list(Indexer *indexer, char* key);
Indexer* indexer_init(int hm_num_buckets);