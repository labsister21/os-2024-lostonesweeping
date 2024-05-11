#include "./find.h"

void find(char* arg){
    uint32_t curr_cluster_number = state.current_directory;

    Queue bfs;
    CreateQueue(&bfs);

    //Initialize Queue with root
    ElType root = ROOT_CLUSTER_NUMBER;
    enqueue(&bfs, root);

    while (!isEmpty(bfs)){
        process(&bfs, arg);
    }

    //End dirTable with curr_cluster_number before find()
    updateDirectoryTable(curr_cluster_number);
}

void CreateQueue(Queue *q){
	IDX_HEAD(*q) = IDX_UNDEF;
	IDX_TAIL(*q) = IDX_UNDEF;
}

bool isEmpty(Queue q){
	return ((IDX_HEAD(q) == IDX_UNDEF) && (IDX_TAIL(q) == IDX_UNDEF));
}
/* Mengirim true jika q kosong: lihat definisi di atas */
bool isFull(Queue q){
	if (IDX_HEAD(q) == 0) return (IDX_TAIL(q) == (CAPACITY-1));
	else return (IDX_TAIL(q) == (IDX_HEAD(q)-1));
}

int length(Queue q){
	if (isEmpty(q)) return 0;
	else if (IDX_TAIL(q) >= IDX_HEAD(q)) return (IDX_TAIL(q) - IDX_HEAD(q) + 1);
	else return ((CAPACITY - IDX_HEAD(q)) + IDX_TAIL(q) + 1);
}

void enqueue(Queue *q, ElType val){
	if (isEmpty(*q)){
		IDX_HEAD(*q) = 0;
		IDX_TAIL(*q) = 0;
	}
	else{
		IDX_TAIL(*q) = (IDX_TAIL(*q) + 1) % CAPACITY;
	}
	TAIL(*q) = val;
}

void dequeue(Queue *q, ElType *val){
	*val = HEAD(*q);
	if (IDX_HEAD(*q) == IDX_TAIL(*q)){
		IDX_HEAD(*q) = IDX_UNDEF;
		IDX_TAIL(*q) = IDX_UNDEF;
	}
	else{
		IDX_HEAD(*q) = (IDX_HEAD(*q) + 1) % CAPACITY;
	}
	

}

void process(Queue *q, char* findName){
    ElType cn;
    dequeue(q, &cn);
    updateDirectoryTable(cn);

    for (int i = 1; i < TOTAL_DIRECTORY_ENTRY; i++){
        if (state.curr_dir.table[i].user_attribute == UATTR_NOT_EMPTY){
            if (state.curr_dir.table[i].attribute == ATTR_SUBDIRECTORY){
                cn = convertToClusterNumber(state.curr_dir.table[i].cluster_high, state.curr_dir.table[i].cluster_low);
                enqueue(q, cn);
            }
            if (memcmp(state.curr_dir.table[i].name, findName, 8)){ //TODO: Compare ext, and print parents
                put_chars(findName);
                if (strlen(state.curr_dir.table[i].ext) != 0 && state.curr_dir.table[i].attribute != ATTR_SUBDIRECTORY){
                    put_char(".");
                    put_chars(state.curr_dir.table[i].ext);
                }
                put_char("\n");
            }
        }
    }
}

uint32_t convertToClusterNumber(uint16_t high, uint16_t low){
    return (uint32_t) ((high << 16) | low);
}
