#ifndef MACRO_H
#define MACRO_H

#include <mutex>

//basic
#define RIDER_NUM 3
#define RESTAURANT_NUM 3
#define USER_NUM 3

//process
#define P_NUM RIDER_NUM + RESTAURANT_NUM + USER_NUM
#define RIDERL USER_NUM + RESTAURANT_NUM
#define RIDERH P_NUM - 1
#define RESTAURANTL USER_NUM
#define RESTAURANTH USER_NUM + RESTAURANT_NUM - 1
#define USERL 0
#define USERH USER_NUM - 1

//test
#define DEBUG std::cout << "debug" << std::endl;

extern std::mutex cout_mutex;
#define print(msg) { \
    std::lock_guard<std::mutex> cout_lock(cout_mutex); \
    std::cout << msg; \
}

#endif