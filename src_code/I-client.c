#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<time.h>
#include <netinet/tcp.h>
#include<signal.h>

#define PORT "5001" // the port client will be connecting to

int connector = -1;// this socket descriptor will be used to connect to server.

void sigint_handler(int sig)
{
	fprintf(stderr, "Client interrupted..\n");
	if(connector > 1)
	{
	 send(connector, "/quit", strlen("/quit") + 1, 0); //Giving the servers proper messages for all clients
	}
	exit(EXIT_FAILURE);
}
/*............................................................................*/




//...............................................this function will return exact form of sockaddr, IPv4 or IPv6..........................
void *convert(struct sockaddr *addr)
{
	if (addr->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *)addr)->sin_addr);
	}
	return &(((struct sockaddr_in6 *)addr)->sin6_addr);
}
//.......................................................................................................................................

int main(int argc, char const *argv[])
{
	if (argc != 2)
	{
		perror("you are entering wrong number of string\n\n");
		exit(EXIT_FAILURE);
	}
	char buffer[4096], temp[4096]; // it will be used for buffer in read(), write()
	ssize_t no_byte; // return value of read() and write()
	ssize_t read_byte; // return value of getline()
	char *line = NULL;
	size_t len = 0;
	FILE *fp;
	
	struct addrinfo hints, *ref, *p; // there are for getaddrinfo()
	int status; // for return value of getaddrinfo()
	pid_t pid; // this is for return value of fork()

	char server_IP[INET6_ADDRSTRLEN]; // inet_ntop save ip address of server into it.

	char *token1, *token2, *token3 , *next, *str;// for strtok_r()

	struct sigaction for_sigint; // This is the struct, we have to pass to sigaction()

	//get us a socket and connect it.
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if( (status = getaddrinfo(argv[1], PORT, &hints, &ref)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		exit(EXIT_FAILURE);
	}

	for (p = ref; p != NULL; p = p->ai_next)
	{
		if ((connector = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("socket: error\n");
			continue;
		}

		if (connect(connector, p->ai_addr, p->ai_addrlen) == -1)
		{
			perror("connect:error\n");
			close(connector);
			continue;
		}
		break;/* we already got here successful connector and successfully connecting to server.
				so we do not need to traverse remaining list anymore.*/
	}// end of the for loop for traversing the return list of getaddrinfo()
	
	//if we do not get any successful socket or successful connection
	if(p == NULL)
	{
		fprintf(stderr, "client: failed at getting successful socket or successful connection 1\n" );
		exit(EXIT_FAILURE);
	}

	inet_ntop(p->ai_family, convert((struct sockaddr *)p->ai_addr), server_IP, sizeof server_IP);
	printf("client: connecting to %s\n", server_IP);

	freeaddrinfo(ref);// i am done with ref;




	/*.....................................Here we are handling SIGINT signal............................*/
	for_sigint.sa_handler = sigint_handler;
	sigemptyset(&for_sigint.sa_mask);
	for_sigint.sa_flags = 0;


	if (sigaction(SIGINT, &for_sigint, NULL) < 0)
	{
	  	perror("SERVER(sigaction): ");
	  	exit(EXIT_FAILURE);
	} 
	/*............................................................................................*/

	

	if( (pid =fork()) == 0)// main loop
	{
		for(;;)
		{
			memset(&buffer, 0, sizeof buffer);// I am setting all bytes of buffer to 0 at begining of every run of loop
			no_byte = read(connector, buffer, 4096);
			
			if (no_byte  <= 0)
			{
				if (no_byte == 0)
 				{
 					printf("CLIENT: Connection with SERVER is closed  \n");
 					kill(getppid(),SIGKILL);
 					close(connector);
 					exit(EXIT_FAILURE);

 				}
 				else
 				{
 					perror("CLIENT: ");
 					close(connector);
 					exit(EXIT_FAILURE);
 				}

			}//..............end of recv() if block..............................
			else if(strncmp(buffer, "/sendfile", 9)==0 && buffer[9] ==' ')
			{
				memset(temp, 0, 4096);
				strncpy(temp, buffer, 4096);
				next = temp;
				token1 = strtok_r(next, " ", &next);
				token2 = strtok_r(next, " ", &next);
				token3 = strtok_r(next, " ", &next);
				//printf("bug_point2 %s %s %s %s\n",token1, token2, token3, next );

				fp=fopen(token3 ,"ab");
				if (fp==NULL)
				{
					printf("file is empty\n");
					exit(EXIT_FAILURE);
				}
				printf("file is downloading from %s\n", token2);
				fputs(next, fp);
				
				fclose(fp);
			}
			else
			  {printf( "%s\n", buffer);}//..............end of recv() else block..............................

		}//............................end of reading for loop....................	
		
	}//............................................end of child process else loop..................................
	else
	{
		for(;;)
		{
			memset(&buffer, 0, sizeof buffer);// I am setting all bytes of buffer to 0 at begining of every run of loop
			//printf("CLIENT: ");
			fgets(buffer,4096, stdin);
			if (strlen(buffer) > 4096)
			{
				printf("you are overflowing my buffer. Try again with less than 4096 char\n");
				continue;
			}
			buffer[strlen(buffer) -1] = '\0';
			//printf("%s\n", buffer);

			if(strncmp(buffer, "/sendfile", 9)==0 && buffer[9] ==' ')
			{
				memset(temp, 0, 4096);
				strncpy(temp, buffer, 4096);
				next = temp;
				token1 = strtok_r(next, " ", &next);
				token2 = strtok_r(next, " ", &next);
				token3 = strtok_r(next, " ", &next);
				//printf("bug_point1 %s %s %s\n",token1, token2, token3 );

				fp=fopen(token3 ,"rb");
				if (fp==NULL)
				{
					printf("file is empty\n");
					exit(EXIT_FAILURE);
				}
				while((read_byte = getline(&line, &len,fp)) != -1)
				{
					memset(buffer, 0, 4096);
					sprintf(buffer, "%s %s %s %s",token1, token2, token3, line);
					if ((no_byte = send(connector, buffer, 4096, 0)) <= 0)
					{
						if (no_byte == 0)
		 				{
		 					printf("CLIENT: Connection with SERVER is closed  \n");
		 					close(connector);
		 					exit(EXIT_FAILURE);

		 				}
		 				else
		 				{
		 					perror("CLIENT: ");
		 					close(connector);
		 					exit(EXIT_FAILURE);
		 				}

					}//..............end of send() else block..............................

				}
				fclose(fp);
				free(line);
				printf("file transfer completed\n");
			}
			else 
			{
					if ((no_byte = send(connector, buffer, 4096, 0)) <= 0)
					{
						if (no_byte == 0)
		 				{
		 					printf("CLIENT: Connection with SERVER is closed  \n");
		 					close(connector);
		 					exit(EXIT_FAILURE);

		 				}
		 				else
		 				{
		 					perror("CLIENT: ");
		 					close(connector);
		 					exit(EXIT_FAILURE);
		 				}

					}//..............end of send() else block..............................
					else
					{	if (strcmp(buffer, "/quit") == 0)
						{
							kill(pid, SIGKILL);
							exit(EXIT_SUCCESS);
						}
						else
							continue;
					}//..............end of send() else block..............................
			}
			
	
			
		}//............................end of writing for loop....................		
		
	}//............................................end of parent process else loop..................................
}	

