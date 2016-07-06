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
    so_path = mystrcat(absolute_path, WEB_PLUGIN_DIR "home" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/",
                          dlsym(dlhandle, "do_response"), dlhandle);

    so_path = mystrcat(absolute_path, WEB_PLUGIN_DIR "dashboard" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/dashboard",
                          dlsym(dlhandle, "do_response"), dlhandle);

    so_path =mystrcat(absolute_path, WEB_PLUGIN_DIR "profile" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/profile",
                          dlsym(dlhandle, "do_response"), dlhandle);

    so_path = mystrcat(absolute_path, WEB_PLUGIN_DIR "unlike" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/unlike",
                          dlsym(dlhandle, "do_response"), dlhandle);

    so_path = mystrcat(absolute_path, WEB_PLUGIN_DIR "like" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/like",
                          dlsym(dlhandle, "do_response"), dlhandle);

    so_path = mystrcat(absolute_path, WEB_PLUGIN_DIR "connect" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/connect",
                          dlsym(dlhandle, "do_response"), dlhandle);

    so_path = mystrcat(absolute_path, WEB_PLUGIN_DIR "search" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/search",
                          dlsym(dlhandle, "do_response"), dlhandle);

    so_path = mystrcat(absolute_path, WEB_PLUGIN_DIR "login" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/login",
                          dlsym(dlhandle, "do_response"), dlhandle);

    so_path = mystrcat(absolute_path, WEB_PLUGIN_DIR "logout" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/logout",
                          dlsym(dlhandle, "do_response"), dlhandle);

    so_path = mystrcat(absolute_path, WEB_PLUGIN_DIR "signup" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/signup",
                          dlsym(dlhandle, "do_response"), dlhandle);

    so_path = mystrcat(absolute_path, WEB_PLUGIN_DIR "search" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/search",
                          dlsym(dlhandle, "do_response"), dlhandle);

    so_path = mystrcat(absolute_path, WEB_PLUGIN_DIR "notFound" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/notFound",
                          dlsym(dlhandle, "do_response"), dlhandle);

    so_path = mystrcat(absolute_path, WEB_PLUGIN_DIR "post" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/post",
                          dlsym(dlhandle, "do_response"), dlhandle);

    so_path = mystrcat(absolute_path, WEB_PLUGIN_DIR "profile" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/profile",
                          dlsym(dlhandle, "do_response"), dlhandle);

    so_path = mystrcat(absolute_path, WEB_PLUGIN_DIR "static_handle" SUFFIX);
    dlhandle = mydlopen(so_path);
    cgi_url_dltrie_insert(head_ptr, "/css/normalize.css",
                          dlsym(dlhandle, "do_response"), dlhandle);
    cgi_url_dltrie_insert(head_ptr, "/css/main.css",
                          dlsym(dlhandle, "do_response"), dlhandle);
    cgi_url_dltrie_insert(head_ptr, "/favicon.ico",
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

void cgi_url_dltrie_find(cgi_url_dltrie_t *head,char *url, cgi_handler_t *_handler)
{
    char *scanner = NULL;
    uint32_t diff;
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
            *_handler  = head->handler;
            break;
        }
        head = CGI_DLTRIE_CHILD(head, linker);
        url = scanner;
    }
    return;
}

void cgi_url_dltrie_delete(cgi_url_dltrie_t *elem)
{
    cgi_factory_destroy(elem,URL_DLTRIE);
}

void cgi_url_dltrie_destroy(cgi_url_dltrie_t **head_ptr)
{
}
