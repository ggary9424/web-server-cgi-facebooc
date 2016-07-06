#ifndef WEB_H
#define WEB_H

#include "cgi.h"
#include "sqlite3.h"
#include "models/account.h"
#include "models/connection.h"
#include "models/like.h"
#include "models/post.h"
#include "models/session.h"
#include "request/request.h"
#include "response/response.h"

#ifdef __cplusplus
extern "C"
{
#endif

void do_response(Request *req, Response ** res, sqlite3 *db);

#ifdef __cplusplus
}
#endif

#endif
