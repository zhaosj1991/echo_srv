#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h> //close
#include "log.h"

#define ECHO_DEFAULT_PORT   12345
#define EPOLL_EVENT_SIZE    1000 
#define EPOLL_WAIT_TIMEOUT  10000    //ms
#define BUF_SIZE            4096

#define ASSERT(x)  {if(!(x)) {                      \
                       LOG_STDERR("ASSERT Err!");    \
                       exit(-1);                   \
                   }}

int setnoblock(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

int epollAdd(int epfd, int sockfd)
{
    struct epoll_event ev;
    ev.data.fd = sockfd;
    ev.events = EPOLLIN | EPOLLET | EPOLLHUP;
    setnoblock(sockfd);
    return epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
}

int epollDel(int epfd, int sockfd)
{
    return epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);
}

int main(int argc, char * argv [])
{
    // signal(SIGINT, SIG_IGN);
    uint16_t port = ECHO_DEFAULT_PORT;
    if (argc > 1)
    {
        port = atoi(argv[1]);
    }

    int udpSock = socket(PF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in uAddr;
    bzero(&uAddr, sizeof(uAddr));
    uAddr.sin_family = AF_INET;
    uAddr.sin_port = htons(port);
    uAddr.sin_addr.s_addr = INADDR_ANY;
    ASSERT(bind(udpSock, (struct sockaddr *)&uAddr, sizeof(uAddr)) == 0);

    int tcpSock = socket(PF_INET, SOCK_STREAM, 0);  
    struct sockaddr_in tAddr;
    tAddr.sin_addr.s_addr = INADDR_ANY;
    tAddr.sin_family = AF_INET;
    tAddr.sin_port = htons(port);
    ASSERT(bind(tcpSock, (struct sockaddr *)&tAddr, sizeof(tAddr)) == 0);
    ASSERT(listen(tcpSock, 5) == 0);

    int epfd = epoll_create(5);
    ASSERT(epfd != -1);
    ASSERT(epollAdd(epfd, udpSock) == 0);
    ASSERT(epollAdd(epfd, tcpSock) == 0);

    struct epoll_event epEvent[EPOLL_EVENT_SIZE];
    bzero(epEvent, sizeof(struct epoll_event) * EPOLL_EVENT_SIZE);    
    while(1)
    {
        char buf[BUF_SIZE] = {0};
    
        int num = epoll_wait(epfd, epEvent, EPOLL_EVENT_SIZE, EPOLL_WAIT_TIMEOUT);
        if (num < 0)
        {
            LOG_STDERR("epoll_wait failed!\n");
            break;
        }
        
        if(num == 0)
            continue;

        for (int i = 0; i < num; i++)
        {
            // if (epEvent[i].data.fd == udpSock && epEvent[i].events == EPOLLIN)
            if (epEvent[i].data.fd == udpSock)
            {
                struct sockaddr_in addr;
                socklen_t len = sizeof(addr);
                bzero(&addr, sizeof(addr));
                bzero(buf, BUF_SIZE);
                int size = recvfrom(udpSock, buf, BUF_SIZE, 0, 
                    (struct sockaddr *)&addr, &len);
                                                
                if (size > 0)
                {
                    ASSERT(sendto(udpSock, buf, size, 0, 
                        (struct sockaddr *)&addr, len) == size);
                }
            } 
            else if (epEvent[i].data.fd == tcpSock)
            {
                LOG_INFO("step into event accept!\n");
                int conn = accept(tcpSock, NULL, NULL);
                ASSERT(conn != -1);
                ASSERT(epollAdd(epfd, conn) == 0);
            }
            else if (epEvent[i].events == EPOLLIN)
            {
                int conn = epEvent[i].data.fd;
                int size = 0;
                while (1)
                {
                    bzero(buf, BUF_SIZE);
                    size = recv(conn, buf, BUF_SIZE, 0);
                    if (size < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            break;
                        }

                        ASSERT(epollDel(epfd, conn) == 0);
                        close(conn);
                        break;
                    }

                    if (size == 0)
                    {
                        ASSERT(epollDel(epfd, conn) == 0);
                        close(conn);
                        break;
                    }

                    send(conn, buf, size, 0);
                }    
            }
            else if (epEvent[i].events & EPOLLHUP)
            {
                int conn = epEvent[i].data.fd;
                ASSERT(epollDel(epfd, conn) == 0);
                close(conn);
                LOG_INFO("close conn fd!\n");
            }
        }
    }

    close(tcpSock);
    return 0;
}