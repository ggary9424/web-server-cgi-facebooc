#ifndef MODELS_LIKE_H
#define MODELS_LIKE_H

#include <stdbool.h>
#include "sqlite3.h"

typedef struct Like {
    int id;
    int createdAt;
    int accountId;
    int authorId;
    int postId;
} Like;

Like *likeNew(int, int, int, int, int);
Like *likeCreate(sqlite3 *, int, int, int);
Like *likeDelete(sqlite3 *, int, int, int);
bool  likeLiked(sqlite3 *, int, int);
void  likeDel(Like *);

#endif
