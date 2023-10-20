#include "rider.h"
#include "graph.h"

Rider rider;

bool Rider::is_accept_order(std::shared_ptr<Order>& order) 
{
    bool res = random_int(0, 1) == 1 ? true : false;
    if(res){order->sent=true;}
    return res;
}

void Rider::manage() 
{
    pthread_mutex_lock(&lock);

    MQ::rider_info_struct rider_info;
    if (MQ::read(MQ::create(RIDER_INFO_FRONT_SVKEY), rider_info, self_id, false) >= 0) {
        self_x = rider_info.rider_y;
        self_y = rider_info.rider_x;
        if (rider_info.event_type == 0) {
            //骑手到指定位置
            if (orders[0]->is_take) {
                erase_order(orders[0]->thread);
                orders.pop_front();
            } else {
                orders[0]->is_take = true;
            }
        } else if (rider_info.event_type == 1) {
            //有新订单
            get_order();
        }
        int next_order_id = cal_next_order();
        if (next_order_id != -1) {
            pthread_cond_signal(&orders[0]->cond);
            pthread_cond_wait(&cond, &lock);
        }
    }
    
    pthread_mutex_unlock(&lock);
}

void Rider::get_order() {
    MQ::info_desc_struct order_info;
    int msgqid = MQ::create(REST_TO_RIDER);
    std::vector<MQ::info_desc_struct> abandon;
    while (MQ::read(msgqid, order_info, 0, false) >= 0) {
        std::shared_ptr<Order> new_order = std::make_shared<Order>();
        new_order->order_id = order_info.order_id;
        new_order->user_x = order_info.user_get_x;
        new_order->user_y = order_info.user_get_y;
        new_order->user_id = order_info.user_id;
        new_order->rest_id = order_info.rest_id;
        new_order->rest_x = order_info.rest_get_x;
        new_order->rest_y = order_info.rest_get_y;
        new_order->required_time = order_info.required_time;
        new_order->done_time = order_info.done_time;
        if (is_accept_order(new_order)) {
            create_order(new_order);
        } else {
            abandon.emplace_back(order_info);
        }
    }
    for (const auto &ao : abandon) {
        MQ::write(MQ::create(REST_TO_RIDER), ao);
    }
    abandon.clear();
}

double Rider::H(const Order& order) 
{
    int dis = Graph::astar(self_x, self_y, order.user_x, order.user_y, Graph::graph);
    double disH = 1.0 / dis;
    double timeH = 1.0 / order.required_time;
    return 0.3 * disH + 0.7 * timeH;
}

void* Rider::deliver(void* arg) 
{
    ThreadArgs* args = static_cast<ThreadArgs*>(arg);
    Rider* rider = args->rider;
    std::shared_ptr<Order> order = args->order;
    pthread_mutex_lock(&(rider->lock));
    pthread_cond_signal(&(rider->cond));
    pthread_cond_wait(&(order->cond), &(rider->lock));

    int msqid = MQ::create(RIDER_INFO_BACK_SVKEY);
    MQ::rider_info_struct rider_info;
    if (order->is_take) {
        rider_info = {rider->self_id, 2, order->user_y, order->user_x};
        MQ::write(msqid, rider_info);
    } else {
        // if (*system_time >= order->done_time) {
        //     rider_info = {rider->self_id, 3, order->rest_x, order->rest_y};
        //     MQ::write(msqid, rider_info);
        //     order->is_take = true;
        // } else {
        //     rider->is_waiting = true;
        // }
        rider_info = {rider->self_id, 3, order->rest_y, order->rest_x};
        MQ::write(msqid, rider_info);
    } 
    
    pthread_cond_signal(&(rider->cond));
    pthread_mutex_unlock(&(rider->lock));
    return nullptr;
}

void Rider::sort_order() 
{
    for (const auto& order : orders) {
        order->weight = H(*order);
    }
    std::sort(orders.begin(), orders.end(), [](const auto& o1, const auto& o2) {
        return o1->weight > o2->weight;
    });
}

void Rider::create_order(std::shared_ptr<Order>& newOrder) 
{
    ThreadArgs args = {this, newOrder};
    pthread_create(&(newOrder->thread), nullptr, deliver, &args);
    pthread_cond_wait(&cond, &lock);
    orders.push_back(std::move(newOrder));
}

int Rider::cal_delivery_time(int x, int y, const std::shared_ptr<Order> &porder)
{
    if (porder->is_take)
        return Graph::astar(x, y, porder->user_x, porder->user_y, Graph::graph);
    int time_to_rest = Graph::astar(x, y, porder->rest_x, porder->rest_y, Graph::graph);
    time_to_rest += std::max(porder->done_time - (*system_time) - time_to_rest, 0);
    return time_to_rest + 
        Graph::astar(porder->rest_x, porder->rest_y, porder->user_x, porder->user_y, Graph::graph);
}

int Rider::cal_next_order()
{
    int orders_size = orders.size();
    if (orders_size == 0)
        return -1;
    if (orders_size == 1)
        return orders[0]->order_id;
    std::sort(orders.begin(), orders.end(), 
        [](std::shared_ptr<Order> porder1, std::shared_ptr<Order> porder2) {
            return porder1->required_time < porder2->required_time;
        });
    if (cal_delivery_time(self_x, self_y, orders[0]) + (*system_time) >= orders[0]->required_time) 
        return orders[0]->order_id;
    
    int transfer_order_id = orders[0]->order_id, ind;
    for (int i = 1; i < orders_size; ++i) {
        std::pair<int, int> transfer_pos;
        if (orders[i]->is_take)
            transfer_pos = {orders[i]->user_x, orders[i]->user_y};
        else
            transfer_pos = {orders[i]->rest_x, orders[i]->rest_y};
        int to_transfer_time = Graph::astar(self_x, self_y, transfer_pos.first, transfer_pos.second, Graph::graph);
        if (orders[i]->is_take)
            to_transfer_time += std::max(orders[i]->done_time - (*system_time) - to_transfer_time, 0);
        int sum_time = to_transfer_time + 
            cal_delivery_time(transfer_pos.first, transfer_pos.second, orders[0]);
        if (sum_time <= orders[0]->required_time) {
            transfer_order_id = orders[i]->order_id;
            ind = i;
            break;
        }
    }
    std::shared_ptr<Order> temp = std::make_shared<Order>(*orders[ind]);
    orders.erase(orders.begin() + ind);
    orders.push_front(temp);
    return transfer_order_id;
}

void Rider::erase_order(pthread_t tid)
{
    pthread_join(tid, NULL);
}