#include "cgi.h"
#include "web.h"
#include "sqlite3.h"
#include "bs/bs.h"
#include "kv/kv.h"
#include "request/request.h"
#include "response/response.h"
#include "template/template.h"

#define invalid(k, v) {          \
    templateSet(template, k, v); \
    valid = false;               \
}

void do_response(Request *req, Response ** res, sqlite3 *db)
{
    if (req->account) {
        *res = responseNewRedirect("/dashboard");
		return;
	}
    *res = responseNew();
    Template *template = templateNew("../web/public/html/signup.html");
    templateSet(template, "active", "signup");
    templateSet(template, "subtitle", "Sign Up");
    responseSetStatus(*res, OK);

    if (req->method == POST) {
        bool valid = true;
        char *name = kvFindList(req->postBody, "name");
        char *email = kvFindList(req->postBody, "email");
        char *username = kvFindList(req->postBody, "username");
        char *password = kvFindList(req->postBody, "password");
        char *confirmPassword = kvFindList(req->postBody, "confirm-password");

        if (!name) {
            invalid("nameError", "You must enter your name!");
        } else if (strlen(name) < 5 || strlen(name) > 50) {
            invalid("nameError",
                    "Your name must be between 5 and 50 characters long.");
        } else {
            templateSet(template, "formName", name);
        }

        if (!email) {
            invalid("emailError", "You must enter an email!");
        } else if (strchr(email, '@') == NULL) {
            invalid("emailError", "Invalid email.");
        } else if (strlen(email) < 3 || strlen(email) > 50) {
            invalid("emailError",
                    "Your email must be between 3 and 50 characters long.");
        } else if (!accountCheckEmail(db, email)) {
            invalid("emailError", "This email is taken.");
        } else {
            templateSet(template, "formEmail", email);
        }

        if (!username) {
            invalid("usernameError", "You must enter a username!");
        } else if (strlen(username) < 3 || strlen(username) > 50) {
            invalid("usernameError",
                    "Your username must be between 3 and 50 characters long.");
        } else if (!accountCheckUsername(db, username)) {
            invalid("usernameError", "This username is taken.");
        } else {
            templateSet(template, "formUsername", username);
        }

        if (!password) {
            invalid("passwordError", "You must enter a password!");
        } else if (strlen(password) < 8) {
            invalid("passwordError",
                    "Your password must be at least 8 characters long!");
        }

        if (!confirmPassword) {
            invalid("confirmPasswordError", "You must confirm your password.");
        } else if (strcmp(password, confirmPassword) != 0) {
            invalid("confirmPasswordError",
                    "The two passwords must be the same.");
        }

        if (valid) {
            Account *account = accountCreate(db, name,
                                             email, username, password);

            if (account) {
                responseSetStatus(*res, FOUND);
                responseAddHeader(*res, "Location", "/login");
                templateDel(template);
                accountDel(account);
                return;
            } else {
                invalid("nameError",
                        "Unexpected error. Please try again later.");
            }
        }
    }

    responseSetBody(*res, templateRender(template));
    templateDel(template);
}
