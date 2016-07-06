#include "cgi.h"
#include "web.h"
#include "bs/bs.h"
#include "request/request.h"
#include "response/response.h"
#include "template/template.h"

void do_response(Request *req, Response ** res, sqlite3 *db)
{
    *res = responseNew();
    Template *template = templateNew("../web/public/html/about.html");
    templateSet(template, "active", "about");
    templateSet(template, "loggedIn", req->account ? "t" : "");
    templateSet(template, "subtitle", "About");
    responseSetStatus(*res, OK);
    responseSetBody(*res, templateRender(template));
    templateDel(template);
}
