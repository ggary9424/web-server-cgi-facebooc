#include <stdio.h>
#include <string.h>

#include "cgi.h"
#include "sqlite3.h"
#include "web.h"

#include "bs/bs.h"
#include "request/request.h"
#include "response/response.h"
#include "template/template.h"

void do_response(Request *req, Response ** response, sqlite3 *db)
{    
    if (!req->account) {
        *response = responseNewRedirect("/login");
		return;
	}

    *response = responseNew();
    Template *template = templateNew("../web/public/html/dashboard.html");

    char *res = NULL;
    char sbuff[128];
    char *bbuff = NULL;
    time_t t;
    bool liked;

    Account *account = NULL;
    Post *post = NULL;
    ListCell *postPCell = NULL;
    ListCell *postCell  = postGetLatestGraph(db, req->account->id, 0);

    if (postCell)
        res = bsNew("<ul class=\"posts\">");

    while (postCell) {
        post = (Post *)postCell->value;
        account = accountGetById(db, post->authorId);
        liked = likeLiked(db, req->account->id, post->id);

        bbuff = bsNewLen("", strlen(post->body) + 256);
        sprintf(bbuff,
                "<li class=\"post-item\">"
                "<div class=\"post-author\">%s</div>"
                "<div class=\"post-content\">"
                "%s"
                "</div>",
                account->name,
                post->body);
        accountDel(account);
        bsLCat(&res, bbuff);

        if (liked) {
            sprintf(sbuff, "<a class=\"btn\" href=\"/unlike?id=%d\">Liked</a> - ", post->id);
	    bsLCat(&res, sbuff);
        } else {
            sprintf(sbuff, "<a class=\"btn\" href=\"/like?id=%d\">Like</a> - ", post->id);
            bsLCat(&res, sbuff);
        }

        t = post->createdAt;
        strftime(sbuff, 128, "%c GMT", gmtime(&t));
        bsLCat(&res, sbuff);
        bsLCat(&res, "</li>");

        bsDel(bbuff);
        postDel(post);
        postPCell = postCell;
        postCell  = postCell->next;

        free(postPCell);
    }

    if (res) {
        bsLCat(&res, "</ul>");
        templateSet(template, "graph", res);
        bsDel(res);
    } else {
        templateSet(template, "graph",
                    "<ul class=\"posts\"><div class=\"not-found\">Nothing here.</div></ul>");
    }

    templateSet(template, "active", "dashboard");
    templateSet(template, "loggedIn", "t");
    templateSet(template, "subtitle", "Dashboard");
    templateSet(template, "accountName", req->account->name);
    responseSetStatus(*response, OK);
    responseSetBody(*response, templateRender(template));
    templateDel(template);
}
