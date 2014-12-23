/********************************************************************
db.h - This file contains all the structures, defines, and function
	prototype for the db.exe program.
*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define MAX_IDENT_LEN		16
#define MAX_NUM_COL			16
#define MAX_NUM_ROWS		1000
#define MAX_TOK_LEN			32
#define KEYWORD_OFFSET		10
#define TIME_STAMP_LENGTH	14
#define STRING_BREAK		" (),<>="
#define NUMBER_BREAK		" ,)"
#define OPERATORS			"=<>"
#define LOG_FILE			"db.log"
#define DB_FILE				"dbfile.bin"
#define MAX_ROW_SIZE		528 // MAX_NUM_COL * (MAX_TOK_LEN + 1)
#define BIG_LINE			2048


/* Column descriptor sturcture = 20+4+4+4+4 = 36 bytes */
typedef struct cd_entry_def
{
	char	col_name[MAX_IDENT_LEN+4];
	int		col_id;                   /* Start from 0 */
	int		col_type;
	int		col_len;
	int 	not_null;
} cd_entry;

/* Table packed descriptor sturcture = 4+20+4+4+4 = 36 bytes
   Minimum of 1 column in a table - therefore minimum size of
	 1 valid tpd_entry is 36+36 = 72 bytes. */
typedef struct tpd_entry_def
{                                             
	int		tpd_size;
	char	table_name[MAX_IDENT_LEN+4];
	int		num_columns;
	int		cd_offset;
	int     tpd_flags;
} tpd_entry;

/* Table packed descriptor list = 4+4+4+36 = 48 bytes.  When no
   table is defined the tpd_list is 48 bytes.  When there is 
	 at least 1 table, then the tpd_entry (36 bytes) will be
	 overlapped by the first valid tpd_entry. */
typedef struct tpd_list_def
{
	int		list_size;
	int		num_tables;
	int		db_flags;
	char	image_name[MAX_IDENT_LEN];
	tpd_entry	tpd_start;
}tpd_list;

/* This struct is used to hold rows of a table in memory. */
typedef struct row_def
{
} row;

/* This struct is use to hold table and its rows in memory. */
typedef struct table_def
{
	int tab_size;
	int num_of_rows;
	int row_size;
} table;

/* This token_list definition is used for breaking the command
   string into separate tokens in function get_tokens().  For
	 each token, a new token_list will be allocated and linked 
	 together. */
typedef struct t_list
{
	char	tok_string[MAX_TOK_LEN];
	int		tok_class;
	int		tok_value;
	struct t_list *next;
} token_list;

/* Struct to hold select conditions. */
typedef struct conditions_def
{
	char *col_names[MAX_NUM_COL];
	char *values[MAX_NUM_COL];
	int and_or[MAX_NUM_COL];
	int operators[MAX_NUM_COL];
} conditions;

/* Struct to hold function and column. */
typedef struct function_def
{
	int func_id;
	char col_name[MAX_IDENT_LEN];
} function;

/* This enum defines the different classes of tokens for 
	 semantic processing. */
typedef enum t_class
{
	keyword = 1,	// 1
	identifier,		// 2
	symbol, 			// 3
	type_name,		// 4
	constant,		  // 5
	function_name,// 6
	terminator,		// 7
	error			    // 8
  
} token_class;

/* This enum defines the different values associated with
   a single valid token.  Use for semantic processing. */
typedef enum t_value
{
	T_INT = 10,		// 10 - new type should be added above this line
	T_CHAR,		    // 11 
	K_CREATE, 		// 12
	K_TABLE,		// 13
	K_NOT,				// 14
	K_NULL,				// 15
	K_DROP,				// 16
	K_LIST,				// 17
	K_SCHEMA,			// 18
	K_FOR,        // 19
	K_TO,				  // 20
	K_INSERT,     // 21
	K_INTO,       // 22
	K_VALUES,     // 23
	K_DELETE,     // 24
	K_FROM,       // 25
	K_WHERE,      // 26
	K_UPDATE,     // 27
	K_SET,        // 28
	K_SELECT,     // 29
	K_ORDER,      // 30
	K_BY,         // 31
	K_DESC,       // 32
	K_IS,         // 33
	K_AND,        // 34
	K_OR,         // 35 - new keyword should be added below this line
	K_ASCE,
	K_BACKUP,
	K_RESTORE,
	K_WITHOUT,
	K_RF,
	K_ROLLFORWARD,
	F_SUM,        // 36
	F_AVG,        // 37
	F_COUNT,      // 38 - new function name should be added below this line
	S_LEFT_PAREN = 70,  // 70
	S_RIGHT_PAREN,		  // 71
	S_COMMA,			      // 72
	S_STAR,             // 73
	S_EQUAL,            // 74
	S_LESS,             // 75
	S_GREATER,          // 76
	S_SINGLE_QUOTE,		//77
	IDENT = 85,			    // 85
	INT_LITERAL = 90,	  // 90
	STRING_LITERAL,     // 91
	EOC = 95,			      // 95
	INVALID = 99		    // 99
} token_value;

/* This constants must be updated when add new keywords */
#define TOTAL_KEYWORDS_PLUS_TYPE_NAMES 35

/* New keyword must be added in the same position/order as the enum
   definition above, otherwise the lookup will be wrong */
char *keyword_table[] = 
{
  "int", "char", "create", "table", "not", "null", "drop", "list", "schema",
  "for", "to", "insert", "into", "values", "delete", "from", "where", 
  "update", "set", "select", "order", "by", "desc", "is", "and", "or",
  "asce", "backup", "restore", "without", "rf", "rollforward", "sum", "avg", "count"
};

/* This enum defines a set of possible statements */
typedef enum s_statement
{
	INVALID_STATEMENT = -199,	// -199
	CREATE_TABLE = 100,				// 100
	DROP_TABLE,								// 101
	LIST_TABLE,								// 102
	LIST_SCHEMA,							// 103
	INSERT,                   // 104
	DELETE,                   // 105
	UPDATE,                   // 106
	SELECT,                    // 107
	BACKUP,
	RESTORE,
	ROLLFORWARD
} semantic_statement;

/* This enum has a list of all the errors that should be detected
   by the program.  Can append to this if necessary. */
typedef enum error_return_codes
{
	INVALID_TABLE_NAME = -399,	// -399
	DUPLICATE_TABLE_NAME,				// -398
	TABLE_NOT_EXIST,						// -397
	INVALID_TABLE_DEFINITION,		// -396
	INVALID_COLUMN_NAME,				// -395
	DUPLICATE_COLUMN_NAME,			// -394
	UNKNOWN_COLUMN,						// -393
	MAX_COLUMN_EXCEEDED,				// -392
	INVALID_TYPE_NAME,					// -391
	INVALID_COLUMN_DEFINITION,	// -390
	INVALID_COLUMN_LENGTH,			// -389
	INVALID_REPORT_FILE_NAME,		// -388
	FILE_DELETE_ERROR,				//-387			
  /* Must add all the possible errors from I/U/D + SELECT here */
	FILE_OPEN_ERROR = -299,			// -299
	DBFILE_CORRUPTION,					// -298
	MEMORY_ERROR,				    // -297
	TYPE_MISMATCH,					// -296
	INVALID_NUMBER_PARAMS,			// -295
	INVALID_OPERATOR,				// -294
	NO_ROWS_DELETED,				// -293
	NO_ROWS_UPDATED,				// -292
	EMPTY_TABLE,					// -291
	NULL_ASSIGNMENT,
	INVALID_VALUE_LENGTH,
	INVALID_PARAM_TO_FUNC,
	IMAGE_FILE_ALREADY_EXISTS,
	ERROR_RENAMING_LOG_FILE,
	ROLLFORWARD_PENDING,
	MAX_NUM_ROWS_REACHED,
	INVALID_TIME_STAMP
} return_codes;

/* Set of function prototypes */
int process_input(char *str);
int get_token(char *command, token_list **tok_list);
void add_to_list(token_list **tok_list, char *tmp, int t_class, int t_value);
int do_semantic(token_list *tok_list, char *statement);
int sem_create_table(token_list *t_list);
int sem_drop_table(token_list *t_list);
int sem_list_tables();
int sem_list_schema(token_list *t_list);
int sem_insert(token_list *t_list);
int sem_delete(token_list *t_list);
int sem_update(token_list *t_list);
int sem_select(token_list *t_list);
int create_tab_file(char *tabname, int row_size);
int delete_tab_file(char *tabname);
table *read_tab_file(char *tabname);
int update_tab_file(table *tab, char *tabname);
cd_entry *get_cd_entry(tpd_entry *tab_entry, char *col_name);
int delete_records(tpd_entry *tab_entry, conditions *cond);
int update_records(tpd_entry *tab_entry, conditions *new_val, conditions *cond);
int process_update(row **cur_row, conditions *new_val, cd_entry *col, int *count, int offset);
void process_delete(row **cur_row, row **last_row, table **tab, int *count);
void update_last_row_pt(table **tab, row **last_row, conditions *cond, tpd_entry *tab_entry,int *count);
void print_tab(table *tab, tpd_entry *tab_entry, char **select_col, conditions *con, function *func);
//void print_table(table *tab, tpd_entry *tab_entry);
int calc_offset(tpd_entry *tab_entry, char *col_name);
//int process_update(cd_entry *col_to_update, char *new_val, int offset_update,	row **cur_row, int *count);
char *get_line_separator(tpd_entry *tab_entry, char **select_col, function *func);
int valid_col_names(tpd_entry *tab_entry, char **col_names);
int get_conditions(tpd_entry *tab_entry, token_list **t_list, conditions *cond);
int order_by(table **tab, tpd_entry *tab_entry, token_list **t_list);
void sort(table **tab, tpd_entry *tab_entry, cd_entry *col, int desc);
void swap(row **r1, row **r2, int size);
bool satisfies_cond(row *r, conditions *cond, tpd_entry *tab_entry);
int get_function(token_list **t_list, function *func);
int get_count(table *tab, tpd_entry *tab_entry, char *col_name, conditions *cond);
int get_sum(table *tab, tpd_entry *tab_entry, char *col_name, conditions *cond);
double get_avg(table *tab, tpd_entry *tab_entry, char *col_name, conditions *cond);
bool unique_col(char **select_col, char *col_name);
char *get_time_stamp();
int create_log_file();
int add_log_entry(char *query);
int sem_backup(token_list *t_list);
int backup(char *image_name);
char *rename_log();
int get_backup_offset(long *offset, char *log);
int redo_logged_transactions(char *time_stamp);
char *stristr(char *str1, char *str2);
int sem_restore(token_list *t_list);
int restore(char *image_name, int rf);
int copy_log(char *log);
int toggle_db_flag();
int sem_rollforward(token_list *t_list);
/*
	Keep a global list of tpd - in real life, this will be stored
	in shared memory.  Build a set of functions/methods around this.
*/
tpd_list	*g_tpd_list;
int initialize_tpd_list();
int add_tpd_to_list(tpd_entry *tpd);
int drop_tpd_from_list(char *tabname);
tpd_entry* get_tpd_from_list(char *tabname);