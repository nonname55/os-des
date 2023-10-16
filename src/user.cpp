#include "user.h"

User user;

void User::manage() {
    struct orderMsg msg;
    if (isOrder()) {
        int msgqid = create_msgque(SVKEY1);
        msg.msg_type = random_int(1, RESTAURANT_NUM);
        msg.tarx = posx;
        msg.tary = posy;
        msg.userId = id;
        write_msgque(msgqid, msg, MQSIZ);
        print("我是用户 " << id << " 我订了饭店 " << msg.msg_type << " 的外卖，我在 "
                << posx << ' ' << posy << std::endl);
    }
    int msgqid = create_msgque(SVKEY7);
    while(read_msgque(msgqid, msg, MQSIZ, id)>=0){
        print("我是用户 "<<id<<" 已收到来自商家 "<<msg.restaurantId<<" 骑手 "<< msg.riderID<<" 送的"<<
        "商品"<<std::endl);
    }
}

bool User::isOrder() {
    cnt++;
    if(cnt==10){
        cnt=0;
        return true;
    }
    else return false;
}