#ifndef RIDER_H
#define RIDER_H

#include "header.h"
#include "macro.h"
#include "order.h"
#include "msgque.h"
#include "file.h"
#include "process.h"

struct Rider {
    int self_x, self_y, self_id, speed;
    std::deque<std::shared_ptr<Order>> orders;
    std::vector<std::shared_ptr<Order>> toDoOrder;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool sending=false;
    bool isNewOrder;
    bool is_waiting = false;
    pid_t pid;
    struct ThreadArgs {
        Rider* rider;
        std::shared_ptr<Order> order;
    };

    Rider(
        int _posx = 0, 
        int _posy = 0, 
        int _id = random_int(1, 10),
        int _speed = 10,
        int _pid = 0
    ) {
        this->self_x = _posx;
        this->self_y = _posy;
        this->self_id = _id;
        this->speed = _speed;
        this->pid = _pid;
    }
    bool is_accept_order(std::shared_ptr<Order>& order);

    void manage();
    
    double H(const Order& order);

    void get_order();

    void getDishFromResta();
    
    void startToSend();

    void sort_order();
    
    static void* deliver(void* arg);

    void create_order(std::shared_ptr<Order>& newOrder);

    void erase_order(pthread_t tid);

    int cal_next_order();

    int cal_delivery_time(int x, int y, const std::shared_ptr<Order> &porder);

    static bool check_newOrder() 
    {
        return false;
    }
};

extern Rider rider;

#endif