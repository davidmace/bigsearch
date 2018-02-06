#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "indexer.h"

int add_element(Indexer *indexer, char* key, int element) {
	if (strlen(key) > 50) {
		return 1;
	}
	PosAndVal *pos_and_val = hm_get(indexer->hm, key);
	int nfile_and_npos;
	if (pos_and_val == NULL) {
		nfile_and_npos = ls_start_list(indexer->ls, element);
		hm_add(indexer->hm, key, nfile_and_npos);
	}
	else {
		nfile_and_npos = ls_add_element(indexer->ls, pos_and_val->ls_file_and_pos, element);
		hm_set(indexer->hm, pos_and_val->hm_pos, nfile_and_npos);
	}
	free(pos_and_val);
	return 0;
}

int* get_list(Indexer *indexer, char* key) {
	PosAndVal *pos_and_val = hm_get(indexer->hm, key);
	if (pos_and_val == NULL) {
		free(pos_and_val);
		return NULL;
	}
	int* l = ls_get_list(indexer->ls, pos_and_val->ls_file_and_pos);
	free(pos_and_val);
	return l;
}

Indexer* indexer_init() {
	Indexer *indexer = (Indexer*)malloc(sizeof(Indexer));
	FSHashMap *hm = hm_init();
	indexer->hm = hm;
	ListStore *ls = ls_init();
	indexer->ls = ls;
	return indexer;
}


/*static void test() {
	FSHashMap *hm = hm_init();
	ListStore *ls = ls_init();
	add_element(hm,ls,"dog",254);
	add_element(hm,ls,"cat",253);
	add_element(hm,ls,"eel",252);
	add_element(hm,ls,"dog",251);
	add_element(hm,ls,"cat",250);
	add_element(hm,ls,"cat",249);
	add_element(hm,ls,"ram",248);
	add_element(hm,ls,"ram",247);
	add_element(hm,ls,"cataaa",246);
	add_element(hm,ls,"cataad",245);
	add_element(hm,ls,"cataad",244);
	add_element(hm,ls,"cataag",243);
	int* l;
	l = get_list(hm,ls,"dog");
	printf("%i\n",l[0]);
	printf("%i\n",l[1]);
	l = get_list(hm,ls,"cat");
	printf("%i\n",l[0]);
	printf("%i\n",l[1]);
	printf("%i\n",l[2]);
	l = get_list(hm,ls,"eel");
	printf("%i\n",l[0]);
	l = get_list(hm,ls,"ram");
	printf("%i\n",l[0]);
	printf("%i\n",l[1]);
	l = get_list(hm,ls,"cataaa");
	printf("%i\n",l[0]);
	l = get_list(hm,ls,"cataad");
	printf("%i\n",l[0]);
	printf("%i\n",l[1]);
	l = get_list(hm,ls,"cataag");
	printf("%i\n",l[0]);
}*/











