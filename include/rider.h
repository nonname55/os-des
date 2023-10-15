#ifndef RIDER_H
#define RIDER_H

#include "header.h"
#include "macro.h"
#include "order.h"
#include "msgque.h"

struct Rider {
    int posx, posy, id, speed;
    std::deque<std::shared_ptr<Order>> orders;
    std::vector<std::shared_ptr<Order>> toDoOrder;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool sending=false;
    bool isNewOrder;
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
        this->posx = _posx;
        this->posy = _posy;
        this->id = _id;
        this->speed = _speed;
        this->pid = _pid;
    }
    bool shouldAcOrd(std::shared_ptr<Order>& order);
    void manage();
    double H(const Order& order);

    void getOrderFromResta();

    void getDishFromResta();
    void startToSend();

    void sort_order() {
        for (const auto& order : orders) {
            order->weight = H(*order);
        }
        std::sort(orders.begin(), orders.end(), [](const auto& o1, const auto& o2) {
            return o1->weight > o2->weight;
        });
    }

    static bool check_newOrder() {
        return false;
    }
    
    static void* deliver(void* arg);
    void create_order(std::shared_ptr<Order>& newOrder) {
        ThreadArgs args = {this, newOrder};
        pthread_create(&(newOrder->thread), nullptr, deliver, &args);
        pthread_cond_wait(&cond, &lock);
        orders.push_back(std::move(newOrder));
    }
};

extern Rider rider;

#endif