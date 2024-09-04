//
// Created by LesPaul on 9/4/2024.
//

#ifndef WEBSERVERDEMO_CONNITEMLISTEN_H
#define WEBSERVERDEMO_CONNITEMLISTEN_H

#include "ConnItem.h"

class ConnItemListen : public ConnItem {
public:
    virtual int epollInCallBack() override;
};


#endif //WEBSERVERDEMO_CONNITEMLISTEN_H
