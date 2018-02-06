typedef struct ListStore {
   int *files;
   int *file_vals_stored;
   int *file_size;
} ListStore;

int ls_start_list(ListStore *ls, int value);
int ls_add_element(ListStore *ls, int filenum_and_position, int value);
int* ls_get_list(ListStore *ls, int filenum_and_position);
ListStore* ls_init();

