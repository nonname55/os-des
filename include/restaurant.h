#ifndef RESTA_H
#define RESTA_H

#include "header.h"
#include "macro.h"
#include "order.h"
#include "msgque.h"
#include "file.h"

struct Restaurant {
    int self_x, self_y, self_id, self_get_x, self_get_y;
    std::vector<Order> orders;
    Restaurant() {}
    
    void manage();
    bool haveArrived() 
    {
        return random_int(0, 1) == 1 ? true : false;
    }
};

extern Restaurant restaurant;

#endif