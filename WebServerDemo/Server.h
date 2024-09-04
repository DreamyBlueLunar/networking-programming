//
// Created by LesPaul on 9/4/2024.
//

#ifndef WEBSERVERDEMO_SERVER_H
#define WEBSERVERDEMO_SERVER_H

#include <vector>
#include "ConnItem/ConnItem.h"

class Server {
private:
    enum{BUFFER_LEN = 512, CONN_LEN = 1024,
            DEBUG_LEVEL = 1, PORT_CNT = 20,
            MOD = 1, ADD = 0};
    int epfd;

public:
    int init_server(unsigned short port);
    int set_event(int fd, int op, int event);
    int run(unsigned short port);
};


#endif //WEBSERVERDEMO_SERVER_H
