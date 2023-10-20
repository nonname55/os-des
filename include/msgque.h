#ifndef MSGQUE_H
#define MSGQUE_H

#include "macro.h"
#include "header.h"

namespace MQ {
    /*  
        type == 1 : 新订单
        type == 2 : 饭店发布订单
        type == 3 : 饭店做好饭
        type == 4 : 骑手接单
    */
    #define INFO_DESC_SVKEY 75 //后端向前端发送事件描述
    #define USER_TO_REST    76
    #define REST_TO_RIDER   77
    
    struct info_desc_struct {
        long type;
        int order_id;
        int user_x, user_y, user_id, user_get_x, user_get_y;
        int rest_x, rest_y, rest_id, rest_get_x, rest_get_y;
        int rider_id;
        int required_time, done_time;
    } __attribute__((packed));
    /*
        type 代表骑手id
        event_type == 0 : 骑手到达指定位置 （前端->后端）
        event_type == 1 : 有新订单产生    （前端->后端）

        后端->前端 骑手下一步位置 type是骑手id
    */
    #define RIDER_INFO_FRONT_SVKEY 81 //前端发信息
    #define RIDER_INFO_BACK_SVKEY 82  //后端发信息
    struct rider_info_struct {
        long type;
        int event_type;
        int rider_x, rider_y;
    } __attribute__((packed));

    int create(int svkey);

    template <typename T>
    void write(int msgqid, T &msg) 
    {
        int msgsiz = sizeof(msg) - sizeof(long);
        int ret = msgsnd(msgqid , &msg, msgsiz, 0);
        if (ret == -1) {
            print(getpid() << " write msg que error at msgqid " << msgqid << std::endl);
            exit(-1);
        }
    }

    template <typename T>
    int read(int msgqid, T &msg, int msgtype, bool is_wait) 
    {
        int msgsiz = sizeof(msg) - sizeof(long);
        int msg_flag = is_wait ? 0 : IPC_NOWAIT;
        int ret = msgrcv(msgqid, &msg, msgsiz, msgtype, msg_flag);
        return ret;
    }

    void delete_mq(int svkey);

    bool hasMessage(int svkey);
}

#endif