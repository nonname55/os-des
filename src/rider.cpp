#include "rider.h"
#include "graph.h"

Rider rider;

bool Rider::shouldAcOrd(std::shared_ptr<Order>& order) {
    bool res = random_int(0, 1) == 1 ? true : false;
    if(res){order->sent=true;}
    return res;
}
void Rider::manage() {
    pthread_mutex_lock(&lock);
    getOrderFromResta();
    
    getDishFromResta();

    startToSend();
SORT_ORDERS:
    sort_order();
    // print("rider pos:(" << posx << ',' << posy << ")" << std::endl);
    // print("sort order: " << std::endl);
    // for (int i = 0; i < 5; ++i) {
    //     print("(" << orders[i]->tarx << ',' << orders[i]->tary << ") required time: " << orders[i]->required_time << std::endl);
    // }
    // print(std::endl);
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
double Rider::H(const Order& order) {
    int dis = graph.astar(posx, posy, order.tarx, order.tary, false);
    double disH = 1.0 / dis;
    double timeH = 1.0 / order.required_time;
    return 0.3 * disH + 0.7 * timeH;
};
void Rider::getOrderFromResta() {
    int msgqid = create_msgque(SVKEY2);
    struct orderMsg msg;
    std::vector<orderMsg> abandon;
    while (read_msgque(msgqid, msg, MQSIZ, 0) >= 0) {
        std::shared_ptr<Order> temp = std::make_shared<Order>();
        temp->tarx = msg.tarx;
        temp->tary = msg.tary;
        temp->userId=msg.userId;
        temp->restaurantId=msg.restaurantId;
        temp->riderId=msg.riderID;
        if (shouldAcOrd(temp)) {    
            //create_order(temp);
            //toDoOrder.emplace_back(temp);
            msg.riderID=id;
            msg.msg_type=msg.restaurantId;
            msgqid = create_msgque(SVKEY3);
            write_msgque(msgqid,msg,MQSIZ);
            msgqid = create_msgque(SVKEY2);
            print("我是骑手" << id << "我接了饭店" << msg.restaurantId << "的订单，顾客位置："
                    << msg.tarx << ' ' << msg.tary << std::endl);
        } else {
            abandon.emplace_back(msg);
        }
    }
    for (int i=0; i<abandon.size();i++){
        write_msgque(msgqid,abandon[i],MQSIZ); // 放回队列，该单不接
    }
}
void Rider::getDishFromResta(){
    int msgqid = create_msgque(SVKEY4);
    struct orderMsg msg;
    while (read_msgque(msgqid, msg, MQSIZ, id) >= 0){
        print("骑手" << id << "收到饭店" << msg.restaurantId << "取餐请求"<< std::endl);
        msg.msg_type=msg.restaurantId;
        if(sending){
            //如果正在送餐，就不去取
            write_msgque(msgqid, msg, MQSIZ);
            break;
        }
        sending=true;
        sleep(1); // 去取餐
        msgqid = create_msgque(SVKEY5);
        write_msgque(msgqid, msg, MQSIZ);
        msgqid = create_msgque(SVKEY4);
        print("骑手" << id << "已到达饭店(正在路上)" << msg.restaurantId << "取餐"<< std::endl);
    }
}
void Rider::startToSend() {
    int msgqid = create_msgque(SVKEY6);
    struct orderMsg msg;
    while (read_msgque(msgqid, msg, MQSIZ, id) >= 0) {
        print("骑手" << id << "开始运送 送到"<<msg.tarx<< " "<< msg.tary<<std::endl);
        std::shared_ptr<Order> temp = std::make_shared<Order>();
        temp->tarx = msg.tarx;
        temp->tary = msg.tary;
        temp->userId=msg.userId;
        temp->restaurantId=msg.restaurantId;
        temp->riderId=msg.riderID;
        create_order(temp);
    }
}
void* Rider::deliver(void* arg) {
    ThreadArgs* args = static_cast<ThreadArgs*>(arg);
    Rider* rider = args->rider;
    std::shared_ptr<Order> order = args->order;
    pthread_mutex_lock(&(rider->lock));
    // std::shared_ptr<Order> order(static_cast<Order*>(arg));
    pthread_cond_signal(&(rider->cond));
    pthread_cond_wait(&(order->cond), &(rider->lock));

FIND_MIN_PATH:        
    graph.astar(rider->posx, rider->posy, order->tarx, order->tary, true);
    print("骑手" << rider->id << "正在送单，路径：");
    for (const auto& node : graph.shortest_path) {
        while (check_newOrder()) {
            rider->isNewOrder = true;
            pthread_cond_signal(&(rider->cond));
            pthread_cond_wait(&(order->cond), &(rider->lock));
            goto FIND_MIN_PATH;
        }
        rider->isNewOrder = false;
        int deliver_time = 1000 / rider->speed;
        std::this_thread::sleep_for(std::chrono::milliseconds(deliver_time));
        rider->posx = node.first;
        rider->posy = node.second;
        print(rider->posx << ',' << rider->posy << ' ' << std::flush);
    }
    print(std::endl);

    int msgqid = create_msgque(SVKEY7);
    struct orderMsg msg;
    msg.msg_type=order->userId;
    msg.riderID=order->riderId;
    msg.restaurantId=order->restaurantId;

    sleep(1); //运送
    print("骑手" << rider->id << "已送达"<<std::endl);
    rider->sending=false;
    write_msgque(msgqid, msg, MQSIZ);
    print(std::endl);

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