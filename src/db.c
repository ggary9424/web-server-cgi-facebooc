#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cgi.h"
#include "db/db.h"
#include "sqlite3.h"

static void createDB(sqlite3 *db, const char *e)
{
    char *err;

    if (sqlite3_exec(db, e, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "error: initDB: %s\n", err);
        sqlite3_free(err);
        exit(1);
    }
}

void initDB(sqlite3 **db)
{
    if (sqlite3_open("db.sqlite3", db)) {
        fprintf(stderr, "error: unable to open DB: %s\n", sqlite3_errmsg(*db));
        exit(1);
    }

    createDB(*db, "CREATE TABLE IF NOT EXISTS accounts ("
             "  id        INTEGER PRIMARY KEY ASC"
             ", createdAt INTEGER"
             ", name      TEXT"
             ", username  TEXT"
             ", email     TEXT UNIQUE"
             ", password  TEXT"
             ")");

    createDB(*db, "CREATE TABLE IF NOT EXISTS sessions ("
             "  id        INTEGER PRIMARY KEY ASC"
             ", createdAt INTEGER"
             ", account   INTEGER"
             ", session   TEXT"
             ")");

    createDB(*db, "CREATE TABLE IF NOT EXISTS connections ("
             "  id        INTEGER PRIMARY KEY ASC"
             ", createdAt INTEGER"
             ", account1  INTEGER"
             ", account2  INTEGER"
             ")");

    createDB(*db, "CREATE TABLE IF NOT EXISTS posts ("
             "  id        INTEGER PRIMARY KEY ASC"
             ", createdAt INTEGER"
             ", author    INTEGER"
             ", body      TEXT"
             ")");

    createDB(*db, "CREATE TABLE IF NOT EXISTS likes ("
             "  id        INTEGER PRIMARY KEY ASC"
             ", createdAt INTEGER"
             ", account   INTEGER"
             ", author    INTEGER"
             ", post      INTEGER"
             ")");
}
