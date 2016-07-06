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

    Account *account = accountGetById(db, id);

    if (!account) return;

    if (account->id == req->account->id) {
        *res = responseNewRedirect("/dashboard");
		return;
	}

    *res = responseNew();
    Template *template = templateNew("../web/public/html/profile.html");
    Connection *connection = connectionGetByAccountIds(db,
                                                       req->account->id,
                                                       account->id);
    char connectStr[512];

    if (connection) {
        sprintf(connectStr, "You and %s are connected!", account->name);
    } else {
        sprintf(connectStr,
                "You and %s are not connected."
                " <a href=\"/connect?id=%d\">Click here</a> to connect!",
                account->name,
                account->id);
    }

    char *res_ch = NULL;
    char sbuff[128];
    char *bbuff = NULL;
    time_t t;
    bool liked;

    Post *post = NULL;
    ListCell *postPCell = NULL;
    ListCell *postCell  = postGetLatest(db, account->id, 0);

    if (postCell)
        res_ch = bsNew("<ul class=\"posts\">");

    while (postCell) {
        post = (Post *)postCell->value;
        liked = likeLiked(db, req->account->id, post->id);

        bbuff = bsNewLen("", strlen(post->body) + 256);
        sprintf(bbuff, "<li class=\"post-item\"><div class=\"post-author\">%s</div>", post->body);
        bsLCat(&res_ch, bbuff);

        if (liked) {
            bsLCat(&res_ch, "Liked - ");
        } else {
            sprintf(sbuff, "<a class=\"btn\" href=\"/like?id=%d\">Like</a> - ", post->id);
            bsLCat(&res_ch, sbuff);
        }

        t = post->createdAt;
        strftime(sbuff, 128, "%c GMT", gmtime(&t));
        bsLCat(&res_ch, sbuff);
        bsLCat(&res_ch, "</li>");

        bsDel(bbuff);
        postDel(post);
        postPCell = postCell;
        postCell  = postCell->next;

        free(postPCell);
    }

    if (res_ch) {
        bsLCat(&res_ch, "</ul>");
        templateSet(template, "profilePosts", res_ch);
        bsDel(res_ch);
    } else {
        templateSet(template, "profilePosts",
                    "<h4 class=\"not-found\">This person has not posted "
                    "anything yet!</h4>");
    }

    templateSet(template, "active", "profile");
    templateSet(template, "loggedIn", "t");
    templateSet(template, "subtitle", account->name);
    templateSet(template, "profileId", idStr);
    templateSet(template, "profileName", account->name);
    templateSet(template, "profileEmail", account->email);
    templateSet(template, "profileConnect", connectStr);
    templateSet(template, "accountName", req->account->name);
    responseSetStatus(*res, OK);
    responseSetBody(*res, templateRender(template));
    connectionDel(connection);
    accountDel(account);
    templateDel(template);
}
