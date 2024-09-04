//
// Created by LesPaul on 9/4/2024.
//

#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include "ConnItemRecv.h"
#include <iostream>

#define DEBUG_LEVEL 1

int ConnItemRecv::epollInCallBack(void) {
    int idx = rlen;

    int cnt = recv(fd, rbuffer + idx, BUFFER_LEN - idx, 0);
    if (cnt == 0) {
        #if DEBUG_LEVEL >= 1
            std::cout << "disconnect\n";
        #endif
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        return -1;
    }
    rlen += cnt;

    memcpy(wbuffer, rbuffer, rlen);
    wlen = rlen;
    rlen -= rlen;

    set_event(fd, MOD, EPOLLOUT);
    return cnt;
}