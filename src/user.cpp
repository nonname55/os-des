#include "user.h"
#include "process.h"

User user;

void User::manage() 
{
    struct orderMsg msg;
    if (isOrder()) {
        int msgqid = create_msgque(SVKEY1);
        int rest = random_int(1, RESTAURANT_NUM);
        msg.msg_type = rest;
        msg.tarx = posx;
        msg.tary = posy;
        msg.userId = id;
        msg.orderId = *order_count;
        ++(*order_count);
        write_msgque(msgqid, msg, ORDERSIZ);

        print("我是用户 " << id << " 我订了饭店 " << msg.msg_type << " 的外卖，我在 "
                << posx << ' ' << posy << std::endl);
        
        std::stringstream info;
        info << "user " << id << ' ' << posx << ' ' << posy;
        WriteFile(logPath, info.str(), true);
        //告诉前端用户产生订单的信息
        // msgqid = create_msgque(SVKEY8);
        // msg = {posx, posy, id, 0, 0, rest, -1};
        // write_msgque(msgqid, msg, ORDERSIZ);
    }
    int msgqid = create_msgque(SVKEY7);
    while(read_msgque(msgqid, msg, ORDERSIZ, id, false)>=0){
        print("我是用户 "<<id<<" 已收到来自商家 "<<msg.restaurantId<<" 骑手 "<< msg.riderID<<" 送的"<<
        "商品"<<std::endl);
    }
}

bool User::isOrder() 
{
    cnt++;
    if(cnt==10){
        cnt=0;
        return true;
    }
    else return false;
}