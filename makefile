#Make a separate client and server program
#Notice a target doesn't require any commands. The fact that it has dependencies
#ensures those dependent targets are made if they're out of date

program : client server

#Target to make client
client: client.o
	gcc `./pflags` -o client client.o -pthread 

#Target to make server
server: server.o
	gcc `./pflags` -o server server.o -pthread

#Client object dependencies
client.o : client.c
	gcc `./pflags` -c client.c -pthread

#Server object dependencies
server.o : server.c
	gcc `./pflags` -c server.c -pthread

clean :
	@rm -f client.o server.o client server -pthread
