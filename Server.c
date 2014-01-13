#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "Socket.h"
#include "Constants.h"


/* variables to hold socket descriptors */
ServerSocket welcome_socket;
Socket connect_socket;

void childProcess(void);

int main(int argc, char* argv[]){


	/* pid_t is typedef for Linux process ID */ 
	pid_t spid, term_pid; 
	
	//childstatus
	int chld_status;

	//check if number of arguments provided is large enough
	if (argc < 2)
	{
		printf("No port specified\n");
		return (-1);
	}

/* create a "welcoming" socket at the specified port */
	welcome_socket = ServerSocket_new(atoi(argv[1]));
	if (welcome_socket < 0)
	{
		printf("Failed new server socket\n");
		return (-1);
	}
	
//loop forever to for daemon connections, we accept new connections and fork off cihld proccess
//that handle the rest of the server code
	while(1){
		//open up new connection whenever a connection comes in, else we wait for one
		connect_socket = ServerSocket_accept(welcome_socket);
		if (connect_socket < 0)
		{
			printf("Failed accept on server socket\n");
			exit (-1);
		}
		
		//fork off a child proces for each new connection, pretty much a copy of the server for each connection
		spid = fork();  
		if (spid == -1) 
		{
			perror("fork error"); 
			exit (-1);
		}
		if (spid == 0) 
        {/* code for the service process */
		//call child process, once done we close socket and exit child process
			childProcess();
		Socket_close(connect_socket);
		exit (0);
        } /* end service process */
     else /* daemon process closes its connect socket */
		{
			Socket_close(connect_socket);
         /* reap a zombie every time through the loop, avoid blocking*/
			term_pid = waitpid(-1, &chld_status, WNOHANG);
		}
	}
}

//Meat code of the server, run by the child process for each connection
void childProcess(void){
/* will not use the server socket */
	Socket_close(welcome_socket);

/* variable names for file "handles" */
	FILE *tmpFP;
	FILE *fp;
	
	//child PIDs and waiting
	pid_t cpid, term_pid;
	int chld_status;
	
	//id = server PID
	int id;
	
	//new_line is equivalent of input from client
	unsigned char new_line[MAX_LINE];
	
	//file name
	unsigned char tmp_name[MAX_NAMELEN];
	
	//string conversion of server PID
	unsigned char id_str[MAX_NAMELEN];
	
	//for execv
	char *parms_array[MAX_ARGS];  /* arrary of pointers to strings */
	
	//get server PID
	id = (int) getpid();
	//convert ID to string and concat with tmp
	sprintf(id_str, "%d", id);
	strcpy(tmp_name,"tmp");
	strcat(tmp_name, id_str);  
	
	

	//loop forever until we recieve EOF or error on input socket
	while(1){
		
		//i = loop index, c = char int value, rc = holds eof or erros
		int i, c, rc;
		
		//for loop to read in input/commands from client program, until we reach MAX_LINE size
		//or if we recieve EOF or error, than we exit entire program
		for (i = 0; i < MAX_LINE; i++)
		{
			c = Socket_getc(connect_socket);
			if (c == EOF)
			{
				remove(tmp_name); //delete tmp file created
				return; /* assume socket EOF ends service for this client */           
			}
			//if we read a null character, means end of line, so we break to process line
			if (c == '\0')
			{
				new_line[i] = c;
				break;
			}
			new_line[i] = c;
		}
		
		// we use strtok to tokenize the inputline and split the words up
		//pch holds each token
		char *pch;
		//i =loop index
		i = 0;
		//tokenize first item
		pch = strtok(new_line, " \n\t");
		//tokenize till we reach null
		while(pch != NULL){
			parms_array[i] = pch;
			pch = strtok(NULL, " \n\t");
			i++;
		}
		//add NULL to end of array
		parms_array[i] = NULL;

		
   //fork off a child for each command
		cpid = fork(); 
		if (cpid == -1) 
		{
	perror("fork"); /* perror creates description of OS error codes */
			remove(tmp_name);
			exit(0); 
		}
   //child processing code, just performs execvp command with params
		if (cpid == 0){
   //redirect stdout and stderr to the file to send over to client
			fp = freopen(tmp_name, "w", stdout);
			fp = freopen(tmp_name, "w", stderr);
			rc = execvp(parms_array[0], parms_array);
			if (rc == -1){
				perror("execvp");
				exit(-1); 
			}
    }  /* end of code that can be executed by the child process */
			else{
	/* code executed by the parent process */
	/* wait for the child to terminate and determine its exit status */
	/* this is necessary to prevent the accumulation of "Zombie" processes */
				
				term_pid = waitpid(cpid, &chld_status, 0);
				
				
				if (term_pid == -1) 
					perror("waitpid"); 
				else
				{
		fp = fopen(tmp_name, "a"); //append status to file
		if (WIFEXITED(chld_status)) {
			fprintf(fp,"PID %d exited, status = %d\n", cpid, WEXITSTATUS(chld_status));
			
		}
		else{
			fprintf(fp, "PID %d did not exit normally\n", cpid);
		}
	}
	
	fclose(fp); //close file and open as read
	if ((tmpFP = fopen (tmp_name, "r")) == NULL) {
		fprintf (stderr, "error opening tmp file\n");
		exit (-1);
	}
	
	//loop forever, sends contents of file, character by character over to the client
	//loop ends when we reach EOF, we close socket to signal to client that socket is done
	while (1) {
		c = fgetc(tmpFP);
		if(c == EOF){	
			Socket_close(connect_socket);
			break;
		}
		rc = Socket_putc(c, connect_socket);
	}
	
     /* delete the temporary file */
	remove(tmp_name);
     } /* end of code that can be executed by the parent process */
	 //loop back till we get EOF or error from client

   } //loop ends when we get EOF or error from client, so we return back to parent server process
   return;
}
