//
// Created by LesPaul on 9/4/2024.
//

#include "Server.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "ConnItem/ConnItemListen.h"
#include "ConnItem/ConnItem.h"
#include <cstring>
#include <iostream>
#include <chrono>

#define DEBUG_LEVEL 1

clock_t st;

int Server::init_server(unsigned short port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd) {
        #if DEBUG_LEVEL >= 1
            std::cout << "failed to create sockfd\n";
        #endif

        return -1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (-1 == bind(sockfd, (struct sockaddr*)&server_addr, sizeof (struct sockaddr))) {
        #if DEBUG_LEVEL >= 1
            std::cout << "failed to bind sockfd and server_addr\n";
        #endif

        return -1;
    }

    listen(sockfd, 10);

    return sockfd;
}

int Server::set_event(int fd, int op, int event) {
    struct epoll_event ev;
    if (op == ADD) {
        ev.events = event;
        ev.data.fd = fd;
        if (-1 == epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev)) {
            #if DEBUG_LEVEL >= 1
                printf("failed to add event\n");
            #endif
            return -1;
        }
    } else {
        ev.events = event;
        ev.data.fd = fd;
        if (-1 == epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev)) {
            #if DEBUG_LEVEL >= 1
                printf("failed to modify event\n");
            #endif
            return -1;
        }
    }

    return 1;
}

int Server::run(unsigned short port) {
    epfd = epoll_create(1);
    if (-1 == epfd) {
        #if DEBUG_LEVEL >= 1
            std::cout << "failed to create epfd" << std::endl;
        #endif
        return -1;
    }

    for (int i = 0; i < PORT_CNT; i ++) {
        int listenfd = init_server(port + i);
        ConnItem* connItem = new ConnItemListen();
        connItem->fd = listenfd;
        connItem->epfd = epfd;
        memset(connItem->rbuffer, 0, sizeof connItem->rbuffer);
        connItem->rlen = 0;
        memset(connItem->wbuffer, 0, sizeof connItem->wbuffer);
        connItem->wlen = 0;
        connList[listenfd] = connItem;
        set_event(listenfd, ADD, EPOLLIN);
    }

    epoll_event events[1024];
    memset(events, 0, sizeof events);

    while (true) {

        int nready = epoll_wait(epfd, events, 1024, -1);
        for (int i = 0; i < nready; i ++) {
            int connfd = events[i].data.fd;
            if (events[i].events & EPOLLIN) {

                int count = connList[connfd]->epollInCallBack();
                #if DEBUG_LEVEL >= 2
                    printf("recv count: %d <-- buffer: %s\n", count, connlist[connfd].rbuffer);
                #endif
            } else if (events[i].events & EPOLLOUT) {
                #if DEBUG_LEVEL >= 2
                    printf("send --> buffer: %s\n",  connlist[connfd].wbuffer);
                #endif
                int count = connList[connfd]->epollOutCallBack();
            }
        }
    }

    return 1;
}