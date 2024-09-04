//
// Created by LesPaul on 9/4/2024.
//

#ifndef WEBSERVERDEMO_CONNITEMRECV_H
#define WEBSERVERDEMO_CONNITEMRECV_H

#include "ConnItem.h"

class ConnItemRecv : public ConnItem {
public:
    virtual int epollInCallBack() override;
};


#endif //WEBSERVERDEMO_CONNITEMRECV_H
