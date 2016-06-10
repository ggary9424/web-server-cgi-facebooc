#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "cgi.h"
#include "utils/cgi_url_dltrie.h"
#include "dispatcher/cgi_event_dispatcher.h"

int main()
{
    cgi_event_dispatcher_t *dispatcher = Dispatcher.create();
    cgi_url_dltrie_default_root();

    int epfd = epoll_create1(0);
    assert(epfd != -1);

    int listenfd = socket(AF_INET,SOCK_STREAM, 0);
    assert(listenfd != -1);

    int retcode;
    int flag = 1;
    retcode = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                         &flag, sizeof(flag));
    assert(retcode != -1 && "Set socket REUSEADDR");
    retcode = setsockopt(listenfd, SOL_TCP, TCP_NODELAY,
                         &flag, sizeof(flag));
    assert(retcode != -1 && "Set socket NODELAY");


    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999);
    retcode = inet_aton("0.0.0.0", &addr.sin_addr);
    assert(retcode != -1);

    retcode = bind(listenfd,(struct sockaddr *) &addr, sizeof(addr));
    assert(retcode != -1);

    retcode = listen(listenfd, 1024);
    assert(retcode != -1);

    Dispatcher.init(dispatcher, epfd, listenfd, -1, -1);
    Dispatcher.addpipe(dispatcher);
    Dispatcher.addfd(dispatcher, listenfd, 1, 0);

    Dispatcher.addsig(SIGHUP);
    Dispatcher.addsig(SIGCHLD);
    Dispatcher.addsig(SIGTERM);
    Dispatcher.addsig(SIGINT);

    Dispatcher.start(dispatcher);

    Dispatcher.rmfd(dispatcher, listenfd);

    retcode = close(listenfd);
    assert(retcode != -1);
    retcode = close(epfd);
    assert(retcode != -1);

    Dispatcher.destroy(dispatcher);
    return 0;
}
