#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "indexer.h"
#include <time.h>

void clear_string(char* s, int len) {
	int i;
	for (i = 0; i < len; i++) {
		s[i] = '\0';
	}
}

int main() {
	Indexer *indexer = indexer_init();
	
	int doc_id = 0;
	int page_id;
	for (page_id = 0; page_id < 100; page_id++) {
		//printf("%i\n", page_id);
		clock_t st = clock();
		
		char str_page_id[5];
		sprintf(str_page_id, "%d", page_id);
		char fn[20] = "documents/";
		strcat(fn,str_page_id);
		strcat(fn,".txt");
		//printf("hi\n");
		//char fn[20] = "documents/test.txt";
		FILE* pFile = fopen(fn,"r");

		int word_i = 0;
		char *word = (char*)malloc(51 * sizeof(char));
		clear_string(word,51);
		char c;
		while (1) {
		//int i;
		//for (i = 0; i < 10; i++) {
			c = fgetc (pFile);
			if (c == EOF) {
				add_element(indexer, word, doc_id);
				break;
			}
			else if (c == '\n') {
				add_element(indexer, word, doc_id);
				word_i = 0;
				clear_string(word,51);
				doc_id += 1;
			}
			else if (c == ' ') {
				add_element(indexer, word, doc_id);
				word_i = 0;
				clear_string(word,51);
			}
			else if (word_i < 50) {
				word[word_i] = c;
				word_i += 1;
			}
			//if (doc_id == 10) {
			//	break;
			//}
		}
		fclose (pFile);

		clock_t dt = clock() - st;
		double time_taken = ((double)dt)/CLOCKS_PER_SEC;
		printf("%i %f\n",page_id,time_taken);
	}

	//int *l;
	//l = get_list(indexer, "italian");
	//printf("%i\n",l[0]);


	int *l;
	int i;

	l = get_list(indexer, "earths");
	i = 0;
	while(l[i] != -1) {
		printf("%i\n",l[i]);
		i += 1;
	}
	printf("\n");

	l = get_list(indexer, "limit");
	i = 0;
	while(l[i] != -1) {
		printf("%i\n",l[i]);
		i += 1;
	}
	printf("\n");

	/*l = get_list(indexer, "asdfkcnkljl");
	i = 0;
	while(l[i] != -1) {
		printf("%i\n",l[i]);
		i += 1;
	}
	printf("\n");

	l = get_list(indexer, "bigger");
	i = 0;
	while(l[i] != -1) {
		printf("%i\n",l[i]);
		i += 1;
	}
	printf("\n");*/
















}