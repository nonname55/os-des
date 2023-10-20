#include "restaurant.h"
#include "process.h"

Restaurant restaurant;

void Restaurant::manage() 
{
    print("restaurant exe" << std::endl);
    auto self_pos = Graph::get_valid_pos(G_REST);
    self_x = self_pos.first.first;
    self_y = self_pos.first.second;
    self_get_x = self_pos.second.first;
    self_get_y = self_pos.second.second;
    int msgqid = MQ::create(USER_TO_REST);
    MQ::info_desc_struct order_msg;
    while (MQ::read(msgqid, order_msg, self_id, false) >= 0) {
        int required_time = random_int(*system_time, (*system_time) + 500);
        int done_time = random_int(*system_time, (*system_time) + 100);
        Order new_order(order_msg.user_x, order_msg.user_y, required_time, done_time);
        print("这里是饭店 " << self_id << " 接收到了用户 " 
                << order_msg.user_id << " 的订单，位于 " 
                << order_msg.user_x << ' ' << order_msg.user_y << std::endl);
        order_msg.rest_x = self_x;
        order_msg.rest_y = self_y;
        order_msg.rest_get_x = self_get_x;
        order_msg.rest_get_y = self_get_y;
        order_msg.required_time = required_time;
        order_msg.done_time = done_time;
        // MQ::write(MQ::create(REST_TO_RIDER), order_msg);
    }
}