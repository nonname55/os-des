#include "restaurant.h"

Restaurant restaurant;

void Restaurant::manage() {
    int msgqid = create_msgque(SVKEY1);
    struct orderMsg msg;
    while (read_msgque(msgqid, msg, MQSIZ, id) >= 0) {
        Order newOrder(msg.tarx, msg.tary, random_int(1, 10));
        orders.emplace_back(newOrder);
        print("这里是饭店 " << id << " 接收到了用户 " 
                << msg.userId << " 的订单，位于 " << msg.tarx << ' ' << msg.tary << std::endl);
    }
    msgqid = create_msgque(SVKEY2);
    for (int i = 0; i < orders.size(); ++i) {
        msg.msg_type = 1;
        msg.tarx = orders[i].tarx;
        msg.tary = orders[i].tary;
        msg.restaurantId = id;
        write_msgque(msgqid, msg, MQSIZ);
    }

    msgqid = create_msgque(SVKEY3);
    while (read_msgque(msgqid, msg, MQSIZ, id) >= 0) {
        //confOrdersQue.emplace_back(msg);
        print("这里是饭店 " << id << " 接收到了骑手 " << msg.riderID 
        << " 的接单消息" << " 该单号需要送到 "<<msg.tarx << ' ' << msg.tary << std::endl);
        sleep(1);//做饭
        msg.msg_type=msg.riderID;
        msgqid = create_msgque(SVKEY4);
        write_msgque(msgqid, msg, MQSIZ);
        msgqid = create_msgque(SVKEY3);
        print("饭店 " << id << " 做餐完毕，向骑手 " << msg.riderID 
            << " 发送取餐请求" << " 该单号需要送到 "<<msg.tarx << ' ' << msg.tary << std::endl);
    }
    
    msgqid = create_msgque(SVKEY5);
    while (read_msgque(msgqid, msg, MQSIZ, id) >= 0) {
        if(haveArrived()){
            msg.msg_type=msg.riderID;
            msgqid = create_msgque(SVKEY6);
            write_msgque(msgqid, msg, MQSIZ);
            msgqid = create_msgque(SVKEY5);
            print("饭店 "<< id <<"已经将餐交给骑手 "<<msg.riderID<<std::endl);
        }
        else {
            unArrived.emplace_back(msg);
        }
    }
    for(int i=0;i<unArrived.size();i++) {
        write_msgque(msgqid,unArrived[i],MQSIZ); // 放回队列，骑手还没到
    }
    unArrived.clear();
    
}