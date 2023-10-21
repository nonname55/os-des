#include "rider.h"
#include "graph.h"

Rider rider;

bool Rider::is_accept_order() 
{
    if (self_id == RIDER_NUM)
        return true;
    bool res = random_int(0, 1) == 1;
    return res;
}

void Rider::manage() 
{
    pthread_mutex_lock(&lock);
    std::stringstream info;
    info << "rider " << self_id << ' ' << orders.size();
    // for (const auto &porder : orders) {
    //     info << porder->user_id << ' ' << ' ' << porder->rest_id << ' ' << porder->order_id << ' '
    //         << porder->user_x << ' ' << porder->user_y << ' ' << porder->rest_x << ' ' << porder->rest_y << ' '
    //         << porder->required_time << ' ' << porder->done_time << ' ' << porder->is_take;
    // }
    WriteFile(logPath, info.str(), true);

    MQ::rider_info_struct rider_info;
    int msqid = MQ::create(RIDER_INFO_FRONT_SVKEY);
    print("骑手" << self_id << "创建消息队列成功, msqid=" << msqid << std::endl);
    int ret = MQ::read(msqid, rider_info, self_id, false);
    print("骑手" << self_id << "成功读取到消息，返回值" << ret << std::endl);
    if (ret >= 0) {
        print("进入到ret>=0的判断" << std::endl);
        self_x = rider_info.rider_y;
        self_y = rider_info.rider_x;
        print("成功给骑手坐标赋值" << self_x << ' ' << self_y << ' ' << rider_info.event_type << std::endl);
        if (rider_info.event_type == 0) {
            print("骑手" << self_id << "有" << orders.size() << "个订单" << std::endl);
            if (orders.size() > 0) {
                if (orders[0]->is_take) {
                    print("骑手" << self_id << "已送达" << std::endl);
                    orders.front().reset();
                    orders.pop_front();
                    print("骑手" << self_id << "已经删除送达了的订单" << std::endl);
                } else {
                    orders[0]->is_take = true;
                    print("骑手" << self_id << "已经取到餐" << std::endl);
                }
                if (cal_next_order() != -1) {
                    pthread_cond_signal(&(orders[0]->cond));
                    pthread_cond_wait(&cond, &lock);
                }
            }
        } else if (rider_info.event_type == 1) {
            print("骑手" << self_id << "接收到了前端发来的有新订单的信息" << std::endl);
            get_order();
            if (cal_next_order() != -1) {
                pthread_cond_signal(&(orders[0]->cond));
                pthread_cond_wait(&cond, &lock);
            }
        }
    } else {
        print("骑手没有接收到任何消息" << std::endl);
    }
    
    pthread_mutex_unlock(&lock);
}

void Rider::get_order() {
    MQ::info_desc_struct order_info;
    int msgqid = MQ::create(REST_TO_RIDER);
    std::vector<MQ::info_desc_struct> abandon;
    print("骑手开始读取商店订单" << std::endl);
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
        print("读取到订单" << new_order->order_id << std::endl);
        if (is_accept_order()) {
            print("骑手" << self_id << "接单" << new_order->order_id << std::endl);
            new_order->cond =  PTHREAD_COND_INITIALIZER;
            create_order(new_order);
            // orders.emplace_back(new_order);
        } else {
            print("不接这个订单" << std::endl);
            abandon.emplace_back(order_info);
        }
    }
    print("骑手" << self_id << "读取完毕" << std::endl);
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
    int msqid = MQ::create(RIDER_INFO_BACK_SVKEY);
    MQ::rider_info_struct rider_info;
    auto send_msg = [&](std::shared_ptr<Order> porder) {
        int next_x, next_y, msg_type;
        if (porder->is_take) {
            next_x = porder->user_y;
            next_y = porder->user_x;
            msg_type = 2;
        } else {
            next_x = porder->rest_y;
            next_y = porder->rest_x;
            msg_type = 3;
        }
        rider_info = {rider->self_id, msg_type, next_x, next_y};
        MQ::write(msqid, rider_info);
    };
    print("骑手" << rider->self_id << "已经决定好下一步取餐 " << order->order_id << std::endl);
    while (true) {
        pthread_cond_signal(&(rider->cond));
        pthread_cond_wait(&(order->cond), &(rider->lock));
        send_msg(order);
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
    print("试图创建订单" << std::endl);
    pthread_create(&(newOrder->thread), nullptr, deliver, &args);
    pthread_cond_wait(&cond, &lock);
    print("创建订单成功" << std::endl);
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
    print("开始决定下一个订单" << std::endl);
    int orders_size = orders.size();
    if (orders_size == 0)
        return -1;
    if (orders_size == 1) {
        return 0;
    }
        
    std::sort(orders.begin(), orders.end(), 
        [](std::shared_ptr<Order> porder1, std::shared_ptr<Order> porder2) {
            return porder1->required_time < porder2->required_time;
        });
    if (cal_delivery_time(self_x, self_y, orders[0]) + (*system_time) >= orders[0]->required_time) {
        return 0;
    }
    print("试图寻找中转订单" << std::endl);
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
            auto it = std::next(orders.begin(), i);
            std::rotate(orders.begin(), it, orders.end());
            break;
        }
    }
    return 0;
}

void Rider::erase_order(pthread_t tid)
{
    pthread_join(tid, NULL);
}