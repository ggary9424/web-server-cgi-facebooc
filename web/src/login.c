#include <stdio.h>
#include <string.h>

#include "cgi.h"
#include "sqlite3.h"
#include "web.h"

#include "bs/bs.h"
#include "kv/kv.h"
#include "request/request.h"
#include "response/response.h"
#include "template/template.h"

#define invalid(k, v) {          \
    templateSet(template, k, v); \
    valid = false;               \
}

void do_response(Request *req, Response ** response, sqlite3 *db)
{
    if (req->account) {
        *response = responseNewRedirect("/dashboard");
		return;
	}

    *response = responseNew();
    Template *template = templateNew("../web/public/html/login.html");
    responseSetStatus(*response, OK);
    templateSet(template, "active", "login");
    templateSet(template, "subtitle", "Login");

    if (req->method == POST) {
        bool valid = true;

        char *username = kvFindList(req->postBody, "username");
        char *password = kvFindList(req->postBody, "password");

        if (!username) {
            invalid("usernameError", "Username missing!");
        } else {
            templateSet(template, "formUsername", username);
        }

        if (!password) {
            invalid("passwordError", "Password missing!");
        }

        if (valid) {
            Session *session = sessionCreate(db, username, password);
            if (session) {
                responseSetStatus(*response, FOUND);
                responseAddCookie(*response, "sid", session->sessionId,
                                  NULL, NULL, 3600 * 24 * 30);
                responseAddHeader(*response, "Location", "/dashboard");
                templateDel(template);
                sessionDel(session);
				return;
            } else {
                invalid("usernameError", "Invalid username or password.");
            }
        }
    }
    responseSetBody(*response, templateRender(template));
    templateDel(template);
	return;
}
