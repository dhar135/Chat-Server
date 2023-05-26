Chat Server
=============

Description
------------
A simple “chat" server (implementing a basic chat room that allows multiple clients to communicate as a group via messages written at the terminal).

Table of Contents
-----------------
- Installation
- Usage
- Bugs
- Contributing
- Acknowledgements


Installation
------------
To get started, you must have at least three terminals running, one for server and two for at least two clients. 

Usage
-----
On the command line type, "server" to get the server started. On the two other terminals, type, "client" to allow the clients to connect to the local server. As a client, you will first be prompted to enter a username. You can enter any name, that is not already taken. After that, a greeting from the server will be sent to the client with their name. From then on as a client you will be able to enter a message to be sent to the server and to other clients. The messages will be displayed on each terminal. 

Bugs
----
I think I successfully got to step 7 of your instructions with many bugs along the way that I ma
y not know of. 
Some I do know include:

-The enter a message prompt is supposed to be there before I enter a message but I cant seem to get it to work right because of buffering or me not reading or writing to the threads correctly.

-When entering a username of the same name, I have to ^C to get out and rejoin as another client.   


Contributing
------------
Specify guidelines and instructions for contributing to your project, if applicable. Include details on how others can submit bug reports, feature requests, or code contributions.


Acknowledgments
---------------
Prof. Trammel's Starter code
The internet (lol)





