TOP_DIR = .
INC_DIR = $(TOP_DIR)/inc
SRC_DIR = $(TOP_DIR)/src
BUILD_DIR = $(TOP_DIR)/build
CUSTOM_WINDOW_SIZE=$(if $(INIT_WDS),$(INIT_WDS),50)

CC=gcc
FLAGS = -pthread -g -ggdb -DDEBUG -I$(INC_DIR) -DCUSTOM_WINDOW_SIZE=$(CUSTOM_WINDOW_SIZE)
OBJS = $(BUILD_DIR)/tju_packet.o \
			 $(BUILD_DIR)/kernel.o \
			 $(BUILD_DIR)/queue.o\
			 $(BUILD_DIR)/trace.o\
			 $(BUILD_DIR)/tran.o\
			 $(BUILD_DIR)/timer_helper.o\
			 $(BUILD_DIR)/tree.o\
			 $(BUILD_DIR)/tju_tcp.o \
			 # $(BUILD_DIR)/list.o\

default:all

all: clean server client

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c 
	$(CC) $(FLAGS) -c -o $@ $<

clean:
	-rm -f ./build/*.o client server
	-rm -f *.event.trace
	-rm -f *.log.log

server: $(OBJS)
	$(CC) $(FLAGS) ./src/server.c -o server $(OBJS)

client:
	$(CC) $(FLAGS) ./src/client.c -o client $(OBJS) 

test_tree: clean $(OBJS)
	$(CC) $(FLAGS) ./src/test_tree.c -o test_tree $(OBJS) 
	./test_tree
	

test: clean server client
	./test/test establish

debug: clean server
	clear
	./server



