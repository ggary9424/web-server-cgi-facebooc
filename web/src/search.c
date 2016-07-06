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
    if (!req->account) {
        *res = responseNewRedirect("/login");
		return;	
	}
    char *query = kvFindList(req->queryString, "q");

    if (!query) return;

    char *res_ch = NULL;
    char  sbuff[1024];

    Account *account = NULL;
    ListCell *accountPCell = NULL;
    ListCell *accountCell  = accountSearch(db, query, 0);

    if (accountCell)
        res_ch = bsNew("<ul class=\"search-results\">");

    while (accountCell) {
        account = (Account *)accountCell->value;

        sprintf(sbuff,
                "<li><a href=\"/profile?id=%d\">%s</a> (<span>%s</span>)</li>\n",
                account->id, account->name, account->email);
        bsLCat(&res_ch, sbuff);

        accountDel(account);
        accountPCell = accountCell;
        accountCell  = accountCell->next;

        free(accountPCell);
    }

    if (res_ch)
        bsLCat(&res_ch, "</ul>");

    *res = responseNew();
    Template *template = templateNew("../web/public/html/search.html");
    responseSetStatus(*res, OK);

    if (!res_ch) {
        templateSet(template, "results",
                    "<h4 class=\"not-found\">There were no results "
                    "for your query.</h4>");
    } else {
        templateSet(template, "results", res_ch);
        bsDel(res_ch);
    }

    templateSet(template, "searchQuery", query);
    templateSet(template, "active", "search");
    templateSet(template, "loggedIn", "t");
    templateSet(template, "subtitle", "Search");
    responseSetBody(*res, templateRender(template));
    templateDel(template);
}
