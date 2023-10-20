#ifndef ORDER_H
#define ORDER_H

#include "header.h"
#include "macro.h"
#include "util.h"
#include "graph.h"

struct Order {
    int user_id, rider_id, rest_id, order_id;
    int user_x, user_y, rest_x, rest_y, required_time, done_time;
    bool is_take = false;
    bool sent = false;
    double weight = 0.0;

    pthread_cond_t cond;
    pthread_t thread;

    Order() {}

    Order(int _tarx, int _tary, int _required_time, int _done_time) 
    {
        this->user_x = _tarx;
        this->user_y = _tary;
        this->required_time = _required_time;
        this->done_time = _done_time;
    }
};

#endif