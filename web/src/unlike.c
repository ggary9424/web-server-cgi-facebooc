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

    int id = -1;
	char *idStr = NULL;
    if((idStr = kvFindList(req->queryString, "id"))){
        sscanf(idStr, "%d", &id);	
    }

    Post *post = postGetById(db, id);
    if (!post) goto fail;

    likeDel(likeDelete(db, req->account->id, post->authorId, post->id));

    if (kvFindList(req->queryString, "r")) {
        char sbuff[1024];
        sprintf(sbuff, "/profile?id=%d", post->authorId);
        bsDel(idStr);
        *res = responseNewRedirect(sbuff);
		return;
    }

fail:
    *res = responseNewRedirect("/dashboard");
}
