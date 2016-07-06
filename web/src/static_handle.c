#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "cgi.h"
#include "web.h"

#include "bs/bs.h"
#include "kv/kv.h"
#include "request/request.h"
#include "response/response.h"
#include "template/template.h"

void do_response(Request *req, Response ** res, sqlite3 *db)
{
    // EXIT ON SHENANIGANS
    if (strstr(req->uri, "../")) return;

    //char *filename = req->uri + 1;
	char filename[100] = "../web/public";
	strcat(filename, req->uri);

    // EXIT ON DIRS
    struct stat sbuff;

    if (stat(filename, &sbuff) < 0 || S_ISDIR(sbuff.st_mode))
        return;

    // EXIT ON NOT FOUND
    FILE *file = fopen(filename, "r");
    if (!file) return;

    // GET LENGTH
    char *buff;
    char  lens[25];
    size_t len;

    fseek(file, 0, SEEK_END);
    len = ftell(file);
    sprintf(lens, "%ld", (long int) len);
    rewind(file);

    // SET BODY
    Response *response = responseNew();

    buff = malloc(sizeof(char) * len);
    fread(buff, sizeof(char), len, file);
    responseSetBody(response, bsNewLen(buff, len));
    fclose(file);
    free(buff);

    // MIME TYPE
    char *mimeType = "text/plain";

    len = bsGetLen(req->uri);

    if (!strncmp(req->uri + len - 4, "html", 4)) mimeType = "text/html";
    else if (!strncmp(req->uri + len - 4, "json", 4)) mimeType = "application/json";
    else if (!strncmp(req->uri + len - 4, "jpeg", 4)) mimeType = "image/jpeg";
    else if (!strncmp(req->uri + len - 3,  "jpg", 3)) mimeType = "image/jpeg";
    else if (!strncmp(req->uri + len - 3,  "gif", 3)) mimeType = "image/gif";
    else if (!strncmp(req->uri + len - 3,  "png", 3)) mimeType = "image/png";
    else if (!strncmp(req->uri + len - 3,  "css", 3)) mimeType = "text/css";
    else if (!strncmp(req->uri + len - 2,   "js", 2)) mimeType = "application/javascript";

    // RESPOND
    responseSetStatus(response, OK);
    responseAddHeader(response, "Content-Type", mimeType);
    responseAddHeader(response, "Content-Length", lens);
    responseAddHeader(response, "Cache-Control", "max-age=2592000");
    *res = response;
}
