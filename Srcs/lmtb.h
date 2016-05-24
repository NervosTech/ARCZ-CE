#ifndef __LMTB_H_
#define __LMTB_H_
#include <windows.h>

//#define TB_DLL_EXPORT

/*** table types: ***/
// endgame table:
#define ML		0
#define WL		1
#define TL		2
#define PL		3
#define DL		4
// dtz50 table:
#define ZML		5
#define ZWL		6
#define ZTL		7
#define ZPL		8
#define ZDL		9

#define DTM_TYPE(table_type) (table_type == ML || table_type == ZML || table_type == PL || table_type == ZPL)
#define DTZ50_TYPE(table_type) (table_type >= ZML)

#define FUNC_ADD_TABLE_PATH					0
#define FUNC_SET_TABLE_PATH					1
#define FUNC_SET_CACHE_SIZE					2
#define FUNC_CLEAR_CACHE					3
#define FUNC_CLEAR_CACHE_ALL				4
#define FUNC_SET_TABLE_ORDER				5
#define FUNC_GET_TABLE_ORDER				6
#define FUNC_GET_MAX_PIECES_COUNT			7
#define FUNC_GET_MAX_PIECES_COUNT_ORDER		8
#define FUNC_GET_TABLE_NAME					9
#define FUNC_GET_MISSING_TABLE_NAME			10
#define FUNC_PROBE_FEN						11
#define FUNC_PROBE_FEN_WITH_ORDER			12
#define FUNC_PROBE_FEN_DTMZ50				13
#define FUNC_PROBE_POSITION					14
#define FUNC_PROBE_POSITION_WITH_ORDER		15
#define FUNC_PROBE_POSITION_DTMZ50			16
#define FUNC_SET_THREADS_COUNT				17
#define FUNC_CONNECT_TO_SERVER				18
#ifndef TB_DLL_EXPORT
#define FUNC_START_COUNT					19
#define FUNC_GET_NUMBER_LOAD_FROM_CACHE		(FUNC_START_COUNT + 0)
#define FUNC_GET_NUMBER_LOAD_FROM_FILE		(FUNC_START_COUNT + 1)
#define FUNC_GET_NUMBER_POP_FROM_CACHE		(FUNC_START_COUNT + 2)
#define FUNC_GET_NUMBER_IN_CACHE			(FUNC_START_COUNT + 3)
#define FUNC_GET_CACHE_SIZE					(FUNC_START_COUNT + 4)
#define FUNC_GET_HIDDEN_SIZE				(FUNC_START_COUNT + 5)
#define FUNC_SET_LOGGING					(FUNC_START_COUNT + 6)
#define FUNC_SET_HIDDEN_CACHE_CLEAN_PERCENT (FUNC_START_COUNT + 7)
#define FUNC_PRINT_STATISTICS				(FUNC_START_COUNT + 8)
#define FUNC_PROBE_FEN_SPECIAL_MATE_STATE	(FUNC_START_COUNT + 9)
#define FUNC_GET_TREE_FEN					(FUNC_START_COUNT + 10)
#define FUNC_GET_TREE_BOUNDED_FEN			(FUNC_START_COUNT + 11)
#define FUNC_GET_BEST_MOVE_FEN				(FUNC_START_COUNT + 12)
#define FUNC_GET_LINE_FEN					(FUNC_START_COUNT + 13)
#define FUNC_GET_LINE_BOUNDED_FEN			(FUNC_START_COUNT + 14)
#endif

typedef void (*add_table_path)(const char *);
//typedef void (*set_table_path)(const char *); = add_table_path
typedef void (*set_cache_size)(int);
typedef void (*clear_cache)(char);
typedef void (*clear_cache_all)(void);
typedef bool (*set_table_order)(const char *);
typedef int (*get_table_order)(char *);
typedef int (*get_max_pieces_count)(char);
typedef int (*get_max_pieces_count_with_order)(void);
typedef void (*get_table_name)(const char *, char *);
typedef void (*get_missing_table_name)(char *);
typedef int (*probe_fen)(const char *, int *, char);
typedef int (*probe_fen_with_order)(const char *, int *, char *);
//typedef int (*probe_fen_dtm50)(const char *, int *, char); = probe_fen_with_order
typedef int (*probe_position)(int, unsigned int *, unsigned int *, int *, int, int *, char, unsigned char);
typedef int (*probe_position_with_order)(int, unsigned int *, unsigned int *, int *, int, int *, char *, unsigned char);
//typedef int (*probe_position_dtm50)(int, unsigned int *, unsigned int *, int *, int, int *, char, unsigned char); = probe_position_with_order
#ifndef TB_DLL_EXPORT
typedef unsigned long long (*get_cache_size)(void);
//+5 functions with the same parameters = get_cache_size
//typedef void (*set_logging)(char); = clear_cache
//typedef void (*set_hidden_cache_clean_percent)(int); = set_cache_size
//typedef void (*print_statistics)(const char *); = add_table_path
//typedef int (*probe_fen_specital_mate_state)(const char *, int *, char); = probe_fen
typedef int (*get_tree_fen)(const char *, char *, char);
typedef int (*get_tree_bounded_fen)(const char *, char *, char, int, int, int);
typedef int (*get_line_bounded_fen)(const char *, char *, char, int);
#endif

typedef int (*probe_fen_server)(const char *, int *, int, char);
typedef int (*probe_fen_with_order_server)(const char *, int *, int, char *);
typedef int (*probe_position_server)(int, unsigned int *, unsigned int *, int *, int, int *, int, char, unsigned char);
typedef int (*probe_position_with_order_server)(int, unsigned int *, unsigned int *, int *, int, int *, int, char *, unsigned char);

#ifdef __cplusplus
extern "C" {
#endif

	int load_lomonosov_tb(void);
	void unload_lomonosov_tb();
	int load_lmtb_server(bool);
	void unload_lmtb_server();

	extern add_table_path tb_add_table_path;
	extern add_table_path tb_set_table_path;
	extern set_cache_size tb_set_cache_size;
	extern clear_cache tb_clear_cache;
	extern clear_cache_all tb_clear_cache_all;
	extern set_table_order tb_set_table_order;
	extern get_table_order tb_get_table_order;
	extern get_max_pieces_count tb_get_max_pieces_count;
	extern get_max_pieces_count_with_order tb_get_max_pieces_count_with_order;
	extern get_table_name tb_get_table_name;
	extern get_missing_table_name tb_get_missing_table_name;
	extern probe_fen tb_probe_fen;
	extern probe_fen_with_order tb_probe_fen_with_order;
	extern probe_fen_with_order tb_probe_fen_dtmz50;
	extern probe_position tb_probe_position;
	extern probe_position_with_order tb_probe_position_with_order;
	extern probe_position_with_order tb_probe_position_dtmz50;
#ifndef TB_DLL_EXPORT
	extern get_cache_size tb_get_number_load_from_cache;
	extern get_cache_size tb_get_number_load_from_file;
	extern get_cache_size tb_get_number_pop_from_cache;
	extern get_cache_size tb_get_number_in_cache;
	extern get_cache_size tb_get_cache_size;
	extern get_cache_size tb_get_hidden_size;
	extern clear_cache tb_set_logging;
	extern set_cache_size tb_set_hidden_cache_clean_percent;
	extern add_table_path tb_print_statistics;
	extern probe_fen tb_probe_fen_special_mate_state;
	extern get_tree_fen tb_get_tree_fen;
	extern get_tree_bounded_fen tb_get_tree_bounded_fen;
	extern get_tree_fen tb_get_best_move_fen;
	extern get_tree_fen tb_get_line_fen;
	extern get_line_bounded_fen tb_get_line_bounded_fen;
#endif

	extern set_cache_size tb_set_threads_count;
	extern probe_fen_server tb_probe_fen_server;
	extern probe_fen_with_order_server tb_probe_fen_with_order_server;
	extern probe_position_server tb_probe_position_server;
	extern probe_position_with_order_server tb_probe_position_with_order_server;

#ifdef __cplusplus
}
#endif

#endif /* __LMTB_H_ */