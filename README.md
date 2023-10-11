# 目录介绍
- doc下存放了需求文档（一些尚未解决的问题）
- src下的main.cpp就是项目代码
- test文件夹下的文件用于测试
- output文件夹存放项目输出文件，其中log.txt主要是守护进程输出，output.txt是项目所有使用cout输出的内容
# 注意事项
如果想要运行项目，首先要设置好自己的项目路径，位置在main.cpp的大约26行的位置：
```cpp
const std::string workdir = "/home/mqr/Workspace/os-des/";
```
要把这个地方改成自己的项目路径，才可以正常看到输出

# 运行及关闭
1. 常规方式，g++编译运行。但生成的文件名最好是main
2. 使用make工具，Ubuntu中可以通过`sudo apt install make`安装make，然后进入src目录下，执行`make run`命令。

如果想终止项目运行，在Linux终端执行以下命令：
```console
pkill -f ./main
```

# 部分功能性函数使用说明
## print
代码：
```cpp
#define print(msg) { \
    std::lock_guard<std::mutex> cout_lock(cout_mutex); \
    std::cout << msg; \
}
```
功能：
```
线程安全的cout
```
使用示例：
```cpp
int x = 0, y = 1;
print("hello world" << x << ' ' << y << std::endl);
```
## Graph
注意，使用Graph结构体中的任何函数都要用`graph.xxx`调用
### getValidPos
功能：
```
在地图上获取一个合法的随机位置
```
代码：
```cpp
std::pair<int, int> getValidPos() {
    int x, y;
    do {
        x = random_int(0, G_ROW - 1);
        y = random_int(0, G_COL - 1);
    } while (bupt_map[x][y] == POS_ERROR);
    return {x, y};
}
```
使用示例：
```cpp
struct Point {
    int x, y;
} point;

auto pos = graph.getValidPos();
point.x = pos.first;
point.y = pos.second;
```
### astar
原型：
```cpp
int astar(int sx, int sy, int ex, int ey, bool isPath)
```
功能：
```
获得最短路，获得最短路径，返回值为最短路的值
如果最后一个参数isPath为true，就是需要保存最短路径（会保存在graph.shortest_path中），反之不需要
为了项目性能考虑，如果只需要知道最短路有多长，isPath就设为false
```
使用示例：
```cpp
//不需要路径
int dis = graph.astar(0, 0, 8, 9, false);
//需要路径
int dis = graph.astar(0, 0, 8, 9, true);
for (const auto& node : graph.shortest_path) {
    ... 
}
```
## random_int
原型：
```cpp
int random_int(int l, int h);
```
功能：
```
生成一个在区间[l, h]的随机数
```
使用示例：
```cpp
int x = random_int(1, 10);
```