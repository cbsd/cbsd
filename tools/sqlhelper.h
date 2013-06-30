// SQL string Maxlen
#define SQLSTRLEN 8192
sqlite3* db;
int first_row;

int select_callback(void *p_data, int num_fields, char **p_fields, char **p_col_names);
int select_stmt(const char* stmt);
int select_valstmt(const char* stmt);

int select_varcallback(void *p_data, int num_fields, char **p_fields, char **p_col_names);
int select_varstmt(const char* stmt);

int sql_stmt(const char* stmt);
