#include <sys/socket.h> //socket(), bind(), listen(), accept()
#include <stdlib.h>     //exit()
#include <unistd.h>     //send(), close(), read(), write()
#include <arpa/inet.h>  //htons()
#include <string.h>     //memset()
#include <stdio.h>      //printf()
#include <pthread.h>    //pthread_create(), pthread_exit()
#include <stdbool.h>


#define TEMP_PORT 12080
#define BUF_SIZE 1024
#define MAX_CLIENTS 10
#define true 1
#define false 0


//Structure to hold the message
typedef struct Message{
        char content[BUF_SIZE];
        struct Message* next;
} Message;

typedef struct ClientNode{
        int clientSocket;
	char username[BUF_SIZE];
        struct ClientNode *next;
}ClientNode;

//Function declaration
void *clientThread(void *arg);
void enqueueMessage(const char *content);
Message *dequeueMessage(void);
int isUsernameTaken(const char* username);
void addClient(int clientSocket, const char* username);
void removeClient(int clientSocket); 






//Global variables
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;      //Mutex for synchronization
pthread_cond_t cond = PTHREAD_COND_INITIALIZER; //Condition variable for signaling
Message *messageQueue = NULL;   //Shared message queue
ClientNode *clientList = NULL;



//MAIN: server
int main (void)
{
        int listen_fd, client_fd, err, opt;
        socklen_t addrlen;
        int port = TEMP_PORT;
        struct sockaddr_in addr;

        pthread_t tid;


        //Create a new Internet domain stream socket (TCP/IP socket)
        listen_fd = socket(AF_INET, SOCK_STREAM, 0);
 	//Exit on socket() error
        if (listen_fd == -1) {
                fprintf(stderr, "\nsocket(): exiting\n");
                exit(1);
        }


        //Set socket opt (don't really know what is does)
        //This option lets the server quickly rebind to a partly closed port
        opt = 1;
        err = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        //Set addr struct to 0 bytes (to zero out fields we're not using)
        memset(&addr, 0, sizeof(struct sockaddr_in));

        //Set 3 addr structure members
        addr.sin_family = AF_INET;              //addr type is Internet (IPv4)
        addr.sin_addr.s_addr = INADDR_ANY;      //bind to any local address
        addr.sin_port = htons(port);            //convert port to network byte ordering

        //Store the structure size
        addrlen = sizeof(addr);

        //Bind socket to TEMP_PORT(12080)
        err = bind(listen_fd, (struct sockaddr *)&addr, addrlen);

        //Exit on bind() error
        if (err == -1) {
                close(listen_fd);
                fprintf(stderr, "\nbind(): exiting\n");
                exit(2);
        }

        //Begin listening for connections with a max backlog of 1
        //(backlog is very low to demonstrate dropped connection attempts)
        err = listen(listen_fd, 32);

        //Exit on listen() error
        if (err == -1) {
                close(listen_fd);
                fprintf(stderr, "\nlisten(): exiting\n");
                exit(3);
        }

        printf("\nServer started. Listening for connections...\n\n");
	 //Initialize mutex and condition variable
        pthread_mutex_init(&mutex, NULL);

        //Accept clients connection on a loop
        while (1){

                //Accept one client connection
                client_fd = accept(listen_fd, NULL, NULL);

                //Create a new thread for the client
                pthread_create(&tid, NULL, clientThread, &client_fd);

                //Detach the thread
                pthread_detach(tid);
                }

        //Cleanup and exit
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
        close(listen_fd);
        exit(EXIT_SUCCESS);
}




//Thread Function
//takes a single argument, which is a pointer to the client file descriptor (int type). //This allows us to pass the client file descriptor to the thread function when creating//the thread.

void *clientThread(void *arg) {
    int client_fd = *((int *)arg);
    ssize_t rcount;
    char readbuf[BUF_SIZE * 2];
    Message *message;
    ClientNode *current;
    const char *msg;
    ssize_t wcount;
    ClientNode *client;
    char greeting[BUF_SIZE * 2];
    char username[BUF_SIZE];  // Buffer to store the received username

    // Receive the username from the client
    rcount = read(client_fd, username, BUF_SIZE);

    if (rcount <= 0) {
        close(client_fd);
        fprintf(stderr, "\nread(); exiting\n");
        pthread_exit(NULL);
    }
   

    // Create a new client node
    client = (ClientNode *)malloc(sizeof(ClientNode));
    client->clientSocket = client_fd;
    strncpy(client->username, username, BUF_SIZE);
    client->next = NULL;


     // Check if the username is already in use
    if (isUsernameTaken(username)) {
        snprintf(readbuf, sizeof(readbuf), "Username '%s' is already in use. Please choose a different username.", username);
        wcount = write(client_fd, readbuf, strlen(readbuf));
        if (wcount < 0) {
            // Handle write error

        }
        close(client_fd);
        removeClient(client_fd);
        pthread_exit(NULL);
    }


    // Add the new client to the clientList
    pthread_mutex_lock(&mutex);
    if (clientList == NULL) {
        clientList = client;
    } else {
        current = clientList;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = client;
    }
    pthread_mutex_unlock(&mutex);

    // Send a welcome message to the client immediately after connecting
    msg = "Server says: Connection established!\n";
    wcount = write(client_fd, msg, strlen(msg));

    // Exit on send error
    if (wcount < (int)strlen(msg)) {
        close(client_fd);
        fprintf(stderr, "\nwrite(); exiting\n");
        pthread_exit(NULL);
    }

    // Send a greeting message to the client including their username
    
    snprintf(greeting, sizeof(greeting), "Welcome, %.*s!\n", (int)(sizeof(greeting) - 1), username);
    wcount = write(client_fd, greeting, strlen(greeting));

    // Exit on send error
    if (wcount < (int)strlen(greeting)) {
        close(client_fd);
        fprintf(stderr, "\nwrite(); exiting\n");
        pthread_exit(NULL);
    }

    while (1) {
        // Check to see if the client has sent any message
        rcount = read(client_fd, readbuf, BUF_SIZE - 1);

        // Close socket and exit on read error or EOF
        if (rcount <= 0) {
            close(client_fd);
            fprintf(stderr, "\nread(); exiting\n");
            pthread_exit(NULL);
        }

	 // Null-terminate the message string
        readbuf[rcount - 1] = '\0';

        // Enqueue the received message
        enqueueMessage(readbuf);

        // Dequeue the next message to be sent to clients
        message = dequeueMessage();

        // Print the received message on the server side
        printf("%s: %s\n", username,  message->content);

	// Iterate over the list of connected clients and send the message
	current = clientList;
	while (current != NULL) {
    		char messageToSend[BUF_SIZE * 4];
    		snprintf(messageToSend, sizeof(messageToSend), "%s: %s", username, message->content);
    		wcount = write(current->clientSocket, messageToSend, strlen(messageToSend));

    	if (wcount < (int)strlen(messageToSend)) {
        	// Error occurred while sending, handle it appropriately
        	fprintf(stderr, "\nwrite(); exiting\n");
        	close(client_fd);
        	pthread_exit(NULL);
    	}

    		current = current->next;
	}
        // Free the memory allocated for the dequeued message
        free(message);
    }

    // Clean up and exit the thread
    close(client_fd);

    // Remove the client from the clientList
    pthread_mutex_lock(&mutex);
    if (clientList == client) {
        clientList = clientList->next;
    } else {
        current = clientList;

        while (current != NULL && current->next != client)
	{
			current = current->next;
	}
	if (current != NULL) {
		current->next = client->next;
	}
	}

	free(client);
	pthread_mutex_unlock(&mutex);

return NULL;
}

		
//Function to enqueue a message to the share message queue
void enqueueMessage(const char *content)
{
        //Create a new message
        Message *message = (Message *)malloc(sizeof(Message));
        strcpy(message->content, content);
        message->next = NULL;

        //Acquire the mutex lock before modifying the message queue
        pthread_mutex_lock(&mutex);

        //Add the message to the end of the queue
        if (messageQueue == NULL)
        {
                messageQueue = message;
        }

        else
        {
                //Find the last message in the queue
                Message *current = messageQueue;
                while(current->next != NULL)
                {
                        current = current->next;
                }

                //Append the new message to the end of the queue
                current->next = message;
        }

        //Release the mutex lock after modifying the message queue
        pthread_mutex_unlock(&mutex);
}

//Function to dequeue a message from the shared message queue
Message *dequeueMessage(void)
{

        //Declare the variable at the beginning of the function
        Message *message;

        //Acquire the mutex lock before accessing the message queue
 	pthread_mutex_lock(&mutex);

        //If the message queue is empty, wait for a new message
        while(messageQueue == NULL){
                pthread_cond_wait(&cond, &mutex);
        }


        //Retrieve the first message from the queue

        message = messageQueue;
        messageQueue = messageQueue->next;

        //Release the mutex lock after accessing the message queue
        pthread_mutex_unlock(&mutex);

        return message;
}


// Function to check if a username is already in use
int isUsernameTaken(const char* username) {
    ClientNode* current = clientList;
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            return 1; // Username is already in use
        }
        current = current->next;
    }
    return 0; // Username is available
}

// Function to add a client to the linked list
void addClient(int clientSocket, const char* username) {
    ClientNode* newNode = (ClientNode*)malloc(sizeof(ClientNode));
    newNode->clientSocket = clientSocket;
    strncpy(newNode->username, username, BUF_SIZE);
    newNode->next = clientList;
    clientList = newNode;
}

// Function to remove a client from the linked list
void removeClient(int clientSocket) {
    ClientNode* current = clientList;
    ClientNode* previous = NULL;
    while (current != NULL) {
        if (current->clientSocket == clientSocket) {
            if (previous != NULL) {
                previous->next = current->next;
            } else {
                clientList = current->next;
            }
            free(current);
            break;
        }
        previous = current;
        current = current->next;
    }
}


