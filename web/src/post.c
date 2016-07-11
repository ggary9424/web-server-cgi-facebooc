#include <stdio.h>
#include <string.h>

#include "cgi.h"
#include "web.h"

#include "bs/bs.h"
#include "kv/kv.h"
#include "request/request.h"
#include "response/response.h"
#include "template/template.h"

void do_response(Request *req, Response ** res, sqlite3 *db)
{
    if (!req->account) return;
    if (req->method != POST) return;

	char *postStr = kvFindList(req->postBody, "post");

    if (bsGetLen(postStr) == 0) {
        *res = responseNewRedirect("/dashboard");
		return;
	}
    else if (bsGetLen(postStr) < MAX_BODY_LEN) {
        postDel(postCreate(db, req->account->id, postStr));
	}
    *res = responseNewRedirect("/dashboard");
}
