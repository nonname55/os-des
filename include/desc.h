#ifndef DESC_H
#define DESC_H

#include "macro.h"

#define MAX_REST_ORDER_NUM 64

struct OrderDes {
    int sx, sy, tx, ty, required_time, userId, riderId, restaurantId;
};

struct UserDes {
    int x, y, id;
};

struct RestDes {
    int x, y, id;
    OrderDes orders[MAX_REST_ORDER_NUM];
};

struct RiderDes {
    int x, y, id;
    OrderDes orders[MAX_REST_ORDER_NUM];
};

struct Descriptor {
    RiderDes riders[RIDER_NUM];
    RestDes rests[RESTAURANT_NUM];
    UserDes users[USER_NUM];

    void log();
    void restore();
};

#endif