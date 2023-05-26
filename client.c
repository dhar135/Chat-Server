#include <sys/socket.h> //socket(), connect()
#include <stdlib.h>     //exit()
#include <unistd.h>     //send(), close, read(), write()
#include <arpa/inet.h>  //htons()
#include <string.h>     //memset()
#include <stdio.h>	//printf()
#include <stddef.h>
#include <pthread.h>


#define TEMP_PORT 12080
#define BUF_SIZE 1024

// Structure to hold the thread arguments
typedef struct {
    int server_fd;
} ThreadArgs;

//Function Declaration
void *receiveThread(void *arg);

// Thread function to receive and print messages from the server
void *receiveThread(void *arg) {
    ThreadArgs *threadArgs = (ThreadArgs *)arg;
    int server_fd = threadArgs->server_fd;
    ssize_t rcount;
    char readbuf[BUF_SIZE];

    while (1) {
        // Receive and print messages from the server
        rcount = read(server_fd, readbuf, BUF_SIZE - 1);

        // Check if the received message is valid
        if (rcount > 0) {
            readbuf[rcount] = '\0';  // Null-terminate the received message
            printf("%s\n", readbuf);
        } else {
            // Handle read error or server shutdown
            break;
        }
    }

    // Clean up and exit the thread
    close(server_fd);
    free(threadArgs);
    pthread_exit(NULL);
}

//MAIN: client
int main(void)
{
    // Initialize variables used throughout
    int server_fd, err;  
    ssize_t wcount;
    socklen_t addrlen;
    int port = TEMP_PORT;
    struct sockaddr_in addr;
    size_t len;
    char inputbuf[BUF_SIZE];
    char username[BUF_SIZE];
    ThreadArgs *threadArgs;
    pthread_t tid;

    // Prompt the user to enter a username
    printf("Enter your username: ");
    fgets(username, BUF_SIZE, stdin);

    // Remove newline character from the username
    len = strlen(username);
    if (len > 0 && username[len - 1] == '\n') {
        username[len - 1] = '\0';
    }
    


    // Create a new Internet domain stream socket (TCP/IP socket)
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Exit on socket() error
    if (server_fd == -1)
        exit(1);

    // Set addr struct to 0 bytes (to zero out fields we're not using)
    memset(&addr, 0, sizeof(struct sockaddr_in));

    // Set 3 addr structure members
    addr.sin_family = AF_INET;                // addr type is Internet (IPv4)
    addr.sin_addr.s_addr = INADDR_ANY;        // bind to any local port
    addr.sin_port = htons(port);              // convert port to network byte ordering

    // Store the structure size
    addrlen = sizeof(addr);

    // Attempt a connection to the server
    err = connect(server_fd, (struct sockaddr *)&addr, addrlen);

    // Close socket and exit on connect() error
    if (err == -1) {
        close(server_fd);
        exit(2);
    }

    // Send the username to the server
    wcount = write(server_fd, username, strlen(username));

    // Close socket and exit on write error
    if (wcount < 0) {
        close(server_fd);
        exit(3);
    }

    // Create thread arguments
    threadArgs = (ThreadArgs *)malloc(sizeof(ThreadArgs));
    threadArgs->server_fd = server_fd;

    // Create a thread for receiving messages from the server 
    pthread_create(&tid, NULL, receiveThread, threadArgs);

	


    // Loop to continuously send messages to the server
    while (1) {

	  

	fgets(inputbuf, BUF_SIZE, stdin);

       /*Prompt the user for input
	*Can't get this to work right. 	
	*
        printf("Enter a message: ");

	fflush(stdout);  // Flush the output buffer

  */


        // Send the message to the server
        wcount = write(server_fd, inputbuf, strlen(inputbuf));
        if (wcount < 0) {
            perror("write");
            break;
        }

        // Check if the user wants to quit
        if (inputbuf[0] == '\\' && inputbuf[1] == 'q') {
            printf("Closing connection. Goodbye!\n");
            break;
        }
        
	
    }

    // Close the client socket (server is still listening at this point)
    close(server_fd);

    // Wait for the receive thread to finish
    pthread_join(tid, NULL);

    free(threadArgs);

    exit(EXIT_SUCCESS);
}
