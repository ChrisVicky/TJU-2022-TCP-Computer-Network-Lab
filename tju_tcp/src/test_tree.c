#include "../inc/global.h"
#include "../inc/debug.h"
#include "../inc/tree.h"
#include "../inc/queue.h"
FILE * test;

tju_tcp_t* new_sock(){
  tju_tcp_t *sock = (tju_tcp_t *) malloc(sizeof(tju_tcp_t));
  sock->state = CLOSED;

  sock->sending_queue = init_q();
  // sock->sending_buf = NULL;
  // sock->sending_len = 0;

  sock->received_buf = NULL;
  sock->received_len = 0;


  sock->window.wnd_send = (sender_window_t*)   malloc(sizeof(sender_window_t));
  sock->window.wnd_recv = (receiver_window_t*) malloc(sizeof(receiver_window_t));

  sock->window.wnd_send->rto = INIT_RTT;
  sock->window.wnd_send->estmated_rtt = INIT_RTT;
  sock->window.wnd_send->window_size = INIT_WINDOW_SIZE;
  sock->window.wnd_send->prev_ack_count = 0;
  sock->window.wnd_send->prev_ack = 0;

  sock->window.wnd_recv->buff_tree = init_tree(); 

  sock->half_queue = init_q();

  sock->full_queue = init_q();
  
  return sock;
}
int main(){
  _line_("TEST START");
  int a[11] = {3,9,16,21,24,29,35,37,46,47,0};
  tju_tcp_t * sock = new_sock();
  
  print_tree(sock->window.wnd_recv->buff_tree);

  _line_("Start Insertion Test");
  int seq = 0;
  for (int i=0;i<10;i++){
    seq = a[i];
    _debug_("Insert %d sock\n" ,seq);
    tju_packet_t* tmp = malloc(sizeof(tju_packet_t));
    insert_key_value(sock->window.wnd_recv->buff_tree, seq, tmp);
    print_tree(sock->window.wnd_recv->buff_tree);
  }
  _line_("Start Deletion");

    _debug_("Remove %d\n",29);
    tju_packet_t* ret = get_value(sock->window.wnd_recv->buff_tree,29);
  print_tree(sock->window.wnd_recv->buff_tree);
  int base = 100;
  for(int i=0;i<10;i++){
    int w = rand()%2;
    int key = a[i];
    if(w){
      key += base;
    }
    _debug_("Remove %d\n",key);
    tju_packet_t* ret = get_value(sock->window.wnd_recv->buff_tree, key);
    if(ret==NULL){
      _debug_("key: %d, pkt NOT Found\n", key);
    }else{
      _debug_("key: %d, pkt Found\n", key);
    }
    print_tree(sock->window.wnd_recv->buff_tree);
  }

  _line_("TEST END");

  char filename[] = "server.event.trace";
  test = fopen(filename, "w");
  fprintf(test, "TEST\n");
}




