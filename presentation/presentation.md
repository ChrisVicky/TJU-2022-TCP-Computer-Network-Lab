---
title: 计网实践大报告
author: 36组 程子姝 刘锦帆 
date: 2022-10-22
extensions:
  - image_ueberzug
  - render
  - terminal
  - file_loader

---
# 任务完成情况总结

```
┌─────────┬────────────────────────┬──────────────────────────────┬────────────────────────────────────────────────┐
│ Mission │ Contents               │ Detail                       │ How                                            │
├─────────┼────────────────────────┼──────────────────────────────┼────────────────────────────────────────────────┤
│ 1       │ Establishment          │ #1 3 hand-shakes             │ tju_conn(), tju_handle_pkt(), tju_accept()     │
│         │                        │ #2 With Loss Rate            │ Same as RDT                                    │
│---------│------------------------│------------------------------│------------------------------------------------│┌────────────┬┬───────────────┐
│ 2       │ Reliable Data Transfer │ #1 Sender Buffer             │ buffer_lock, tju_send(), send thread, Timer    ││ $timer_list││ $sending_queue│
│         │                        │ #2 Recv Buffer               │ buffer_lock, tju_recv(), recv thread, AVL Tree │└────────────┴┴───────────────┘
│         │                        │ #3 Pkt Loss Detection        │ No ACK, Retransmission(Timer), Dynamic RTO     │
│         │                        │ #4 Retransmission            │ Timer_List Structure, ACK                      │
│         │                        │ #5 Dynamic RTO               │ update rto when receiving specific ACK         │
│         │                        │ #6 ACK Accumulation          │ ACK Strategy & Timer Destruction Strategy      │
│         │                        │ #7 Fast Retransmission       │ ack Redundancy                                 │
│         │                        │ #8 Capability                │ Test Results                                   │
│---------│------------------------│------------------------------│------------------------------------------------│
│ 3       │ Flow Control           │ #1 RWND & Advertised         │ handle_pkt(), ack                              │
│         │                        │ #2 SWND & Advertised         │ SWND update, ack                               │
│         │                        │ #3 0 RWND                    │ empty pkt                                      │
│---------│------------------------│------------------------------│------------------------------------------------│
│ 4       │ Close                  │ #1 4 Wavehands               │ tju_close(), socket state                      │
│         │                        │ #2 Close one by one          │ Covers                                         │
│         │                        │ #3 Close Simutaneously       │ Covers                                         │
│         │                        │ #4 With Loss Rate            │ RDT                                            │
│---------│------------------------│------------------------------│------------------------------------------------│
│ 5       │ Efficiency             │ #1 Window Size vs Throughput │ Tested by Scripts (Buffer Unlimited)           │
│         │                        │ #2 Loss Rate vs Throughput   │ Tested by Scripts (Buffer Limited)             │
└─────────┴────────────────────────┴──────────────────────────────┴────────────────────────────────────────────────┘
```
![13](img/image_2022-10-22-13-05-25.png)

---

# ESTABLISHMENT - 3 HAND-SHAKES

![45](img/image_2022-10-22-13-09-13.png)

---

![48](img/image_2022-10-22-13-11-48.png)

---

# RDT - SENDER BUFFER

1. Send Queue  ==> tju_send() , Send Thread
2. Timer List  ==> Send Thread, Recv Thread

## Client 端流程图
```
                                                                                                  ┌─┐                              
                                               创建 SYN 包并发送(需要ack)                         │T│ destroytimer()
                                   tju_conn()├──────────────────────────────┐ tju_close()         │I│◄───────────────────────┐     
                                        ▲      等待 state = ESTABLISHED     │     ▲               │M│ send_with_retransmit() │
 ┌─────────┐                            │                                   │     │  checktimer() │E│◄─────────────────────┐ │
 │ #CLIENT │──────┬──────────┬──────────┴───────────────────┬─────────────────────┘  ┌───────────►│R│                    发│ │     
 │ THREAD  │      │          │                              ▼       创建 pkt│放入队列│   push_q() ├─┤                    送│ │
 └─────────┘      │          │           ┌────────┐      tju_send()├────────│────────│───────────►│S│                    报│ │     
                  │          ▼           │ #SEND  │                         │        │  get_pkt() │E│                    文│ │     
                  │    tju_socket()├────►│ TRHEAD │─────────────────────────│────────┴────────────┤N├──────────────────────┘ │
                  │          │           └────────┘                         │            push_q() │D│                        │
                  │          │                                      滑动窗口└────────────────────►│ │                        │
                  │          │                            ┌─────────────┴───────────────┐         │Q│                        │
                  │          │                              已发未确认       尚未发送             │U│                        │
                  │          │        ┌──────────────┐    ┌─────┴──────┐┌───────┴───────┐         │E│                        │
                  │          │      ┌► $sending_queue ─┐  ┌────────────┬┬───────────────┐         │U│                        │
                  │          │      │ ├──────────────┤ ├─►│ $timer_list││ $sending_queue│         │E│                        │
                  │          ▼      ├► $timer_list    ─┘  └────────────┴┴───────────────┘         └─┘                        │
                  │    $tju_socket ─┤ ├──────────────┴───────────────────────┐                                               │
                  │                 ├► $disorder_buff ───► AVL TREE: 乱序到达                                                │
                  │                 │ ├──────────────────────────────────────┤                                               │
                  │                 └► $recv_buff     ───► BUFFER: 正序数据                                                  │
                  │                   └──────────────────────────────────────┘                                               │
                  │               ┌────────┐                                                                                 │
                  ▼               │ #RECV  │                                                                                 │
          startSmulation()├──────►│ THREAD │─────┤handle_packet()├─┬┤ack├────────────────────────────────────────────────────┘
                                  └────────┘                       │        
                                                                   └┤SYN & ACK├─►传回ACK，当前sock->state = ESTABLISHED
```
                           
## 3 threads
  * Main Thread(**#CLIENT THREAD**)
    * #RECV THREAD : 接收报文，交给 tju_handle_pkt() 处理
    * #SEND THREAD : 发送报文，创建 timer_list 处理重传

---

# RDT - RECV BUFFER

1. Recv buffer ==> tju_recv(), char\*
2. Disordered buffer ==> AVL Tree

## Server 端流程图
```
                                               ┌─────────────────────┐
                                             ┌► $half queue ─► 半连接 ◄───────────────────────────────────────────┐                
                                $tju_socket──┤ ├─────────────────────┤                                            │
                                    ▲        └► $full queue ─► 全连接 ◄──────────────────────────────────────────┐│
                                    │          └─────────────────────┘    ┌┤ACK & SYN_RECV├─► state= ESTABLISHED├┘│
                                    │              ▲    │                 │                                       │
                                    │              │    │                 ├┤SYN├─►传回SYN & ACK, state=SYN_RECV├──┘
                                    │              │    │                 │                                    
                                    │              │    │                 │                       ┌─┐ remove()                    
                                    │              │    ▼                 │               insert()│A│────────────┐       
                                    │           tju_accept()              │      ┌┤FUTURE├───────►│V│ find_next()│                 
                                tju_socket()         ▲                    │      │                │L│◄─────────┐ │         
                                    ▲     ┌────────┐ │                    │      │                └─┘          │ │                 
                                    │     │ #RECV  │ │                    │      │          push()┌─┐          │ │                 
                  startSmulation()├─│────►│ THREAD │─│──┤handle_packet()├─┴┤data├┼┤EXPECTED├─────►│R│──────────┘ │                 
                       ▲            │     └────────┘ │                           │                │E│ push()     │
  ┌─────────┐          │            │                │                           └┤OLD├─►drop()   │C│◄───────────┘
  │ #SERVER │──────────┴────────────┴────────────────┴───────┬─────────────┐                      │V│             
  │ THREAD  │                                                ▼           从│buffer 中顺序读取     │ │             
  └─────────┘                                             tju_recv() ◄─────│──────────────────────┤B│             
                                                                           ▼                      │U│             
                                                                      tju_close()                 │F│             
                                                                                                  │F│             
                                                                                                  └─┘             
```

## AVL Tree

1. Disordered Input, Ordered Ouput
2. Usage: Graph

---

# RDT - Pkt Loss Detection

1. 

## Client 端流程图
```
                                                                                                  ┌─┐                              
                                               创建 SYN 包并发送(需要ack)                         │T│ destroytimer()
                                   tju_conn()├──────────────────────────────┐ tju_close()         │I│◄───────────────────────┐     
                                        ▲      等待 state = ESTABLISHED     │     ▲               │M│ send_with_retransmit() │
 ┌─────────┐                            │                                   │     │  checktimer() │E│◄─────────────────────┐ │
 │ #CLIENT │──────┬──────────┬──────────┴───────────────────┬─────────────────────┘  ┌───────────►│R│                    发│ │     
 │ THREAD  │      │          │                              ▼       创建 pkt│放入队列│   push_q() ├─┤                    送│ │
 └─────────┘      │          │           ┌────────┐      tju_send()├────────│────────│───────────►│S│                    报│ │     
                  │          ▼           │ #SEND  │                         │        │  get_pkt() │E│                    文│ │     
                  │    tju_socket()├────►│ TRHEAD │─────────────────────────│────────┴────────────┤N├──────────────────────┘ │
                  │          │           └────────┘                         │            push_q() │D│                        │
                  │          │                                      滑动窗口└────────────────────►│ │                        │
                  │          │                            ┌─────────────┴───────────────┐         │Q│                        │
                  │          │                              已发未确认       尚未发送             │U│                        │
                  │          │        ┌──────────────┐    ┌─────┴──────┐┌───────┴───────┐         │E│                        │
                  │          │      ┌► $sending_queue ─┐  ┌────────────┬┬───────────────┐         │U│                        │
                  │          │      │ ├──────────────┤ ├─►│ $timer_list││ $sending_queue│         │E│                        │
                  │          ▼      ├► $timer_list    ─┘  └────────────┴┴───────────────┘         └─┘                        │
                  │    $tju_socket ─┤ ├──────────────┴───────────────────────┐                                               │
                  │                 ├► $disorder_buff ───► AVL TREE: 乱序到达                                                │
                  │                 │ ├──────────────────────────────────────┤                                               │
                  │                 └► $recv_buff     ───► BUFFER: 正序数据                                                  │
                  │                   └──────────────────────────────────────┘                                               │
                  │               ┌────────┐                                                                                 │
                  ▼               │ #RECV  │                                                                                 │
          startSmulation()├──────►│ THREAD │─────┤handle_packet()├─┬┤ack├────────────────────────────────────────────────────┘
                                  └────────┘                       │        
                                                                   └┤SYN & ACK├─►传回ACK，当前sock->state = ESTABLISHED
```



---






## 项目结构图

![35](img/image_2022-10-22-11-59-25.png)


---

# Data Structure

## Queue
1. First in First out ==> Queue Structure
2. Support Multiple Data Type ==> void\*
3. Serialised ==> Lock

## Timer_List
1. Destroy timer with unique **id** ==> Cannot use Queue Structure 
2. Time out detection ==> timer\_event
3. Serialised ==> Lock
4. Asynchronous Destruction ==> Queue to buffer the process

## AVL Tree
1. Disordered Input & Ordered Output ==> AVL Tree
2. Search Tree According to Seq Number ==> Key-Value

---


## Queue 
1. First in First out ==> Queue Structure
2. Support Multiple Data Type ==> void\*
3. Serialised ==> Lock

```cpp
#define MAX_QUEUE_SIZE 2*INIT_WINDOW_SIZE
typedef struct myNode{
  void *data;             // --> support for multiple Data Type
  struct myNode *next;
}myNode;
typedef struct myQueue{
  int size;
  struct myNode *head;
  struct myNode *tail;
  pthread_mutex_t q_lock; // --> Serialised
}myQueue;
myQueue* init_q();
int push_q(myQueue* q, void *data);
void *pop_q(myQueue* q);
int is_empty_q(myQueue *q);
int is_full_q(myQueue* q);
```

---

## Timer List

1. Destroy timer with unique **id** ==> Cannot use Queue Structure 
2. Time out detection ==> timer\_event
3. Serialised ==> Lock
4. Asynchronous Destruction ==> Queue to buffer the process

```cpp
typedef struct timer_event{
  struct timespec * create_at;
  struct timespec * timeout_at;
  tju_packet_t * args;                // Retransmited Pkt
}timer_event;
typedef struct timer_node{
  struct timer_event *event;
  struct timer_node *next;
  int id;
}timer_node;
typedef struct timer_list{
  timer_node* head;
  timer_node* tail;
  int size;
  int total;
  pthread_mutex_t lock;
  myQueue *queue;                     // Asynchronous Destruction
}timer_list;
struct timer_list* init_timer_list();
uint32_t set_timer(struct timer_list *list, uint32_t sec, uint64_t nano_sec, void *(*callback)(void *, void*), void *args);
int check_timer(tju_tcp_t * sock);
int destroy_timer(tju_tcp_t* sock, uint32_t id);
int get_list_size(struct timer_list*);
void print_timers(timer_list* list);
```

---

## AVL Tree

1. Disordered Input & Ordered Output ==> AVL Tree
2. Search Tree According to Seq Number ==> Key-Value

```cpp
typedef struct treeNode{
  int key;
  void *value;
  struct treeNode*left;
  struct treeNode*right;
  int height;
}treeNode;
typedef struct myTree{
  struct treeNode *root;
  uint32_t size;
}myTree;
tju_packet_t* get_value(struct myTree *root, int key);
void free_tree(myTree* root);
void insert_key_value(myTree *node, int key, void *value);
void print_tree(myTree *root);
struct myTree* init_tree();
```


---

---

# 测试图片显示

![35](img/image_2022-10-22-11-55-46.png)

---

# 代码测试

```cpp
#include <bits/stdc++.h>
int main(){
    cout<<"Hello World"<<endl;
  }

```

---

## 三次握手 - Server 端(1)
```file
path: ./netProj/tju_tcp/src/tju_tcp.c # required
relative: true         # relative to the slide source directory
lang: c                 # pygments language to render in the code block
lines:
  start: 296
  end: 315
```
## 三次握手 - Server 端(2)

```file
path: ./netProj/tju_tcp/src/tju_tcp.c
lang: c
lines:
  start: 330
  end: 345
```

---

## 三次握手 - Client 端(1)
```file
path: ./netProj/tju_tcp/src/tju_tcp.c # required
relative: true         # relative to the slide source directory
lang: c                 # pygments language to render in the code block
lines:
  start: 315
  end: 330
```

