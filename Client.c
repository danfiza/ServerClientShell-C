#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "Socket.h"
#include "Constants.h"

/*
Danyal Fiza
*/


int main(int argc, char* argv[]) {
//i = loop index, c = char int value, rc = holds eof or erros
//count holds length of input lines read from fgets
	int i, c, rc;
	int count = 0;

//Input array to hold each line read from fgets and socket, sufficently large
	char input[MAX_LINE]; 

/* variable to hold socket descriptor */
	Socket connect_socket;

//Check if sufficent amount of arguments for client
	if (argc < 3)
	{
		printf("No host and port\n");
		return (-1);
	}

/* connect to the server at the specified host and port;
* blocks the process until the connection is accepted
* by the server; creates a new data transfer socket.
*/
connect_socket = Socket_new(argv[1], atoi(argv[2]));
if (connect_socket < 0)
{
	printf("Failed to connect to server\n");
	return (-1);
}

//Default asthetic output for input
printf("shell%% "); 

//loops all code for each command entered until EOF is reached
while((fgets(input, MAX_LINE, stdin) != NULL)) {

//count + 1 to know how many characters + '\0' to send over socket
	count = strlen(input) + 1; 

//for loop to send line read from fgets stored in input over socket, ends once we reach \0 or socket error
	for (i = 0; i < count; i++)
	{
		c = input[i];
		rc = Socket_putc(c, connect_socket); //Get either 1 or EOF on error or EOF
		if (rc == EOF)
		{
			printf("Socket_putc EOF or error\n");             
			Socket_close(connect_socket);
        exit (-1);  /* assume socket problem is fatal, end client */
		}
	}
	
	
	//infinite loop to read input from server socket, ends at EOF or error, print each character recieved
//blocks on getc till character is sent over
	while(1)
	{
		c = Socket_getc(connect_socket);
		if(c == EOF)
			break;
		printf("%c", c);
		
	}
//because we stop loop at EOF, connection disconnects, so we have to connect back to the server again
	connect_socket = Socket_new(argv[1], atoi(argv[2]));

//print astethtic output again and loop again to get input till EOF or error is reached
	printf("shell%% "); 
}
/* end of while loop; at EOF */
Socket_close(connect_socket);
exit(0);
}