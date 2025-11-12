#include "db.h"

static int exec_sql(sqlite3 *db, const char *sql) {
    char *err = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "sqlite exec error %s\n", err ? err : "unknown");
        sqlite3_free(err);
        return rc;
    }
    return rc;
}

int cv_db_open(cv_db_t *out, const char *path) {
    if (!out || !path) return SQLITE_ERROR;
    memset(out, 0, sizeof(*out));
    int rc = sqlite3_open_v2(path, &out->db, 
                            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
                                NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "sqlite open(%s) error %s\n", path, out->db ? sqlite3_errmsg(out->db) : "unknown");
        if (out->db) sqlite3_close_v2(out->db);
        return rc;
    }
    out->path = strdup(path);

    rc = exec_sql(out->db,  "PRAGMA journal_mode=WAL;");
    if (rc != SQLITE_OK) {cv_db_close(out); return rc;}

    rc = exec_sql(out->db, "PRAGMA synchronous=NORMAL;");
    if (rc != SQLITE_OK) {cv_db_close(out); return rc;}

    return SQLITE_OK;
}

int cv_db_init_schema(cv_db_t *db) {
    if (!db || !db->db) return SQLITE_ERROR;
    const char *sql = 
        "BEGIN EXCLUSIVE;\n"
        "CREATE TABLE IF NOT EXISTS commits (\n"
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
        "  repo TEXT NOT NULL,\n"
        "  commit_hash TEXT NOT NULL,\n"
        "  author TEXT,\n"
        "  email TEXT,\n"
        "  date INTEGER,\n"
        "  message TEXT,\n"
        "  indexed_at INTEGER DEFAULT (strftime('%s','now')),\n"
        "  UNIQUE(repo, commit_hash)\n"
        ");\n"
        "CREATE INDEX IF NOT EXISTS idx_commits_repo_date ON commits(repo, date);\n"
        "COMMIT;\n";
    return exec_sql(db->db, sql);
}

void cv_db_close(cv_db_t *db) {
    if (!db) return;
    if (db->db) {
        sqlite3_close_v2(db->db);
    }
    free(db->path);
    db->db = NULL;
    db->path = NULL;
}

int cv_db_insert_commit(cv_db_t *db, 
                        const char *repo, 
                        const char *commit_hash,
                        const char *author,
                        const char *email, 
                        long date_unix,
                        const char *message) {
    if (!db || !db->db) return SQLITE_ERROR;
    const char *sql = "INSERT OR IGNORE INTO commits (repo, commit_hash, author, email, date, message) VALUES (?,?,?,?,?,?);";
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL prepare error: %s\n", sqlite3_errmsg(db->db));
        return rc;
    }
    sqlite3_bind_text(stmt, 1, repo, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, commit_hash, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, author ? author : "", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, email ? email : "", -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, (sqlite3_int64)date_unix);
    sqlite3_bind_text(stmt, 6, message ? message : "", -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_CONSTRAINT) {
        fprintf(stderr, "SQL step error: %s\n", sqlite3_errmsg(db->db));
        sqlite3_finalize(stmt);
        return rc;
    }

    sqlite3_finalize(stmt);
    return SQLITE_OK;
}
