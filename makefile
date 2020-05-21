CC=gcc
CFLAGS=-I.

all: server client client_eval server_dbg client_dbg
release: server client client_eval
debug: server_dbg client_dbg

server: server.c server.h common.h myQueue.c myQueue.h
	$(CC) $(CFLAGS) -g -o server server.c myQueue.c -lpthread
client: client.c common.h
	$(CC) $(CFLAGS) -DDBG -g -o client client.c 

client_eval: client.c common.h
	$(CC) $(CFLAGS) -DDBG -DEVAL_MODE -g -o client_eval client.c 

server_dbg: server.c server.h common.h myQueue.c myQueue.h
	$(CC) $(CFLAGS) -DDBG -g -o server_dbg server.c myQueue.c -lpthread

client_dbg: client.c common.h
	$(CC) $(CFLAGS) -g -DDBG -o client_dbg client.c    

clean:
	rm -r server client client_eval server_dbg client_dbg 
