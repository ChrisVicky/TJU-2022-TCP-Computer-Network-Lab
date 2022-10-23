code
├── rdt\_send\_file.txt --> Client 端使用的发送文本
├── tags 		--> ctag 生成
├── test\_Makefile 	--> 用于替换 test 时的 Makefile
├── Makefile 		--> 手动测试时的 Makefile 
├── inc 		--> .h 文件
│   ├── kernel.h 	--> tcp kernel，不需要修改
│   ├── queue.h 	--> 队列数据结构，void \*，兼容多种场景
│   ├── tju\_packet.h 	--> packet处理，不需要修改
│   ├── tju\_tcp.h 	--> 主要函数，各种 handler 和 用法 	==> 接收线程 tju\_handle\_packet
│   ├── trace.h 	--> trace 文件生成
│   ├── tran.h 		--> 发送/重传函数 			==> 发送线程 check\_timer 以及 send\_with\_retransmit
│   ├── tree.h 		--> avl 数据结构，乱序缓冲区
│   ├── timer\_helper.h --> timer 主函数
│   ├── global.h 	--> 全局宏等
│   └── debug.h 	--> debug 信息宏
├── src                         │
│   ├── queue.c                 │ 
│   ├── test\_tree.c            │ debug
│   ├── tju\_packet.c           │ 输出到文件时
│   ├── tran.c                  │ 句柄在此
│   ├── timer\_helper.c         │
│   ├── tree.c                  │
│   ├── tju\_tcp.c              │
│   ├── trace.c                 │
│   ├── kernel.c ◄──────────────┘
│   ├── client.c 						--> Client 端主线程
│   └── server.c 						--> Server 端主线程
├── client.log.log 	--> Client 输出的 debug
├── rdt\_recv\_file.txt --> Server 端生成，接收到的文本
├── server.log.log 	--> Server 输出的 debug 
└── build 		--> .o 文件生成位置

3 directories, 31 files




