#include "rider.h"
#include "graph.h"

Rider rider;

bool Rider::shouldAcOrd(std::shared_ptr<Order>& order) 
{
    bool res = random_int(0, 1) == 1 ? true : false;
    if(res){order->sent=true;}
    return res;
}

void Rider::manage() 
{
    // int msgqid = create_msgque(RIDER_GET_POS);
    // orderMsg msg;
    // read_msgque(msgqid, msg, MQSIZ, id, true);
    pthread_mutex_lock(&lock);
    getOrderFromResta();
    
    getDishFromResta();

    startToSend();
SORT_ORDERS:
    sort_order();
    while (orders.size() > 0) {
        pthread_cond_signal(&(orders.front()->cond));
        pthread_cond_wait(&cond, &lock);
        while (isNewOrder) {
            isNewOrder = false;
            goto SORT_ORDERS;
        }
        orders.pop_front();
    }
    pthread_mutex_unlock(&lock);
}

double Rider::H(const Order& order) 
{
    std::vector<std::pair<int, int>> sp;
    int dis = graph.astar(self_x, self_y, order.user_x, order.user_y, false, sp);
    double disH = 1.0 / dis;
    double timeH = 1.0 / order.required_time;
    return 0.3 * disH + 0.7 * timeH;
}

void Rider::getOrderFromResta() 
{
    int msgqid = create_msgque(SVKEY2);
    struct orderMsg msg;
    std::vector<orderMsg> abandon;
    while (read_msgque(msgqid, msg, ORDERSIZ, 0, false) >= 0) {
        std::shared_ptr<Order> temp = std::make_shared<Order>();
        temp->user_x = msg.tarx;
        temp->user_y = msg.tary;
        temp->user_id=msg.userId;
        temp->rest_id=msg.restaurantId;
        temp->rider_id=msg.riderID;
        if (shouldAcOrd(temp)) {    
            //create_order(temp);
            //toDoOrder.emplace_back(temp);
            msg.riderID=self_id;
            msg.msg_type=msg.restaurantId;
            msgqid = create_msgque(SVKEY3);
            write_msgque(msgqid,msg,ORDERSIZ);
            msgqid = create_msgque(SVKEY2);
            print("我是骑手" << self_id << "我接了饭店" << msg.restaurantId << "的订单，顾客位置："
                    << msg.tarx << ' ' << msg.tary << std::endl);
        } else {
            abandon.emplace_back(msg);
        }
    }
    for (int i=0; i < abandon.size();i++){
        write_msgque(msgqid,abandon[i],ORDERSIZ); // 放回队列，该单不接
    }
}

void Rider::getDishFromResta()
{
    int msgqid = create_msgque(SVKEY4);
    struct orderMsg msg;
    while (read_msgque(msgqid, msg, ORDERSIZ, self_id, false) >= 0){
        print("骑手" << self_id << "收到饭店" << msg.restaurantId << "取餐请求"<< std::endl);
        msg.msg_type=msg.restaurantId;
        if(sending){
            //如果正在送餐，就不去取
            write_msgque(msgqid, msg, ORDERSIZ);
            break;
        }
        sending=true;
        sleep(1); // 去取餐
        msgqid = create_msgque(SVKEY5);
        write_msgque(msgqid, msg, ORDERSIZ);
        msgqid = create_msgque(SVKEY4);
        print("骑手" << self_id << "已到达饭店(正在路上)" << msg.restaurantId << "取餐"<< std::endl);
    }
}

void Rider::startToSend() 
{
    int msgqid = create_msgque(SVKEY6);
    struct orderMsg msg;
    while (read_msgque(msgqid, msg, ORDERSIZ, self_id, false) >= 0) {
        print("骑手" << self_id << "开始运送 送到"<<msg.tarx<< " "<< msg.tary<<std::endl);
        std::shared_ptr<Order> temp = std::make_shared<Order>();
        temp->user_x = msg.tarx;
        temp->user_y = msg.tary;
        temp->user_id=msg.userId;
        temp->rest_id=msg.restaurantId;
        temp->rider_id=msg.riderID;
        create_order(temp);
    }
}

void* Rider::deliver(void* arg) 
{
    ThreadArgs* args = static_cast<ThreadArgs*>(arg);
    Rider* rider = args->rider;
    std::shared_ptr<Order> order = args->order;
    pthread_mutex_lock(&(rider->lock));
    // std::shared_ptr<Order> order(static_cast<Order*>(arg));
    pthread_cond_signal(&(rider->cond));
    pthread_cond_wait(&(order->cond), &(rider->lock));

FIND_MIN_PATH:        
    std::vector<std::pair<int, int>> sp;
    graph.astar(rider->self_x, rider->self_y, order->user_x, order->user_y, true, sp);
    print("骑手" << rider->self_id << "正在送单，路径：");
    for (const auto& node : sp) {
        while (check_newOrder()) {
            rider->isNewOrder = true;
            pthread_cond_signal(&(rider->cond));
            pthread_cond_wait(&(order->cond), &(rider->lock));
            goto FIND_MIN_PATH;
        }
        rider->isNewOrder = false;
        int deliver_time = 1000 / rider->speed;
        std::this_thread::sleep_for(std::chrono::milliseconds(deliver_time));
        rider->self_x = node.first;
        rider->self_y = node.second;
        print(rider->self_x << ',' << rider->self_y << ' ' << std::flush);
    }
    print(std::endl);

    int msgqid = create_msgque(SVKEY7);
    struct orderMsg msg;
    msg.msg_type=order->user_id;
    msg.riderID=order->rider_id;
    msg.restaurantId=order->rest_id;

    sleep(1); //运送
    print("骑手" << rider->self_id << "已送达"<<std::endl);
    rider->sending=false;
    write_msgque(msgqid, msg, ORDERSIZ);
    print(std::endl);

    std::stringstream info;
    info << "rider " << msg.userId << ' ' << msg.restaurantId;
    WriteFile(logPath, info.str(), true);

    // print("rider pos:(" << posx << ',' << posy << ") tar:(" << order->tarx << ',' << order->tary << ") path: ");
    // for (const auto& node : graph.shortest_path) {
    //     print('(' << node.first << ',' << node.second << ")->" << std::flush);
    //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // }
    // posx = order->tarx;
    // posy = order->tary;

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

int Rider::next_order()
{
    int orders_size = orders.size();
    if (orders_size == 0)
        return -1;
    if (orders_size == 1)
        return orders[0]->order_id;
    
    auto cal_delivery_time = [](int x, int y, const std::shared_ptr<Order> &porder) {
        if (porder->is_take) 
            return graph.astar(x, y, porder->user_x, porder->user_y);
        int time_to_rest = graph.astar(x, y, porder->rest_x, porder->rest_y);
        time_to_rest += std::max(porder->done_time - (*system_time) - time_to_rest, 0);
        return time_to_rest + 
            graph.astar(porder->rest_x, porder->rest_y, porder->user_x, porder->user_y);
    };

    if (cal_delivery_time(self_x, self_y, orders[0])) 
        return orders[0]->order_id;
    
}