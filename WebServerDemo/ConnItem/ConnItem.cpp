//
// Created by LesPaul on 9/4/2024.
//

#include "ConnItem.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <vector>
#include <iostream>

#define DEBUG_LEVEL 1

std::vector<ConnItem*> connList = std::vector<ConnItem*>(1048576);

int ConnItem::epollInCallBack() {}

int ConnItem::set_event(int fd, int op, int event) {
    epoll_event ev;
    if (op == ADD) {
        ev.events = event;
        ev.data.fd = fd;
        if (-1 == epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev)) {
            #if DEBUG_LEVEL >= 1
                std::cout << "failed to add event\n";
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

int ConnItem::epollOutCallBack(void) {
    char* buffer = wbuffer;
    int cnt = send(fd, wbuffer, wlen, 0);
    set_event(fd, MOD, EPOLLIN);
    return cnt;
}