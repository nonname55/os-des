#ifndef ORDER_H
#define ORDER_H

#include "header.h"
#include "macro.h"
#include "util.h"
#include "graph.h"

struct Order {
    int tarx, tary, required_time, userId, riderId, restaurantId;
    int dis = 0;
    bool sent=false;
    double weight = 0.0;

    pthread_cond_t cond;
    pthread_t thread;

    Order() {
        pthread_cond_init(&cond, nullptr);
        auto validPos = graph.getValidPos();
        tarx = validPos.first;
        tary = validPos.second;
        required_time = random_int(1, 10);
    }

    Order(int _tarx, int _tary, int _required_time) {
        this->tarx = _tarx;
        this->tary = _tary;
        this->required_time = _required_time;
    }
};

#endif