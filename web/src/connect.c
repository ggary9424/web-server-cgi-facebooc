#include <stdio.h>
#include <string.h>

#include "cgi.h"
#include "web.h"
#include "bs/bs.h"
#include "db/db.h"
#include "kv/kv.h"
#include "request/request.h"
#include "response/response.h"
#include "template/template.h"

void do_response(Request *req, Response ** res, sqlite3 *db)
{
    if (!req->account) return ;

    int id = -1;
	char *idStr = NULL;
    if((idStr = kvFindList(req->queryString, "id"))){
        sscanf(idStr, "%d", &id);	
    }

    Account *account = accountGetById(db, id);
    if (!account) goto fail;

    connectionDel(connectionCreate(db, req->account->id, account->id));

    char sbuff[1024];
    sprintf(sbuff, "/profile?id=%d", account->id);
    bsDel(idStr);
    *res = responseNewRedirect(sbuff);
	return;

fail:
    *res = responseNewRedirect("/dashboard");
}
