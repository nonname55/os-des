#ifndef ORDER_H
#define ORDER_H

#include "header.h"
#include "macro.h"
#include "util.h"
#include "graph.h"

struct Order {
    int user_id, rider_id, rest_id, order_id;
    int user_x, user_y, rest_x, rest_y, required_time, done_time;
    bool is_take;
    bool sent = false;
    double weight = 0.0;

    pthread_cond_t cond;
    pthread_t thread;

    Order() 
    {
        pthread_cond_init(&cond, nullptr);
        auto validPos = graph.getValidPos();
        user_x = validPos.first;
        user_y = validPos.second;
        required_time = random_int(1, 10);
    }

    Order(int _tarx, int _tary, int _required_time) 
    {
        this->user_x = _tarx;
        this->user_y = _tary;
        this->required_time = _required_time;
    }
};

#endif