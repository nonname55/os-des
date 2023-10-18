#ifndef DESC_H
#define DESC_H

#include "macro.h"
#include "header.h"

struct RiderDes {
    int riderID, posx, posy;
};

struct UserDes {
    bool isNew;
    int posx, posy, restID, reqTime, userID;
};

struct OrderDes {
    int userID, riderID, reqArrTime, restID;
};

struct RestDes {
    int id;
    std::vector<OrderDes> orders;
};

struct Des {
    std::vector<RiderDes> riders;
    std::vector<UserDes> users;
    std::vector<RestDes> rest;

    void readFromLog();
};

#endif