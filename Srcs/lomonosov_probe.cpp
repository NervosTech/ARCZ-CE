#ifdef LOMONOSOV_TB
#include "lomonosov_probe.h"
#include "position.h"
#include "search.h"
#include "bitcount.h"
#include "movegen.h"
#include "lmtb.h"

bool lomonosov_server_mode = false;
bool lomonosov_loaded = false;

int lomonosov_change_server_mode(bool server_mode, bool console) {
	int result = -1;
	if (!lomonosov_loaded || server_mode != lomonosov_server_mode) {
		if (server_mode) {
			unload_lomonosov_tb();
			result = load_lmtb_server(console);
		} else {
			unload_lmtb_server();
			result = load_lomonosov_tb();
		}
		lomonosov_server_mode = server_mode;
		if (result != -1)
			lomonosov_loaded = true;
	}
	return result;
}

void lomonosov_set_threads_count(int threads_count) {
	if (lomonosov_loaded && lomonosov_server_mode)
		tb_set_threads_count(threads_count);
}

inline int position_sign(int value) {
	if (value < 0)
		return -1;
	if (value > 0)
		return 1;
	return 0;
}

bool lomonosov_tbprobe(Position& pos, int ss_ply, int *value, bool ce_value, int thread_idx, bool *from_dtm) {
	if (!lomonosov_loaded)
		return false;
	*value = VALUE_DRAW;
	int side; unsigned int psqW[KING_INDEX + 1]; unsigned int psqB[KING_INDEX + 1]; int piCount[10]; int sqEnP;
	pos.lomonosov_position(&side, psqW, psqB, piCount, &sqEnP);
	int eval = 0;
	char table_type = 0;
	int success = lomonosov_server_mode ?
		tb_probe_position_with_order_server(side, psqW, psqB, piCount, sqEnP, &eval, thread_idx, &table_type, 0) :
		tb_probe_position_with_order(side, psqW, psqB, piCount, sqEnP, &eval, &table_type, 0);
	if (from_dtm)
		*from_dtm = false;
	int dtm = MAX_PLY;
	if (DTM_TYPE(table_type)) {
		if (from_dtm)
			*from_dtm = true;
		if (!DTZ50_TYPE(table_type) && eval == -1) // mate
			dtm = 0;
		else
			dtm = abs(eval);
	}
	if (success == 0) {
		int sign = position_sign(eval);
		*value = (ce_value ? (sign * (VALUE_MATE - dtm - ss_ply))
			: (sign * (dtm + ss_ply)));
		return true;
	}
	return false;
}

bool lomonosov_root_probe(Position& pos, Search::RootMoves& rootMoves, bool *from_dtm) {
	if (!lomonosov_loaded)
		return false;
	int value;
	*from_dtm = false;

	if (!lomonosov_tbprobe(pos, 0, &value, false, 0)) return false;

	StateInfo st;
	CheckInfo ci(pos);
	bool success = false;
	bool from_dtm_moves = true;

	// Probe each move.
	for (size_t i = 0; i < rootMoves.size(); i++) {
		Move move = rootMoves[i].pv[0];
		pos.do_move(move, st, pos.gives_check(move, ci));
		int v = 0;
		bool from_dtm_cur = false;
		success = lomonosov_tbprobe(pos, 1, &v, false, 0, &from_dtm_cur);
		pos.undo_move(move);
		if (!success) return false;
		from_dtm_moves &= from_dtm_cur;
		rootMoves[i].score = (Value)v;
	}

	size_t j = 0;
	if (value > 0) {
		int best = 0;
		if (from_dtm_moves) {
			for (size_t i = 0; i < rootMoves.size(); i++) {
				int v = rootMoves[i].score;
				if (v < 0 && (v > best || best == 0))
					best = v;
			}
		}
		for (size_t i = 0; i < rootMoves.size(); i++) {
			int v = rootMoves[i].score;
			if (from_dtm_moves) {
				if (v == best) {
					rootMoves[j++] = rootMoves[i];
					rootMoves[0].score = VALUE_MATE - value;
					*from_dtm = true;
					break;
				}
			} else if (v < 0) {
				rootMoves[j++] = rootMoves[i];
			}
		}
	} else if (value < 0) {
		if (from_dtm_moves) {
			int best = 0;
			for (size_t i = 0; i < rootMoves.size(); i++) {
				int v = rootMoves[i].score;
				if (v > best)
					best = v;
			}
			for (size_t i = 0; i < rootMoves.size(); i++) {
				if (rootMoves[i].score == best)
					rootMoves[j++] = rootMoves[i];
			}
		} else
			j = rootMoves.size();
	} else {
		for (size_t i = 0; i < rootMoves.size(); i++) {
			if (rootMoves[i].score == 0)
				rootMoves[j++] = rootMoves[i];
		}
	}
	rootMoves.resize(j, Search::RootMove(MOVE_NONE));

	return true;
}

#endif // LOMONOSOV_TB