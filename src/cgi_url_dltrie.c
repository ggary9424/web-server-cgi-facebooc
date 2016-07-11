#include <fcntl.h>
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

void cgi_url_dltrie_init(cgi_url_dltrie_t **head_ptr)
{
    cgi_url_dltrie_load(head_ptr, "route.conf");

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

void cgi_url_dltrie_load(cgi_url_dltrie_t **head_ptr, char *filename) {
    char *file_path = mystrcat(CGI_WEB_ROOT, filename);
    char stmp[100] = "";
    char *url = "";
    char *so_file = ""; //shared object name, jsut name
    char so_path[128] = ""; //shared object path
    void *dlhandle = NULL;

    FILE * fptr = fopen(file_path, "r");

    if (!fptr) {
        perror ("Error opening file");
        exit(1);
    }
    while (fgets(stmp, 100, fptr) != NULL) {
        so_file = stmp;
        while (*so_file != ' ')
            ++so_file;
        *so_file = '\0';
        ++so_file;
        url = stmp;
        so_file[strlen(so_file) - 1] = '\0';
        memset(so_path, 0, sizeof(so_path));
        strcat(so_path, CGI_WEB_PLUGIN_DIR);
        strcat(so_path, so_file);
        strcat(so_path, SUFFIX);

        dlhandle = mydlopen(so_path);
        cgi_url_dltrie_insert(head_ptr, url,
                              dlsym(dlhandle, "do_response"), dlhandle);
    }

    fclose(fptr);
    free(file_path);
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
