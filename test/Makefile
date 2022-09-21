BUILD_DIR = ../build
OBJS = $(BUILD_DIR)/tju_packet.o \
	   $(BUILD_DIR)/kernel.o \
	   $(BUILD_DIR)/tju_tcp.o \
	#    $(BUILD_DIR)/timer.o \
	#    $(BUILD_DIR)/SKB.o \
	#    $(BUILD_DIR)/debug.o 


default:all

all: clean test_client test_server rdt_server rdt_client close_client

clean:
	-rm -f test_client test_server rdt_server rdt_client close_client client.log server.log rdt_server.log rdt_client.log

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
