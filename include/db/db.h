#ifndef DB_H
#define DB_H

#include <stdint.h>

#include "cgi.h"

#ifdef __cplusplus
extern "C"
{
#endif

void initDB(sqlite3 **db);

#ifdef __cplusplus
}
#endif

#endif