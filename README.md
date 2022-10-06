# TJU TCP 

## 文件结构说明

* Structure: 
```markdown
.
├── report 	--> 报告的 Latex 模板等
├── code 	--> 执行代码
└── README.md 	--> 本Readme
```

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

* Report

```
report
├── appendix 				--> 附录文件
│   ├── acknowledgements.tex
│   ├── paperInChinese.tex
│   └── paperInEnglish.tex
├── body.aux
├── body.tex 				--> 正文部分
├── chapters 				--> 正文每个章节
│   ├── chapter1.tex
│   ├── chapter2.tex
│   ├── chapter3.tex
│   ├── chapter4.tex
│   └── chapter5.tex
├── clean.py 				--> 清理不必要的 build 文件
├── preface 				--> 封面文件
│   └── cover.tex
├── references 				--> 参考文献
│   ├── ref.buk
│   └── reference.bib
├── setup 				--> 全局 config
│   ├── format.tex
│   └── package.tex
├── tjumain.aux
├── tjumain.log
├── tjumain.out
├── tjumain.pdf
├── tjumain.synctex.gz
├── tjumain.tex 			--> root 文件
├── tjumain.thm
├── tjumain.toc
├── tjumain.xdv
├── TJU_report_template_master.pdf 	--> 示例 pdf 
├── 周报告模板.docx 		
├── 计网实践_第一周_程子姝_刘锦帆.pdf
└── 计网实践_第二周_程子姝_刘锦帆.pdf

5 directories, 29 files
```


