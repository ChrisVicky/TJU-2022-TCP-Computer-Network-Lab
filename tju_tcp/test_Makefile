BUILD_DIR = ../build
OBJS = $(BUILD_DIR)/tju_packet.o \
			 $(BUILD_DIR)/kernel.o \
			 $(BUILD_DIR)/queue.o\
			 $(BUILD_DIR)/trace.o\
			 $(BUILD_DIR)/tran.o\
			 $(BUILD_DIR)/timer_helper.o\
			 $(BUILD_DIR)/tree.o\
			 $(BUILD_DIR)/tju_tcp.o \
			 #    $(BUILD_DIR)/timer.o \
			 #    $(BUILD_DIR)/SKB.o \
			 #    $(BUILD_DIR)/debug.o 
			 # 		$(BUILD_DIR)/list.o\


default:all

# establish and rdt
# all: clean test_client test_server rdt_server rdt_client close_client 

# close
all: clean test_client test_server rdt_server rdt_client close_client close_server_for_server

clean:
	-rm -f test_client test_server rdt_server rdt_client close_client client.log server.log rdt_server.log rdt_client.log rdt_recv_file.txt

test_client:
	gcc -pthread -g -ggdb -DDEBUG -I../inc ./test_client.c -o test_client $(OBJS)

test_server:
	gcc -pthread -g -ggdb -DDEBUG -I../inc ./test_server.c -o test_server $(OBJS)

rdt_server:
	gcc -pthread -g -ggdb -DDEBUG -I../inc ./test_rdt_server.c -o rdt_server $(OBJS)
rdt_client:
	gcc -pthread -g -ggdb -DDEBUG -I../inc ./test_rdt_client.c -o rdt_client $(OBJS)

close_client:
	gcc -pthread -g -ggdb -DDEBUG -I../inc ./test_close_client.c -o close_client $(OBJS)

close_server_for_server:
	gcc -pthread -g -ggdb -DDEBUG -I../inc ./test_close_server_for_server.c -o close_server_for_server $(OBJS)
