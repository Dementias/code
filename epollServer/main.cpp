#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define BUFLEN 20
#define SERVER_PORT 5555
#define MAX_EVENTS 2048
#define MAX_CONN 1024

int set_nonblocking (int sckt)
{
    if (fcntl(sckt, F_SETFD, fcntl(sckt, F_GETFD, 0) | O_NONBLOCK) == -1)
        return -1;

    return 0;
}

void epoll_ctl_add(int epfd, int fd, uint32_t events)
{
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        perror("epoll_ctl()\n");
        exit(1);
    }
}

void set_sockaddr (struct sockaddr_in *addr)
{
    bzero((char *) addr, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = INADDR_ANY;
    addr->sin_port = htons(SERVER_PORT);
}

void run_server()
{
    int i;
    int n;
    int epfd;
    int nfds;
    int listening_socket;
    int connection_socket;
    unsigned int socklen;
    char buf[BUFLEN];
    struct sockaddr_in srv_addr;
    struct sockaddr_in cli_addr;
    struct epoll_event events[MAX_EVENTS];

    listening_socket = socket(AF_INET, SOCK_STREAM, 0);

    set_sockaddr(&srv_addr);
    bind(listening_socket, (struct sockaddr *)&srv_addr, sizeof(srv_addr));

    set_nonblocking(listening_socket);
    listen(listening_socket, MAX_CONN);

    epfd = epoll_create(1);
    epoll_ctl_add(epfd, listening_socket, EPOLLIN | EPOLLOUT | EPOLLET);

    socklen = sizeof(cli_addr);

    while(1)
    {
        // - waiting for an event
        nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);

        for (int i = 0; i < nfds; ++i)
        {
            if (events[i].data.fd == listening_socket)
            {
                // new connection
                connection_socket = accept(listening_socket, (struct sockaddr *)&cli_addr, &socklen);

                inet_ntop(AF_INET, (char *)&(cli_addr.sin_addr), buf, sizeof(cli_addr));

                std::cout << "[+] connected with " << buf << ":" << ntohs(cli_addr.sin_port) << std::endl;
                set_nonblocking(connection_socket);
                epoll_ctl_add(epfd, connection_socket, EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP);
            }

            else if (events[i].events & EPOLLIN) {
                // EPOLLIN event
                bzero(buf, sizeof(buf));
                n = read(events[i].data.fd, buf, sizeof(buf));

                if (n <= 0)
                    break;

                else
                {
                    std::cout << "[+] data:" << buf << std::endl;
                    write(events[i].data.fd, buf, strlen(buf));
                }
            }
            else
                std::cout << "I dont know what happened" << std::endl;

            //check for closing connections

            if (events[i].events & (EPOLLRDHUP | EPOLLHUP))
            {
                std::cout << "[-] connection closed" << std::endl;
                epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                close(events[i].data.fd);
                continue;
            }
        }
    }
}



int main()
{
    run_server();
}