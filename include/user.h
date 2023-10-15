#ifndef USER_H
#define USER_H

#include "header.h"
#include "msgque.h"
#include "util.h"
#include "macro.h"

struct User {
    int posx, posy, id;
    int cnt=9;
    User() {}
    User(int _posx, int _posy, int _id) {
        this->posx = _posx;
        this->posy = _posy;
        this->id = _id;
    }

    void manage();
    bool isOrder();
};

extern User user;

#endif