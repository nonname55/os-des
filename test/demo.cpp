#include <iostream>
#include <sys/ipc.h>
#include <sys/msg.h>  
#include <sys/types.h>
#include <unistd.h>

using namespace std;

// 消息结构体
struct msg_form {
  long msg_type;
  char text[100];
};

int main() {

  // 创建消息队列
  key_t key = ftok("/tmp", 'a');
  int qid = msgget(key, 0666 | IPC_CREAT);

  pid_t pid1 = fork();
  if (pid1 == 0) {
    // 子进程1发送消息
    msg_form send_msg;
    send_msg.msg_type = 1;
    cout << "Enter message to send: ";
    cin.getline(send_msg.text, 100);
    msgsnd(qid, &send_msg, sizeof(send_msg), 0);
  }

  pid_t pid2 = fork();
  if (pid2 == 0) {
    // 子进程2接收消息
    msg_form recv_msg;
    msgrcv(qid, &recv_msg, sizeof(recv_msg), 1, 0);
    cout << "Received: " << recv_msg.text << endl;
  }

  // 等待子进程
  wait(NULL);
  wait(NULL);

  // 删除消息队列
  msgctl(qid, IPC_RMID, NULL);

  return 0;
}