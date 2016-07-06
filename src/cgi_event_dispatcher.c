#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/timerfd.h>

#include "cgi.h"
#include "async/cgi_async.h"
#include "db/db.h"
#include "dispatcher/cgi_event_dispatcher.h"
#include "factory/cgi_factory.h"
#include "kv/kv.h"
#include "models/account.h"
#include "utils/cgi_url_dltrie.h"

#define GET_TIME                                  \
    time_t t = time(NULL);                        \
    char timebuff[100];                           \
    strftime(timebuff, sizeof(timebuff),          \
             "%c", localtime(&t));

#define LOG_400(addr)                             \
    do {                                          \
        GET_TIME;                                 \
        fprintf(stdout,                           \
                "%s %s 400\n",                    \
                timebuff,                         \
                inet_ntoa(addr->sin_addr));       \
    } while (0)

#define LOG_REQUEST(addr, method, path, status)   \
    do {                                          \
        GET_TIME;                                 \
        fprintf(stdout,                           \
                "%s %s %s %s %d\n",               \
                timebuff,                         \
                inet_ntoa(addr->sin_addr),        \
                method,                           \
                path,                             \
                status);                          \
    } while (0)

char *METHODS[8] = {
    "OPTIONS", "GET", "HEAD", "POST", "PUT", "DELETE", "TRACE", "CONNECT"
};

static pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;
static int usable = 0;
static int pipefd[2];

void cgi_http_connection_init(cgi_http_connection_t *connection);
void cgi_http_connection_init5(cgi_http_connection_t *connection,
                               cgi_event_dispatcher_t *dispatcher,int sockfd,
                               struct sockaddr_in *clientaddr,socklen_t clientlen);
static void cgi_event_dispatcher_signal_handler(int sig);
void cgi_event_dispatcher_addtimer(cgi_event_dispatcher_t *, long);
void cgi_event_dispatcher_set_nonblocking(int);
void cgi_event_dispatcher_addfd(cgi_event_dispatcher_t *, int, int, int);
void cgi_event_dispatcher_modfd(cgi_event_dispatcher_t *, int, int);

void cgi_http_connection_init(cgi_http_connection_t *connection)
{
    if (connection->rsize == 0)
        connection->rbuffer = NULL;
    if (connection->wsize == 0)
        connection->wbuffer = NULL;
    connection->version = NULL;
    connection->url = NULL;
    connection->cookie = NULL;
    connection->content = NULL;
    connection->head = NULL;
    connection->read_idx = 0;
    connection->write_idx = 0;
    connection->checked_idx = 0;
    connection->start_line_idx = 0;
    connection->content_length = 0;
    connection->linger = 0;
    connection->fsize = -1;
    connection->ffd = -1;
    connection->idle_start.tv_sec = 0;
    connection->idle_start.tv_nsec = 0.0;
}

void cgi_http_connection_init5(cgi_http_connection_t *connection,
                               cgi_event_dispatcher_t *dispatcher,int sockfd,
                               struct sockaddr_in *clientaddr,socklen_t clientlen)
{
    cgi_http_connection_init(connection);
    connection->dispatcher = dispatcher;
    connection->sockfd = sockfd;
    connection->clientlen = clientlen;
    memcpy(&(connection->clientaddr),clientaddr,clientlen);
}

cgi_event_dispatcher_t *cgi_event_dispatcher_create()
{
    cgi_event_dispatcher_t *dispatcher = cgi_factory_create(EVENT_DISPATCHER);
    dispatcher->timeout = 0;
    return dispatcher;
}

void cgi_event_dispatcher_init(cgi_event_dispatcher_t *dispatcher,
                               int epfd, int listenfd, int timeout, long connection_timeout)
{
	initDB(&dispatcher->db);
    dispatcher->epfd = epfd;
    dispatcher->listenfd = listenfd;
    dispatcher->timeout = timeout;
    dispatcher->timerfd = -1;
    for (int i = 0; i < CGI_CONNECTION_SIZE; ++i) {
        dispatcher->isconn[i] = 0;
    }
    if (connection_timeout <= 0) {
        dispatcher->connection_timeout = 0;
        return;
    } else if (connection_timeout > 0 && connection_timeout < 2000) {
        dispatcher->connection_timeout = 2000; //default connection timeout
        cgi_event_dispatcher_addtimer(dispatcher, 1950);
    } else {
        dispatcher->connection_timeout = connection_timeout;
        cgi_event_dispatcher_addtimer(dispatcher, connection_timeout-50);
    }
}

void cgi_event_dispatcher_signal_handler(int sig)
{
    int eno = errno;
    char msg = sig;
    send(pipefd[1],&msg,1,0);
    errno = eno;
}

void cgi_event_dispatcher_addpipe(cgi_event_dispatcher_t *dispatcher)
{
    if (!usable) {
        pthread_mutex_lock(&mlock);
        if (!usable) {
            usable = 1;
            pthread_mutex_unlock(&mlock);
            if (socketpair(AF_UNIX, SOCK_STREAM, 0, pipefd) != -1) {
                cgi_event_dispatcher_set_nonblocking(pipefd[1]);
                cgi_event_dispatcher_addfd(dispatcher, pipefd[0], 1, 0);
            }
        } else {
            pthread_mutex_unlock(&mlock);
        }
    }
}

void cgi_event_dispatcher_addsig(int sig)
{
    signal(sig,cgi_event_dispatcher_signal_handler);
}

void cgi_event_dispatcher_addfd(cgi_event_dispatcher_t *dispatcher,
                                int fd, int in, int oneshot)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLET | EPOLLRDHUP;
    event.events |= in ? EPOLLIN : EPOLLOUT;
    if (oneshot)
        event.events |= EPOLLONESHOT;
    if (epoll_ctl(dispatcher->epfd,EPOLL_CTL_ADD,fd,&event) == -1)
        perror("epoll_ctl");
    cgi_event_dispatcher_set_nonblocking(fd);
}

void cgi_event_dispatcher_addtimer(cgi_event_dispatcher_t *dispatcher,
                                   long milliseconds)
{
    int timerfd = timerfd_create(CLOCK_MONOTONIC, O_NONBLOCK);
    if (milliseconds > 0) {
        struct itimerspec newtime;
        newtime.it_value.tv_sec = newtime.it_interval.tv_sec =
                                      milliseconds / 1000;
        newtime.it_value.tv_nsec = newtime.it_interval.tv_nsec =
                                       (milliseconds % 1000) * 1000000;
        timerfd_settime(timerfd, 0, &newtime, NULL);
    }
    cgi_event_dispatcher_addfd(dispatcher, timerfd, 1, 1);
    dispatcher->timerfd = timerfd;
}

void cgi_event_dispatcher_reset_timer(cgi_event_dispatcher_t *dispatcher)
{
    char data[sizeof(void *)];
    if (read(dispatcher->timerfd, &data, sizeof(void *)) < 0)
        data[0] = 0;
    cgi_event_dispatcher_modfd(dispatcher,
                               dispatcher->timerfd, EPOLLIN);
}

void cgi_event_dispatcher_rmfd(cgi_event_dispatcher_t *dispatcher,
                               int fd)
{
    if (epoll_ctl(dispatcher->epfd, EPOLL_CTL_DEL, fd, NULL) == -1)
        perror("epoll_ctl");
}

void cgi_event_dispatcher_modfd(cgi_event_dispatcher_t *dispatcher,
                                int fd, int ev)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
    epoll_ctl(dispatcher->epfd, EPOLL_CTL_MOD, fd, &event);
}

void cgi_event_dispatcher_set_nonblocking(int fd)
{
    int fsflags = fcntl(fd, F_GETFL);
    fsflags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, fsflags) == -1)
        perror("fcntl");
}

static void session(Request *req, sqlite3 *db)
{
    char *sid = kvFindList(req->cookies, "sid");

    if (sid)
        req->account = accountGetBySId(db, sid);
}

static void handle(void *__connection)
{
	cgi_http_connection_t *connection = (cgi_http_connection_t *)__connection;
    int  nread;
    char buff[20480];
	struct sockaddr_in * addr = &(connection->clientaddr);

    if ((nread = recv(connection->sockfd, buff, sizeof(buff), 0)) < 0) {
        fprintf(stderr, "error: read failed\n");
    } else if (nread > 0) {
        buff[nread] = '\0';

        Request *req = requestNew(buff);

        if (!req) {
            send(connection->sockfd, "HTTP/1.0 400 Bad Request\r\n\r\nBad Request", 39, 0);
            LOG_400(addr);
        } else {
            Response *response = NULL;
            session(req, connection->dispatcher->db);
            /*check keep-alive*/
            char *tmp;
            if ((tmp = kvFindList(req->headers, "Connection"))) {
                if(strcmp(tmp, "Keep-Alive") == 0) {
					connection->linger = 1;
                    printf("keep-alive\n");
				}
            }
			
			if (!response) {
				cgi_url_dltrie_t *url_dltrie = cgi_url_dltrie_default_root();
                cgi_handler_t handler = NULL;
				cgi_url_dltrie_find(url_dltrie, req->uri, &handler);
				handler(req, &response, connection->dispatcher->db);
            }

            if (!response) {
                send(connection->sockfd, "HTTP/1.0 404 Not Found\r\n\r\nNot Found!", 36, 0);
                LOG_REQUEST(addr, METHODS[req->method], req->path, 404);
            } else {
                LOG_REQUEST(addr, METHODS[req->method], req->path,
                            response->status);

                responseWrite(response, connection->sockfd);
                responseDel(response);
            }

            requestDel(req);
        }
    }
    //cgi_event_dispatcher_rmfd(server->epollfd, connection->sockfd);
    if(connection->linger == 1) {
        Dispatcher.modfd(connection->dispatcher, connection->sockfd, EPOLLIN);
        clock_gettime(CLOCK_REALTIME, &connection->idle_start);
    } else {
        close(connection->sockfd);
        connection->dispatcher->isconn[connection->sockfd] = 0;
    }
}

void cgi_event_dispatcher_loop(cgi_event_dispatcher_t *dispatcher)
{
    int stop = 0;
    int nfds;
    int tmpfd;
    int cfd;
    int retcode;
    char istimertrigger = 0;
    long interval = 0.0;
    struct timespec current_time;
    struct epoll_event event;
    struct sockaddr_in clientaddr;
    socklen_t clientlen;
    char signum;

    dispatcher->async = (async_p)cgi_factory_create(ASYNC);
    clientlen = sizeof(clientaddr); //must do that!

    while (!stop) {
        nfds = epoll_wait(dispatcher->epfd, dispatcher->events,
                          dispatcher->evsize, dispatcher->timeout);
        for (int i = 0; i < nfds; ++i) {
            event = dispatcher->events[i];
            tmpfd = event.data.fd;
            if (tmpfd == dispatcher->listenfd) {
                while ((cfd = accept(tmpfd, (struct sockaddr*)&clientaddr, &clientlen)) > 0) {
                    cgi_event_dispatcher_addfd(dispatcher, cfd, 1, 1);
                    cgi_http_connection_init5(dispatcher->connections + cfd,
                                              dispatcher,
                                              cfd, &clientaddr, clientlen);
                    dispatcher->isconn[cfd] = 1;
                }
                if (cfd == -1) {
                    if (errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO && errno != EINTR)
                        perror("accept");
                }
            } else if (tmpfd == dispatcher->timerfd) {
                cgi_event_dispatcher_reset_timer(dispatcher);
                istimertrigger = 1;
            } else if (event.events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                cgi_event_dispatcher_rmfd(dispatcher, tmpfd);
            } else if (event.events & EPOLLIN) {
                if (tmpfd == pipefd[0]) {
                    retcode = recv(pipefd[0], &signum, sizeof(signum), 0);
                    if (retcode <= 0)
                        continue;
                    pid_t pid = 0;
                    int status = 0, s = 0;					
                    switch (signum) {
                        case SIGCHLD:
                            pid = wait(&status);
                            s = WEXITSTATUS(status);
                            printf("child's pid =%d :  exit status=%d\n", pid, s);
                            break;
                        case SIGHUP:
                            break;

                        case SIGTERM:
                        case SIGINT:
                            stop = 1;
                            break;

                        default:
                            break;
                    }
                    continue;
                }
                Async.run(dispatcher->async, (void (*)(void *))handle, (void *)(dispatcher->connections + tmpfd));
            } else {
                printf("Error!");
            }
        }
        if (istimertrigger == 1) {
            for (int i = 0; i < CGI_CONNECTION_SIZE; ++i) {
                if (dispatcher->isconn[i] == 1) {
                    tmpfd = i;
                    if ((dispatcher->connections + tmpfd)->idle_start.tv_nsec != 0) {
                        clock_gettime(CLOCK_REALTIME, &current_time);
                        interval = (current_time.tv_sec - (dispatcher->connections + tmpfd)->idle_start.tv_sec) * 1000;
                        interval = interval - (dispatcher->connections + tmpfd)->idle_start.tv_nsec/1000000 + (current_time.tv_nsec/1000000);
                        if (interval >= dispatcher->connection_timeout) {
                            close(tmpfd);
                            cgi_http_connection_init(dispatcher->connections + tmpfd);
                            dispatcher->isconn[i] = 0;
                            printf("(fd : %d) is timeout.\n", tmpfd);
                        }
                    }
                }
            }
            istimertrigger = 0;
        }
    }
    Async.finish(dispatcher->async);
}

void cgi_event_dispatcher_destroy(cgi_event_dispatcher_t *dispatcher)
{
    cgi_factory_destroy(dispatcher, EVENT_DISPATCHER);
}

struct __DISPATCHER_API__ Dispatcher = {
    .create = cgi_event_dispatcher_create,
    .init = cgi_event_dispatcher_init,
    .addfd = cgi_event_dispatcher_addfd,
    .modfd = cgi_event_dispatcher_modfd,
    .rmfd = cgi_event_dispatcher_rmfd,
    .addpipe = cgi_event_dispatcher_addpipe,
    .addsig = cgi_event_dispatcher_addsig,
    .start = cgi_event_dispatcher_loop,
    .destroy = cgi_event_dispatcher_destroy,
};
