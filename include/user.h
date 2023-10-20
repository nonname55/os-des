#ifndef USER_H
#define USER_H

#include "header.h"
#include "msgque.h"
#include "util.h"
#include "macro.h"
#include "restaurant.h"
#include "file.h"

struct User {
    int self_x, self_y, self_id, self_get_x, self_get_y;

    int cnt = 9;
    User() {}

    void manage();

    bool is_order();
};

extern User user;

#endif