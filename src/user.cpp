#include "user.h"
#include "process.h"

User user;

void User::manage() 
{
    MQ::info_desc_struct order_msg;
    if (is_order()) {
        ++(*order_count);
        int rest_id = random_int(1, RESTAURANT_NUM);
        auto self_pos = Graph::get_valid_pos(G_USER);
        self_x = self_pos.first.first;
        self_y = self_pos.first.second;
        self_get_x = self_pos.second.first;
        self_get_y = self_pos.second.second;
        //给饭店发消息
        order_msg = {rest_id, *order_count, self_x, self_y, self_id, self_get_x, self_get_y,
            -1, -1, rest_id, -1, -1, -1, -1, -1};
        MQ::write(MQ::create(USER_TO_REST), order_msg);
        // print("用户 " << self_id << " 点了饭店 " << rest_id << " 的外卖，我在" << self_x << ' ' << self_y << std::endl);
        //发送到前端
        order_msg = {1, *order_count, self_y, self_x, self_id, self_get_y, self_get_x,
            -1, -1, rest_id, -1, -1, -1, -1, -1};
        MQ::write(MQ::create(INFO_DESC_SVKEY), order_msg);
    }
}

bool User::is_order() {
    int ri = random_int(1, 3);
    return ri == 1;
}