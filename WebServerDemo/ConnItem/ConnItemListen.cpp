//
// Created by LesPaul on 9/4/2024.
//

#include "ConnItem.h"
#include "ConnItemListen.h"
#include "ConnItemRecv.h"
#include <netinet/in.h>
#include <sys/epoll.h>
#include <cstring>
#include <iostream>

#define DEBUG_LEVEL 1

int ConnItemListen::epollInCallBack(void) {
    sockaddr_in client_addr;
    socklen_t len = sizeof client_addr;
    int clientfd = accept(fd, (struct sockaddr*)&client_addr, &len);
    if (-1 == clientfd) {
        #if DEBUG_LEVEL >= 1
            std::cout << "failed to create clientfd\n";
        #endif
        return -1;
    }

    ConnItem* connItem = new ConnItemRecv();
    connItem->fd = clientfd;
    connItem->epfd = epfd;
    memset(connItem->rbuffer, 0, BUFFER_LEN);
    connItem->rlen = 0;
    memset(connItem->wbuffer, 0, BUFFER_LEN);
    connItem->wlen = 0;
    connList[clientfd] = connItem;

    set_event(clientfd, ADD, EPOLLIN);

#if DEBUG_LEVEL >= 1
    if (999 == clientfd % 1000) {
        std::cout << "connections: " << clientfd << std::endl;
    }
#endif

    return clientfd;
}