#include "cgi.h"
#include "web.h"
#include "bs/bs.h"
#include "request/request.h"
#include "response/response.h"
#include "template/template.h"

void do_response(Request *req, Response ** res, sqlite3 *db)
{
    if (!req->account) {
        *res = responseNewRedirect("/");
		return;
	}

    *res = responseNewRedirect("/");
    responseAddCookie(*res, "sid", "", NULL, NULL, -1);
}
