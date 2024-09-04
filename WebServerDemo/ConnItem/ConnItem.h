//
// Created by LesPaul on 9/4/2024.
//

#ifndef WEBSERVERDEMO_CONNITEM_H
#define WEBSERVERDEMO_CONNITEM_H

#include <vector>

class ConnItem {
public:
    enum{BUFFER_LEN = 512, DEBUG_LEVEL = 1,
        MOD = 1, ADD = 0};
    int epfd;
    int fd;

    char rbuffer[BUFFER_LEN];
    int rlen;
    char wbuffer[BUFFER_LEN];
    int wlen;

    virtual int epollInCallBack(void);
    int epollOutCallBack(void);
    int set_event(int fd, int op, int event);

    ConnItem() {}
    virtual ~ConnItem() {}
};

extern std::vector<ConnItem*> connList;

#endif //WEBSERVERDEMO_CONNITEM_H
