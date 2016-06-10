#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>

#include "cgi.h"
#include "factory/cgi_factory.h"
#include "utils/cgi_url_dltrie.h"

// FIXME: move dynamic library routines to dedicated file
#include <dlfcn.h>

static cgi_url_dltrie_t *url_trie = NULL;

static char *cgi_url_strpbrk(char *url);

cgi_url_dltrie_t *cgi_url_dltrie_create()
{
    cgi_url_dltrie_t *elem = cgi_factory_create(URL_DLTRIE);
    elem->handler = NULL;
    elem->dlhandle = NULL;
    return elem;
}

cgi_url_dltrie_t *cgi_url_dltrie_default_root()
{
    if (url_trie == NULL) {
        cgi_url_dltrie_init(&url_trie);
    }
    return url_trie;
}

char *mystrcat(char *str1, char * str2)
{
    char *new_str;
    if((new_str = malloc(strlen(str1)+strlen(str2)+1)) != NULL) {
        new_str[0] = '\0';   // ensures the memory is an empty string
        strcat(new_str,str1);
        strcat(new_str,str2);
    } else {
        fprintf(stderr, "malloc failed!\n");
        exit(1);
    }
    return new_str;
}
/* dlopen with RTLD_LAZY and check error */
void *mydlopen(char *path)
{
    void *dlhandle = NULL;
    char *errstr;

    dlhandle = dlopen(path, RTLD_LAZY);
    errstr = dlerror();
    if (errstr != NULL) {
        printf ("A dynamic linking error occurred: (%s)\n", errstr);
        exit(1);
    }
    return dlhandle;
}

#define WEB_PLUGIN_DIR "/web/plugins/"
#define SUFFIX ".so"
void cgi_url_dltrie_init(cgi_url_dltrie_t **head_ptr)
{
    char absolute_path[PATH_MAX];
    char *so_path; //shared object path
    void *dlhandle = NULL;

    realpath("../", absolute_path);
    so_path = mystrcat(absolute_path, WEB_PLUGIN_DIR "web_index" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/",
                          dlsym(dlhandle, "do_response"), dlhandle);
    cgi_url_dltrie_insert(head_ptr, "/index.html",
                          dlsym(dlhandle, "do_response"), dlhandle);

    so_path = mystrcat(absolute_path, WEB_PLUGIN_DIR "web_error" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/error.html",
                          dlsym(dlhandle, "do_response"), dlhandle);

    so_path =mystrcat(absolute_path, WEB_PLUGIN_DIR "web_signin" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/signIn.html",
                          dlsym(dlhandle, "do_response"), dlhandle);

    so_path = mystrcat(absolute_path, WEB_PLUGIN_DIR "web_signup" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/signUp.html",
                          dlsym(dlhandle, "do_response"), dlhandle);

    so_path = mystrcat(absolute_path, WEB_PLUGIN_DIR "web_verify_signin" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/verifySignIn",
                          dlsym(dlhandle, "do_response"), dlhandle);

    so_path = mystrcat(absolute_path, WEB_PLUGIN_DIR "web_verify_signup" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/verifySignUp",
                          dlsym(dlhandle, "do_response"), dlhandle);

    so_path = mystrcat(absolute_path, WEB_PLUGIN_DIR "web_default" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/favicon.ico",
                          dlsym(dlhandle, "do_response"), dlhandle);
    cgi_url_dltrie_insert(head_ptr, "/css/index.css",
                          dlsym(dlhandle, "do_response"), dlhandle);
    cgi_url_dltrie_insert(head_ptr, "/css/reset.css",
                          dlsym(dlhandle, "do_response"), dlhandle);
    cgi_url_dltrie_insert(head_ptr, "/js/signUp.js",
                          dlsym(dlhandle, "do_response"), dlhandle);
    // FIXME: call dlclose()
}

char *cgi_url_strpbrk(char *url)
{
    char *scanner = url;
    while (*scanner != '/' && *scanner != '\0')
        ++scanner;
    if (*scanner == '/')
        ++scanner;
    return scanner;
}

void cgi_url_dltrie_insert(cgi_url_dltrie_t **head_ptr, char *url,
                           cgi_handler_t handler, void *dlhandle)
{
    if (head_ptr == NULL || url == NULL || *url == '\0') return;

    char *scanner = NULL;
    cgi_url_dltrie_t *head = NULL;
    uint32_t diff;
    while (1) {
        scanner = cgi_url_strpbrk(url);
        head = *head_ptr;
        diff = scanner - url;
        if (head == NULL) {
            *head_ptr = cgi_url_dltrie_create();
            head = *head_ptr;
            snprintf(head->key, diff + 1, "%s", url);
        } else if (memcmp(head->key, url, diff)) {
            head_ptr = &CGI_DLTRIE_SIBLING(head, linker);
            continue;
        }
        if (*scanner == '\0') {
            head->handler = handler;
            head->dlhandle = dlhandle;
            break;
        }
        head_ptr = &CGI_DLTRIE_CHILD(head, linker);
        url = scanner;
    }
}

cgi_handler_t cgi_url_dltrie_find(cgi_url_dltrie_t *head,char *url)
{
    char *scanner = NULL;
    uint32_t diff;
    cgi_handler_t handler = NULL;
    while (1) {
        scanner = cgi_url_strpbrk(url);
        diff = scanner - url;
        if (head == NULL || url == NULL || *url == '\0')
            break;
        if (memcmp(head->key, url, diff)) {
            head = CGI_DLTRIE_SIBLING(head, linker);
            continue;
        }
        if (*scanner == '\0') {
            handler = head->handler;
            break;
        }
        head = CGI_DLTRIE_CHILD(head, linker);
        url = scanner;
    }
    return handler;
}

void cgi_url_dltrie_delete(cgi_url_dltrie_t *elem)
{
    cgi_factory_destroy(elem,URL_DLTRIE);
}

void cgi_url_dltrie_destroy(cgi_url_dltrie_t **head_ptr)
{
}
