#ifdef LOMONOSOV_TB
#include "position.h"
#include "search.h"

extern bool lomonosov_server_mode;
extern bool lomonosov_loaded;

int lomonosov_change_server_mode(bool server_mode, bool console);
void lomonosov_set_threads_count(int threads_count);

bool lomonosov_tbprobe(Position& pos, int ss_ply, int *value, bool ce_value, int thread_idx, bool *from_dtm = 0);
bool lomonosov_root_probe(Position& pos, Search::RootMoves& rootMoves, bool *from_dtm = 0);
#endif