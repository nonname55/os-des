#ifndef RESTA_H
#define RESTA_H

#include "header.h"
#include "macro.h"
#include "order.h"
#include "msgque.h"
#include "file.h"

struct Restaurant {
    int posx, posy, id;
    std::vector<Order> orders,confOrders;
    std::vector<orderMsg> confOrdersQue,unArrived;
    Restaurant() {}
    Restaurant(int _posx, int _posy, int _id) 
    {
        this->posx = _posx;
        this->posy = _posy;
        this->id = _id;
    }
    
    void manage();
    bool haveArrived() 
    {
        return random_int(0, 1) == 1 ? true : false;
    }
};

extern Restaurant restaurant;

#endif