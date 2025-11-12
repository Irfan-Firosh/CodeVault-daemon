#ifndef DB_H
#define DB_H
#include "codevault.h"
#include <sqlite3.h>

typedef struct {
    sqlite3 *db;
    char *path;
} cv_db_t;


int cv_db_open(cv_db_t *out, const char *path);
int cv_db_init_schema(cv_db_t *db);
void cv_db_close(cv_db_t *db);

int cv_db_insert_commit(cv_db_t *db,
                        const char *repo,
                        const char *commit_hash,
                        const char *author,
                        const char *email,
                        long date_unix,
                        const char *message);

#endif
