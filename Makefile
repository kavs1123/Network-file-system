all: client namingserver ss1

client: client.c
	gcc -g client.c -o client

namingserver: namingserver.c
	gcc -g namingserver.c -o namingserver -lpthread

ss1: ss1.c
	gcc -g ss1.c -o ss1 -lpthread

clean:
	rm -f client namingserver ss1
