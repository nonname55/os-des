# 目录介绍
- src下是所有函数、变量的定义，其中main.cpp是项目的入口文件（main函数所在文件）
- doc下存放了需求文档（一些尚未解决的问题）
- test文件夹下的文件用于测试
- output文件夹存放项目输出文件，其中log.txt主要是守护进程输出，output.txt是项目所有使用cout输出的内容
- include下是项目头文件，是所有模块的变量、函数声明
- bin下是项目生成的可执行文件

# .h文件和.cpp文件
.h文件中主要是函数、变量的声明 .cpp文件中是函数、变量的定义

如果想修改或者新建函数、定义，.h文件和.cpp文件中都要添加、修改

# 运行及关闭
由于项目比较庞大，所以需要借助makefile进行编译。使用`sudo apt install make`安装make，然后将目录切换为项目路径（~/os-des/）下，执行`make`。经过一段时间的等待（等待时间可能比较长），控制台所有提示信息被清空，此时项目已经开始运行

编译+运行：
```
~/Workspace/os-des
➞  make
```

如果想终止项目运行，在Linux终端执行以下命令：
```console
pkill -9 -f ./main
```

项目所有使用print打印出的内容都会在output文件夹下的output.txt显示出来

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