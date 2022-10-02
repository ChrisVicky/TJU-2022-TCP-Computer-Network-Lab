TOP_DIR = .
INC_DIR = $(TOP_DIR)/inc
SRC_DIR = $(TOP_DIR)/src
BUILD_DIR = $(TOP_DIR)/build

CC=gcc
FLAGS = -pthread -g -ggdb -DDEBUG -I$(INC_DIR)
OBJS = $(BUILD_DIR)/tju_packet.o \
			 $(BUILD_DIR)/kernel.o \
			 $(BUILD_DIR)/list.o\
			 $(BUILD_DIR)/queue.o\
			 $(BUILD_DIR)/timer_helper.o\
			 $(BUILD_DIR)/trace.o\
			 $(BUILD_DIR)/tran.o\
			 $(BUILD_DIR)/tree.o\
			 $(BUILD_DIR)/tju_tcp.o \

default:all

all: clean server client

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c 
	$(CC) $(FLAGS) -c -o $@ $<

clean:
	-rm -f ./build/*.o client server
	-rm -f *.event.trace

server: $(OBJS)
	$(CC) $(FLAGS) ./src/server.c -o server $(OBJS)

client:
	$(CC) $(FLAGS) ./src/client.c -o client $(OBJS) 

test: clean server client
	./test/test establish

debug: clean server
	clear
	./server



