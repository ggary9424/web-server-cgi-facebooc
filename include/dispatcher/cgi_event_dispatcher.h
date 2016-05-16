#ifndef CGI_EVENT_DISPATCHER_H
#define CGI_EVENT_DISPATCHER_H

#include <stdint.h>

#include "cgi.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern struct __DISPATCHER_API__ {
    cgi_event_dispatcher_t * (*create)();
    void (*init)(cgi_event_dispatcher_t *, int, int, int, long);
    void (*addfd)(cgi_event_dispatcher_t *, int, int, int);
    void (*rmfd)(cgi_event_dispatcher_t *, int);
    void (*addpipe)(cgi_event_dispatcher_t *);
    void (*addsig)(int);
    void (*start)(cgi_event_dispatcher_t *);
    void (*destory)(cgi_event_dispatcher_t *);
} Dispatcher;

#ifdef __cplusplus
}
#endif

#endif
