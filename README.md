# 目录介绍
- doc下存放了需求文档
- src下的main.cpp就是项目代码
- test文件夹下的文件用于测试
- output文件夹存放项目输出文件，其中log.txt主要是守护进程输出，output.txt是项目所有使用cout输出的内容
# 注意事项
如果想要运行项目，首先要设置好自己的项目路径，位置在main.cpp的大约26行的位置：
```cpp
const std::string workdir = "/home/mqr/Workspace/os-des/";
```
要把这个地方改成自己的路径，才可以正常看到输出

# 运行及关闭
可以自己g++编译main.cpp
也可以在src文件夹下使用命令：`make run`。前提是安装了make

如果想终止项目运行，在Linux终端执行以下命令：
```console
pkill -f ./main
```

# 函数说明
## Graph
### astar
```cpp
int astar(int sx, int sy, int ex, int ey, bool isPath);
```
找最短路的函数，返回值为最短路长度

起点：(sx, sy)，终点：(ex, ey)，isPath为true是需要保存最短**路径**，为false就是不需要保存最短**路径**

使用示例：
```cpp
int dis = graph.astar(posx, posy, order.tarx, order.tary, false);
```

注意，如果设置了最后一个参数为true，那么最短路径会保存在graph.shortest_path中，访问示例：
```cpp
for (const auto& node : graph.shortest_path) {
    ... 
}
```

## random_int
```cpp
static int random_int(int l, int h);
```
作用：生成一个在区间`[l,h]`间的随机数

## print
由于项目是多线程、多进程环境，std::cout不安全，容易导致输出混乱，所以为了std::cout输出方便，写了print函数：
```cpp
#define print(msg) { \
    std::lock_guard<std::mutex> cout_lock(cout_mutex); \
    std::cout << msg; \
}
```
使用示例（其实跟用cout差不多）
```cpp
print("hello world" << message << std::endl);
```

使用c语言的printf也可以保证线程安全