#include "cgi.h"
#include "web.h"
#include "bs/bs.h"
#include "request/request.h"
#include "response/response.h"
#include "template/template.h"

void do_response(Request *req, Response ** res, sqlite3 *db)
{
    if (req->account)
        *res = responseNewRedirect("/dashboard");

    *res = responseNew();
    Template *template = templateNew("../web/public/html/index.html");
    responseSetStatus(*res, OK);
    templateSet(template, "active", "home");
    templateSet(template, "subtitle", "Home");
    responseSetBody(*res, templateRender(template));
    templateDel(template);
}
