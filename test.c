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

int parse(char* fn, Indexer *indexer, int start_doc_id) {
	int doc_id = start_doc_id;
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
			//printf("%i\n", doc_id);
			word_i = 0;
			clear_string(word,51);

			
			//int *l = get_list(indexer, word);
			//int i = 0;
			//while(l[i] != -1) {
			//	printf("   %i\n",l[i]);
			//	i += 1;
			//}
			//printf("\n");
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
	return doc_id;
}

void small_test() {
	Indexer *indexer = indexer_init(4);
	char fn[20] = "test.txt";
	parse(fn, indexer, 7);

	print_hm(indexer->hm);

	/*int *l = get_list(indexer, "thirtyfirst");
	int i = 0;
	while(l[i] != -1) {
		printf("33 %i\n",l[i]);
		i += 1;
	}
	printf("\n");

	l = get_list(indexer, "eleventh");
	i = 0;
	while(l[i] != -1) {
		printf("11 %i\n",l[i]);
		i += 1;
	}
	printf("\n");

	l = get_list(indexer, "sixth");
	i = 0;
	while(l[i] != -1) {
		printf("6 %i\n",l[i]);
		i += 1;
	}
	printf("\n");

	l = get_list(indexer, "fiftysecond");
	i = 0;
	while(l[i] != -1) {
		printf("52 %i\n",l[i]);
		i += 1;
	}
	printf("\n");

	l = get_list(indexer, "twentyninth");
	i = 0;
	while(l[i] != -1) {
		printf("29 %i\n",l[i]);
		i += 1;
	}
	printf("\n");*/

	//expand_hm(indexer->hm);
}

void big_test() {
	Indexer *indexer = indexer_init(100000);
	
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
		doc_id = parse(fn, indexer, doc_id);
		

		clock_t dt = clock() - st;
		double time_taken = ((double)dt)/CLOCKS_PER_SEC;
		printf("%i %f\n",page_id,time_taken);
		//print_stats();
	}

	int *l;
	int i;

	l = get_list(indexer, "earths");
	i = 0;
	while(l[i] != -1) {
		printf("%i\n",l[i]);
		i += 1;
	}
	printf("\n");

}

int main() {
	big_test();
}















