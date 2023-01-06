
* Code
```
code
├── rdt_send_file.txt 	--> Client 端使用的发送文本
├── tags 		--> ctag 生成
├── test_Makefile 	--> 用于替换 test 时的 Makefile
├── Makefile 		--> 手动测试时的 Makefile 
├── inc 		--> .h 文件
│   ├── kernel.h 	--> tcp kernel，不需要更改
│   ├── queue.h 	--> 全连接队列和发送队列（void* data）
│   ├── tju_packet.h 	--> 不动
│   ├── tju_tcp.h 	--> 主要函数，各种 handler 和 用法
│   ├── trace.h 	--> trace 文件生成
│   ├── tran.h 		--> 发送/重传函数
│   ├── tree.h 		--> avl 数据结构，接收乱序缓冲区
│   ├── timer_helper.h 	--> timer 主函数
│   ├── global.h 	--> 全局宏等
│   └── debug.h 	--> debug 信息宏
├── src                         │
│   ├── queue.c                 │ 
│   ├── test_tree.c             │ debug
│   ├── tju_packet.c            │ 输出到文件时
│   ├── tran.c                  │ 句柄在此
│   ├── timer_helper.c          │
│   ├── tree.c                  │
│   ├── tju_tcp.c               │
│   ├── trace.c                 │
│   ├── kernel.c ◄──────────────┘
│   ├── client.c
│   └── server.c
├── client.log.log 	--> Client 输出的 debug
├── rdt_recv_file.txt 	--> Server 端生成，接收到的文本
├── server.log.log 	--> Server 输出的 debug 
└── build 		--> .o 文件生成位置

3 directories, 31 files
```
