#include<stdio.h>  
#include<stdlib.h>
#include<string.h>
#include<limits.h>
#include<fcntl.h>
#include<ctype.h> // this is for type checking e.g. isspace()
#include<errno.h> // this header file defines integer variable errno which is set by system calls and some library functions in the event of an error. 
#include<unistd.h> // This header file defines miscellaneous symbolic constants, types and funtion related typical system call.
#include<sys/socket.h> // This header file defines miscellaneous socket related constants, types, funtion.
#include<sys/types.h> // This header file defines miscellaneous types
#include<sys/sem.h> // This header file defines semaphores related types, constants, functions.
#include<sys/wait.h> // This header file defines blocking, wait related system calls
#include<sys/ipc.h> // This header file defines interprocess communication access structure mainly semaphore, shared memory,and message. 
#include<netinet/in.h> // this header file defines system parameter related to address
#include<arpa/inet.h> // This header file defines some macro related to address len of different type address
#include<time.h> // This header file defines funtion to getting time
#include<sys/shm.h> // This header file defines system funtion related to shared memory
#include<netdb.h>  // This header file has definitions for network database operations
#include<signal.h>
/*..............................................................................................*/




/*...............We are defining P(s), V(s) operation in terms of semop() system call...........................*/
#define P(s) semop(s, &pop, 1)
#define V(s) semop(s, &vop, 1)
/*...................................................................*/


#define PORT "5001" // this is the port, server will be listening on.
#define BACKLOG 5 // how many pending connections queue will hold, though this might be ignored by os.
/*.............................................................................*/


#define MAX_OUTSTANDING_MESSAGE 4000 //THIS number of message can be resided in shared memory simultaneoustly




/*............Shared Memory........*/

typedef struct shared_memory                     //Structure is for shared memory architecture
  {
     int source;
     int dest;
     char message[4096];
  } SHARED_MEMORY;

typedef struct client_protocol               //Structure keeps protocol records for a client 
  {
     int client_sockId;
     int rule_one;
     int rule_two; 
  } CLIENT_PROTOCOL;

typedef struct connection_history            //Structure maintains Connection History
  {
    int no_present_connection;
    int source;
    int dest1;
    int dest2;
    int dest3;
    int dest4;
  } CONNECTION_HISTORY;

typedef struct connection_identifier         //Structure mintains Identifier against a socket ID
  {
     int socketId;
     int identifier;
     int group[10] ;
     int group_req[10] ;
     int request[10];
  } CONNECTION_IDENTIFIER;

typedef struct 
{
  int group_id;
  int no_member;
  int group_member[5] ;
 }GROUP; 

typedef struct 
{
  int group_id;
  int no_member;
  int request[10];
  int group_member[5];
}GROUP_REQ; 
/*.....................................................................................*/  




/*............declaration of global variable..........................*/
SHARED_MEMORY       *shared_buffer;         //Buffer will be created at shared memory section
CLIENT_PROTOCOL     *client_protocol;   
CONNECTION_HISTORY  *conn_history;
CONNECTION_IDENTIFIER  *sock_identifier;
GROUP *group_detail; 
GROUP_REQ *group_req_detail; 
int *no_client;// This keeps track no of clients.
int *outstanding_msg;
int *no_present_group;
int *no_present_group_req;
int shm_id1, shm_id2, shm_id3, shm_id4, shm_id5, shm_id6, shm_id7, shm_id8, shm_id9, shm_id10; //these will hold return value of shmget()
int listener;         // This is the socket descriptor, server will be listening on
/*.........................................................................................*/




/*.............................Semaphore Section............................*/
int client_protocol_lock;    //Simulating lock for protocol structre
int socket_identifier_lock;        //Simulating lock for Dictionary
int conn_hst_lock;           //Simulating lock for Conection History Table
int outstanding_msg_sem;           //Semaphore for shared memory section
int no_clients_sem;          //Semaphore keeps tracks of no. of active clients
int group_lock;            // semaphore keeps consistency of group shared memory
int group_req_lock;       // semaphore keeps consistency of group_req shared memory 
int no_present_group_lock; // semaphore keeps consistency of no_present_group shared memory
int no_present_group_req_lock; // semaphore keeps consistency of no_present_group_req_lock shared memory
/*........................................................................................*/

struct sembuf pop, vop ; // these are the structure, we have to pass to semop()





/*................Here we are checking whether any input int or not........................*/
int isInt (char *s)
{
   char *ep = NULL;

   // zero errno first!
   errno = 0;
   long i = strtol (s, &ep, 10);
   if (errno) {
       return 0;
   }

   // matching failure.
   if (ep == s) {
       return 0;
   }

   // garbage follows
   if (! ((*ep == 0) || (!strcmp(ep,"\n")))) {
      return 0;
   }

   // it is outside the range of `int`
   if (i < INT_MIN || i > INT_MAX) {
      return 0;
   }

   return 1; 
}
/*...............................................................................................*/






/*................convert ip address .........................................................*/
void *convert(struct sockaddr *client_addr)
{
	if (client_addr->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *)client_addr)->sin_addr);
	}
  return &(((struct sockaddr_in6 *)client_addr)->sin6_addr);
}
/*.............................................................................................*/




/*................This function return unique 4 digit random number................................*/
int generate_random_num_6()
{
  int random, not_unique = 1, findFlag = 0;
  srand(time(0));
  
  for (;not_unique == 1;)
  {
        random = rand() % 900000 + 100000;
        for(int i = 0; i < (*no_present_group_req); i++ )
        {
              if ((group_detail + i)->group_id == random)
              {
                findFlag = 1;
              }
        }
        if (findFlag != 1)
        {
          not_unique = 0;
        }
        findFlag = 0;
  }
  return random;
}/*.......................................................................................................*/  




/*................This function return unique 4 digit random number................................*/
int generate_random_num()
{
  int random, not_unique = 1, findFlag = 0;
  srand(time(0));
  
  for (;not_unique == 1;)
  {
        random = rand() % 9000 + 1000;
        for(int i = 0; i < (*no_present_group); i++ )
        {
              if ((group_detail + i)->group_id == random)
              {
                findFlag = 1;
              }
        }
        if (findFlag != 1)
        {
          not_unique = 0;
        }
        findFlag = 0;
  }
  return random;
}/*.......................................................................................................*/  






/*...........................This funtion return unique random number........................*/
int get_rand_num()
{
    int tempArr[5],randomNumber=0,not_unique=1,findFlag=0;
    srand(time(0));
      
    for(; not_unique == 1;)
    {
       randomNumber = rand() % 90000 + 10000; 
       for (int i=0; i < (*no_client); i++) //Verifying uniqueness of the number
       {
       	  if (((sock_identifier + i)-> identifier) == randomNumber)
       	  	findFlag = 1;
       }
       if (findFlag != 1)
       	{ not_unique=0; }
       findFlag = 0;
    }
      return randomNumber;
}
/*................................................................................................*/





/*.......................................This funtion for handling sigint signal.........................*/
void sigint_handler(int sig)
{
  char buffer[4096];
    int pos;
    memset(buffer, 0, 4096);
    //sprintf(buffer,"Server interrupted..");
  /*....Destroying the semaphore....*/
    semctl(outstanding_msg_sem, 0, IPC_RMID, 0);
    semctl(client_protocol_lock, 0, IPC_RMID, 0);
    semctl(conn_hst_lock, 0, IPC_RMID, 0);
    semctl(no_clients_sem, 0, IPC_RMID, 0);
    semctl(outstanding_msg_sem, 0, IPC_RMID, 0);
    semctl(no_present_group_lock, 0, IPC_RMID, 0);
    semctl(no_present_group_req_lock, 0, IPC_RMID, 0);
    semctl(group_lock, 0, IPC_RMID, 0);      // unused
    semctl(group_req_lock, 0, IPC_RMID, 0);  // unused

    close(listener); 
    /*
    for (pos=0; pos<(*no_client);pos++)
      {
           send((sock_identifier +pos)->socketId, buffer, strlen(buffer),0 );
           close((sock_identifier+pos)->socketId);       
   
      }*/

    /*....Destroying the sharedmemory....*/
    shmctl(shm_id1, IPC_RMID, 0);
    shmctl(shm_id2, IPC_RMID, 0);
    shmctl(shm_id3, IPC_RMID, 0);
    shmctl(shm_id4, IPC_RMID, 0);
    shmctl(shm_id5, IPC_RMID, 0);
    shmctl(shm_id6, IPC_RMID, 0);
    shmctl(shm_id7, IPC_RMID, 0);
    shmctl(shm_id8, IPC_RMID, 0);
    shmctl(shm_id9, IPC_RMID, 0);
    shmctl(shm_id10, IPC_RMID, 0);

       

    printf("Server is Closing.\n");
    /*...............................*/
  exit(EXIT_FAILURE);
}
/*.......................................................................................................*/





/*..............This funtion for handling for reaping zombie process...............*/
void sigchild_handler(int sig)
{
	int saved_errno = errno;
	for(;waitpid(-1, NULL, WNOHANG) > 0;);
	errno = saved_errno;	
}
/*...............................................................................*/





/*..............................This function performs a normal data transfer between clients.....................*/
void dataTransfer(int source, int dest,char* message) 
{ 
      char buffer[4096], forSource[4096], failure[4096];
      int position, sourceIdentifier, offlineFlag=1, no_byte;
      for ( position = 0; (sock_identifier + position) -> socketId != source ; position++);
            //if (( ) break;
      for (int i = 0; i < 10; ++i)
      {
        /* code */
      }

      /*...Checking if destination has gone offline....*/
      for (int i = 0; i < (*no_client); i++)
            if ((sock_identifier + i) -> socketId == dest ) offlineFlag = 0; 
      for (int i = 0; i < 5; ++i)
      {
        sourceIdentifier = (sock_identifier + position) -> identifier; 
      }
         
      
      
      
      

         // printf("%s\n",buffer );
      if ( offlineFlag  == 0)
      {
            memset(buffer, 0, 4096);
            sprintf(buffer,"->Message From User %d: ",sourceIdentifier);
            strncat(buffer, message, strlen(message) +1 );
            if((no_byte = send(dest, buffer , strlen(buffer) + 1, 0)) <= 0) // got error or connection closed by client
            { 
                    send(source, failure, strlen(failure) + 1,0);
                    if (no_byte == 0)
                    {
                      printf("SERVER: Connection with client  %d is closed . \n", dest);
                      close(dest);
                      exit(EXIT_FAILURE);
                    }
                    else
                    {
                      perror("SERVER: \n");
                      close(dest);
                      exit(EXIT_FAILURE);
                    }
          }// ..............end of send() if.................................
          else
          {
                    memset(forSource, 0, 4096);
              			sprintf(forSource,"->Reply From Server:Message sent.\n");
                    send(source , forSource, strlen(forSource) + 1,0);
          }  
            
    }
    else
     { 
        memset(failure, 0, 4096);
        sprintf(failure,"->Reply From Server:Message not sent.\n");
        send(source,failure,strlen(failure) + 1,0); 
    }
}
/*......................................................................................................................*/




/*............................This function handle joingroup utility...................................................*/
void join_group( int source_sock_id, int group_id)
{
	char buffer[4096], forSource[4096], failMessage[4096], forRequest[4096];
	int userIdentifier, position, destSocket, failFlag = 1,requestflag = 0, no_byte, j, i, group_req_position, admin_position, request_pos;
	sprintf(forRequest,"->Reply From Server:SUCCESSFULLY join request delivered.");

	sprintf(forSource,"->Reply From Server:You are now member of group Id.%d", group_id);
  printf(failMessage,">Reply From Server:Could not deliver join request, because, somehow it is not existed now.");

	for( j = 0; j < (*no_client); j++)
		if ((sock_identifier + j)->socketId == source_sock_id)break;

	userIdentifier = (sock_identifier + j)->identifier;  
	sprintf(buffer,"->This is a join group request from the user %d: for joining group Id.%d", userIdentifier, group_id);

	if(group_id < 10000)
     {
		 	for( position = 0; position < (*no_present_group); position++)
		    	if ((group_detail + position)->group_id == group_id)break;

		    P(no_present_group_lock);
		    (group_detail + position)->group_member[(group_detail + position)->no_member] == userIdentifier;
		    (group_detail + position)->no_member++; 
		    V(no_present_group_lock);
		    for ( i = 0; i < 10; ++i)
		    {
		    	if((sock_identifier + j)->group[i] == 0)break;
		    }
		    P(socket_identifier_lock);
		    (sock_identifier + j)->group[i] = group_id;
		    V(socket_identifier_lock);

			if((no_byte = send(source_sock_id , forSource , strlen(forSource) + 1, 0)) <= 0) // got error or connection closed by client
		      { 
		                if (no_byte == 0)
		                {
		                  printf("SERVER: Connection with client  %d is closed . \n", source_sock_id);
		                  close(source_sock_id);
		                }
		                else
		                {
		                  
		                }
		      }// ..............end of send() if.................................	 
     }

     else 
     {
     		for( position = 0; position < (*no_present_group_req); position++)
		    	if ((group_req_detail + position)->group_id == group_id)break;
		    for ( i = 0; i < 10; ++i)
		    {
		    	if((sock_identifier + j)->request[i] == group_id)
		    		{
		    				P(no_present_group_req_lock);
		    				(group_req_detail + position)->group_member[(group_req_detail + position)->no_member] = userIdentifier;
		    				(group_req_detail + position)->no_member;

		    				V(no_present_group_req_lock);
		    				for(group_req_position = 0; group_req_position < 10; group_req_position++)
		    				{
		    					if((sock_identifier + j)->group_req[group_req_position] == 0)break;
		    				}
		    				P(socket_identifier_lock);
						    (sock_identifier + j)->group_req[group_req_position] = group_id;
						    V(socket_identifier_lock);
						    if((no_byte = send(source_sock_id , forSource , strlen(forSource) + 1, 0)) <= 0) // got error or connection closed by client
						      { 
						                if (no_byte == 0)
						                {
						                  printf("SERVER: Connection with client  %d is closed . \n", source_sock_id);
						                  close(source_sock_id);
						                  continue;
						                }
						                else
						                {
						                  
						                }
						      }// ..............end of send() if.................................	
						    requestflag = 1;
		    				break;
		    		}		
		    }
		    if(requestflag == 0)
		    {
          for(request_pos = 0; request_pos < 5; request_pos++)
            if((group_req_detail + position)->request[request_pos] == 0)break;
          P(no_present_group_req_lock);
          (group_req_detail + position)->request[request_pos] = (sock_identifier + j)->identifier;
          V(no_present_group_req_lock);

          for(admin_position = 0; admin_position < (*no_client); admin_position++)
            if((sock_identifier + admin_position)->identifier == (group_req_detail + position)->group_member[0])break;

		    	if((no_byte = send((sock_identifier + admin_position)->socketId, buffer , strlen(buffer) + 1, 0)) <= 0) // got error or connection closed by client
			      { 
			                if (no_byte == 0)
			                {
			                  printf("SERVER: Connection with client  %d is closed . \n", source_sock_id);
			                  close(source_sock_id);
			                 } 
			                else
			                {
			                  
			                }
			      }// ..............end of send() if.................................
		    	if((no_byte = send((sock_identifier + j)->socketId, forRequest , strlen(forRequest) + 1, 0)) <= 0) // got error or connection closed by client
			      { 
			                if (no_byte == 0)
			                {
			                  printf("SERVER: Connection with client  %d is closed . \n", source_sock_id);
			                  close(source_sock_id);
			                }
			                else
			                {
			                  
			                }
			      }// ..............end of send() if.................................
		    }
	  }	    

} 
/*..................................................................................................................................*/







/*................................This function acceots joining group request................................................*/
void accept_request( int admin_sock_id, int req_id, int group_id)
{
      int group_pos, req_pos, position, entry_pos1, entry_pos2;
      char success_msg[4096], fail_msg[4096], welcome_msg[4096];

      for(group_pos = 0; group_pos < (*no_present_group_req); group_pos++)
        if((group_req_detail + group_pos)->group_id == group_id)break;
      for(req_pos= 0; (group_req_detail + group_pos)->request[req_pos] !=0; req_pos++)
        if((group_req_detail + group_pos)->request[req_pos] == req_id)
        {
              for(position = 0; position< (*no_client); position++)
                if((sock_identifier + position)->identifier == req_id)break;
              for(entry_pos1 =0; entry_pos1 < 5; entry_pos1++)
                if((sock_identifier + position)->group_req[entry_pos1] == 0)break;
              P(socket_identifier_lock);
              (sock_identifier + position)->group_req[entry_pos1] = group_id;
              V(socket_identifier_lock);
              sprintf(welcome_msg, "SERVER: Your join group request is accepted. You are now member of group Id.%d", group_id);
              send((sock_identifier + position)->socketId, welcome_msg, strlen(welcome_msg) + 1, 0);

              P(no_present_group_req_lock);
              (group_req_detail + group_pos)->group_member[(group_req_detail + group_pos)->no_member] = req_id;
              (group_req_detail + group_pos)->no_member++;
              V(no_present_group_req_lock);
              sprintf(success_msg, "SERVER: /accept instruction successful." );
              send(admin_sock_id, success_msg, strlen(success_msg) + 1, 0);

        }
        else
        {
              sprintf(fail_msg, "SERVER: /accept instruction failed. You do not have request to accept" );
              send(admin_sock_id, fail_msg, strlen(fail_msg) + 1, 0);
        }
}
/*...............................................................................................................................*/






/*.................................This function broadcasts message to all users.......................................*/
void group_message(int source_sock_id, int group_id, char* message)  
{
     char buffer[4096], forSource[4096], failMessage[4096];
     int userIdentifier, position, destSocket, failFlag = 1, no_byte, j, i;

     sprintf(forSource,"->Reply From Server:SUCCESSFULLY message delivered in group %d\n", group_id);
     sprintf(failMessage,">Reply From Server:Could not deliver, because, somehow it is not existed now or you are not part of that group\n");

     for( position = 0; position < (*no_client); position++)
        if ((sock_identifier + position)->socketId == source_sock_id)break;

     userIdentifier = (sock_identifier + position)->identifier;  
     sprintf(buffer,"->This is a group message from the user %d:\n%s", userIdentifier, message);
     if(group_id < 10000)
     {
		 	for( position = 0; position < (*no_present_group); position++)
		    if ((group_detail + position)->group_id == group_id)break;

			 /*.....For loop sends message to each one of the users....*/
			for( j = 0; j< (group_detail + position)->no_member; j++)
			 {
			 		destSocket = (group_detail + position)->group_member[j];
			 		for ( i = 0; i < (*no_client); ++i)
			 		{
			 			if((sock_identifier + i)->identifier == destSocket)break;
			 		}
			 		if ((sock_identifier + i)->socketId == source_sock_id)
			 		{
				 			if((no_byte = send((sock_identifier + i)->socketId, forSource , strlen(forSource) + 1, 0)) <= 0) // got error or connection closed by client
						      { 
						                if (no_byte == 0)
						                {
						                  printf("SERVER: Connection with client  %d is closed . \n", source_sock_id);
						                  close(source_sock_id);
						                  continue;
						                }
						                else
						                {
						                  
						                }
						      }// ..............end of send() if.................................
			 		}
			 		else
			 		{
			 				if((no_byte = send((sock_identifier + i)->socketId, buffer , strlen(forSource) + 1, 0)) <= 0) // got error or connection closed by client
						      { 
						                if (no_byte == 0)
						                {
						                  printf("SERVER: Connection with client  %d is closed . \n", source_sock_id);
						                  close(source_sock_id);
						                  continue;
						                }
						                else
						                {
						                  
						                }
						      }// ..............end of send() if.................................
			 		}
			 		
			 }
     }
     else 
     {
     		for( position = 0; position < (*no_present_group_req); position++)
		    if ((group_req_detail + position)->group_id == group_id)break;

			 /*.....For loop sends message to each one of the users....*/
			for( j = 0; j< (group_req_detail + position)->no_member; j++)
			 {
			 		destSocket = (group_req_detail + position)->group_member[j];
			 		for ( i = 0; i < (*no_client); ++i)
			 		{
			 			if((sock_identifier + i)->identifier == destSocket)break;
			 		}
			 		if ((sock_identifier + i)->socketId == source_sock_id)
			 		{
				 			if((no_byte = send((sock_identifier + i)->socketId, forSource , strlen(forSource) + 1, 0)) <= 0) // got error or connection closed by client
						      { 
						                if (no_byte == 0)
						                {
						                  printf("SERVER: Connection with client  %d is closed . \n", source_sock_id);
						                  close(source_sock_id);
						                  continue;
						                }
						                else
						                {
						                  
						                }
						      }// ..............end of send() if.................................
			 		}
			 		else
			 		{
			 				if((no_byte = send((sock_identifier + i)->socketId, buffer , strlen(forSource) + 1, 0)) <= 0) // got error or connection closed by client
						      { 
						                if (no_byte == 0)
						                {
						                  printf("SERVER: Connection with client  %d is closed . \n", source_sock_id);
						                  close(source_sock_id);
						                  continue;
						                }
						                else
						                {
						                  
						                }
						      }// ..............end of send() if.................................
			 		}
			 		
			 }
     }

      
    
}/*..........................................................................................................................*/






void file_transfer(int source_sock_id, int group_id, char* message)  
{//printf("BUg_point0 %d\n%s\n",group_id, message);
     char buffer[4096], forSource[4096],forClient[4096], failMessage[4096];
     int userIdentifier, position, destSocket, failFlag = 1, no_byte, j, i;

     sprintf(forSource,"->Reply From Server:SUCCESSFULLY file transfering in group %d\n", group_id);
     sprintf(forClient,"->Reply From Server:SUCCESSFULLY file transfering to client Id. %d\n", group_id);
     sprintf(failMessage,">Reply From Server:Could not deliver, because, somehow it is not existed now or you are not part of that group\n");

     for( position = 0; position < (*no_client); position++)
        if ((sock_identifier + position)->socketId == source_sock_id)break;

     userIdentifier = (sock_identifier + position)->identifier;  
     sprintf(buffer,"->This is a group message from the user %d:\n%s", userIdentifier, message);
     if(group_id < 10000)
     {//printf("BUg_point5\n");
      for( position = 0; position < (*no_present_group); position++)
        if ((group_detail + position)->group_id == group_id)break;

       /*.....For loop sends message to each one of the users....*/
      for( j = 0; j< (group_detail + position)->no_member; j++)
       {
          destSocket = (group_detail + position)->group_member[j];
          for ( i = 0; i < (*no_client); ++i)
          {
            if((sock_identifier + i)->identifier == destSocket)break;
          }
          if ((sock_identifier + i)->socketId == source_sock_id)
          {
              if((no_byte = send((sock_identifier + i)->socketId, forSource , strlen(forSource) + 1, 0)) <= 0) // got error or connection closed by client
                  { 
                            if (no_byte == 0)
                            {
                              printf("SERVER: Connection with client  %d is closed . \n", source_sock_id);
                              close(source_sock_id);
                              continue;
                            }
                            else
                            {
                              
                            }
                  }// ..............end of send() if.................................
          }
          else
          {
              if((no_byte = send((sock_identifier + i)->socketId, message , strlen(message) + 1, 0)) <= 0) // got error or connection closed by client
                  { 
                            if (no_byte == 0)
                            {
                              printf("SERVER: Connection with client  %d is closed . \n", source_sock_id);
                              close(source_sock_id);
                              continue;
                            }
                            else
                            {
                              
                            }
                  }// ..............end of send() if.................................
          }
          
       }
     }
     else if(group_id < 100000)
     {//printf("BUg_point1\n");
          for( position = 0; position < (*no_client); position++)
            if ((sock_identifier + position)->identifier == group_id)break;  
          if((no_byte = send(source_sock_id, forClient , strlen(forClient) + 1, 0)) <= 0) // got error or connection closed by client
              { 
                        if (no_byte == 0)
                        {
                          printf("SERVER: Connection with client  %d is closed . \n", source_sock_id);
                          close(source_sock_id);
                          exit(EXIT_FAILURE);
                        }
                        else
                        {
                          
                        }
              }// ..............end of send() if.................................
           
              
          if((no_byte = send((sock_identifier + position)->socketId, message , strlen(message) + 1, 0)) <= 0) // got error or connection closed by client
              { 
                        if (no_byte == 0)
                        {
                          printf("SERVER: Connection with client  %d is closed . \n", source_sock_id);
                          close(source_sock_id);
                          exit(EXIT_FAILURE);
                        }
                        else
                        {
                          
                        }
              }// ..............end of send() if.................................
       
     }
     else 
     {//printf("BUg_point6\n");
        for( position = 0; position < (*no_present_group_req); position++)
        if ((group_req_detail + position)->group_id == group_id)break;

       /*.....For loop sends message to each one of the users....*/
      for( j = 0; j< (group_req_detail + position)->no_member; j++)
       {
          destSocket = (group_req_detail + position)->group_member[j];
          for ( i = 0; i < (*no_client); ++i)
          {
            if((sock_identifier + i)->identifier == destSocket)break;
          }
          if ((sock_identifier + i)->socketId == source_sock_id)
          {
              if((no_byte = send((sock_identifier + i)->socketId, forSource , strlen(forSource) + 1, 0)) <= 0) // got error or connection closed by client
                  { 
                            if (no_byte == 0)
                            {
                              printf("SERVER: Connection with client  %d is closed . \n", source_sock_id);
                              close(source_sock_id);
                              continue;
                            }
                            else
                            {
                              
                            }
                  }// ..............end of send() if.................................
          }
          else
          {
              if((no_byte = send((sock_identifier + i)->socketId, buffer , strlen(forSource) + 1, 0)) <= 0) // got error or connection closed by client
                  { 
                            if (no_byte == 0)
                            {
                              printf("SERVER: Connection with client  %d is closed . \n", source_sock_id);
                              close(source_sock_id);
                              continue;
                            }
                            else
                            {
                              
                            }
                  }// ..............end of send() if.................................
          }
          
       }
     }

      
    
}/*..........................................................................................................................*/     





/*.................................This function broadcasts message to all users.......................................*/
void broadcast_message(int source,char* message)  
{
     char buffer[4096], forSource[4096], failMessage[4096];
     int userIdentifier, position, destSocket, failFlag = 1, no_byte;

     sprintf(forSource,"->Reply From Server:SUCCESSFULLY BROADCASTED");
     sprintf(failMessage,">Reply From Server:Could not broadcast as there are no users online.");

     for( position = 0; position < *(no_client); position++)
        if ((sock_identifier + position)->socketId == source)break;

     userIdentifier = (sock_identifier + position) -> identifier;  
     sprintf(buffer,"->This is a broadcast message from the user %d:\n%s", userIdentifier, message);
    
     /*.....For loop sends message to each one of the users....*/
     for (int i = 0; i < (*no_client); i++)
     {     if(i != position)
          { 
                destSocket = (sock_identifier + i)->socketId;
                if((no_byte = send(destSocket, buffer , strlen(buffer) + 1, 0)) <= 0) // got error or connection closed by client
                { 
                          if (no_byte == 0)
                          {
                            printf("SERVER: Connection with client  %d is closed . \n", destSocket);
                            close(destSocket);
                            exit(EXIT_FAILURE);
                          }
                          else
                          {
                            perror("SERVER: \n");
                            close(destSocket);
                            exit(EXIT_FAILURE);
                          }
                }// ..............end of send() if.................................
                failFlag = 0; 
          }    
      }
      if (failFlag == 0)
      {
         if((no_byte = send(source, forSource , strlen(forSource) + 1, 0)) <= 0) // got error or connection closed by client
          { 
                    if (no_byte == 0)
                    {
                      printf("SERVER: Connection with client  %d is closed . \n", source);
                      close(source);
                      exit(EXIT_FAILURE);
                    }
                    else
                    {
                      perror("SERVER: \n");
                      close(source);
                      exit(EXIT_FAILURE);
                    }
          }// ..............end of send() if.................................
      }
      else                                        //Handling the case of no broadcasting
      {
          send(source,failMessage,strlen(failMessage),0);
          if((no_byte = send(source, failMessage , strlen(failMessage) + 1, 0)) <= 0) // got error or connection closed by client
          { 
                    if (no_byte == 0)
                    {
                      printf("SERVER: Connection with client  %d is closed . \n", source);
                      close(source);
                      exit(EXIT_FAILURE);
                    }
                    else
                    {
                      perror("SERVER: \n");
                      close(source);
                      exit(EXIT_FAILURE);
                    }
          }// ..............end of send() if.................................  
      }  
}/*..........................................................................................................................*/







 /*..........................Here we handle /makegroup utility............................................*/
void make_group(int socketId, char* next)
{	
	  int groupId, i, j,k, l ,okFlag = 1, Clientid, no_byte, temp_sock, present_group_member;
      char temp[4096], message[4096],buffer[4096], *token, *str;

      groupId = generate_random_num();
      strncpy(temp, next, strlen(next) + 1);
      str = temp;
	  for (i = 0; i < (*no_client); ++i)
	  {
	  	 if((sock_identifier + i)->socketId == socketId)
	  	 	Clientid = (sock_identifier + i)->identifier;
	  }
      //printf("%d\n",Clientid );
      P(no_present_group_lock);
      (group_detail + (*no_present_group))->group_member[0] = Clientid;
      (group_detail + (*no_present_group))->no_member = 1;

      
      for(i = 1; (token = strtok_r(str, " ", &str)) != NULL ; i++)
      {
            if (i > 4)// no of client can not be more than 5 because, we are only allowing atmost 5 client simultaneously.
            {
              okFlag = 0;
              break;
            }
            if (!isInt(token) || strlen(token)!= 5  )// to check the ClientId int or not and its length 5 or not
            {
              okFlag = 0;
              break;
            }
            for ( j =0; j < (*no_client) ; ++j)// to check the Clientid id is valid or not
            {
                  okFlag = 0;
                  if ((sock_identifier + j)->identifier == atoi(token))
                  {
                    okFlag = 1;
                    break;
                  }   
            }
            if (okFlag == 0)break;
            for ( k = 0; k < (group_detail + (*no_present_group))->no_member; ++k)// check whether ClientId is repeating or not
            {
                 if((group_detail + (*no_present_group))->group_member[k] == atoi(token))
                 {
                    okFlag = 0;
                    break;
                 }
            }
            if (okFlag == 0)break;
            if(okFlag == 1)
            {
		              present_group_member = (group_detail + (*no_present_group))->no_member;
		              (group_detail + (*no_present_group))->group_member[present_group_member] = atoi(token);
		              (group_detail + (*no_present_group))->no_member++;
		              memset(buffer, 0, 4096);
		        	  sprintf(buffer,"Reply From Server:->\nClient %d added you in group ID.%d", Clientid, groupId);
		        	  temp_sock = (sock_identifier + j)->socketId;
		              if((no_byte = send(temp_sock, buffer , strlen(buffer) + 1, 0)) <= 0) // got error or connection closed by client
			 		  { 
			 		  			memset(buffer, 0, 4096);
			     				sprintf(buffer,"Reply From Server:->\nclient id.%d closed", (sock_identifier + j)->identifier);
			    				if((no_byte = send(socketId, buffer , strlen(buffer) + 1, 0)) <= 0) // got error or connection closed by client
			 					{ 
					     				if (no_byte == 0)
					     				{
					     					printf("SERVER: Connection with client %d is closed ", Clientid);
					     					close(socketId);
					             			exit(EXIT_FAILURE);

					     				}
					     				else
					     				{
					     					
					     				}	
			     				}	 
			          }

         	}
       }

      if(okFlag == 0 )
      {   
          strncpy(message,"PROTOCOL_FAILURE", strlen("PROTOCOL_FAILURE") + 1);
          P(outstanding_msg_sem);
          (shared_buffer + (*outstanding_msg)) -> dest = socketId;
          (shared_buffer + (*outstanding_msg)) -> source = -1; 
          strncpy((shared_buffer + (*outstanding_msg)) ->message, message, strlen(message) + 1);
          (*outstanding_msg)++;
          V(outstanding_msg_sem);
            
          V(no_present_group_lock);
      }
      else
      {
      			
		        (group_detail + (*no_present_group))->group_id = groupId;
		        memset(buffer, 0, 4096);
		        sprintf(buffer,"Reply From Server:->\nGROUP SUCCESSFULLY CREATED.ID.%d", groupId);
			    if((no_byte = send(socketId, buffer , strlen(buffer) + 1, 0)) <= 0) // got error or connection closed by client
			 	{ 
			     				if (no_byte == 0)
			     				{
			     					printf("SERVER: Connection with client %d is closed ", Clientid);
			     					close(socketId);
			             			exit(EXIT_FAILURE);

			     				}
			     				else
			     				{}
			     					
			              
			    }
		        for (k = 0; k < (*no_client); k++)
		        {
		        	if((sock_identifier + k)->socketId == socketId)
		        	{

		        		for ( l = 0; l < 5; l++)
		        		{
		        			if((sock_identifier + k)->group[l] == 0)break;
		        		}
		        		P(socket_identifier_lock);
		        		(sock_identifier + k)->group[l] = groupId;
		        		V(socket_identifier_lock);
		        		break;

		        	}
		        }
		        for (i= 0; i <= (*no_present_group); ++i)
		        {
					if ((group_detail + i)->group_id== groupId)
					{
						for (j = 1;  j< (group_detail + i)->no_member; ++j)
						{
							for (k = 0; k < (*no_client); ++k)
					        {
					        	if ((sock_identifier + k)->identifier == ((group_detail + i)->group_member[j]))
					        	{

					        		for ( l = 0; l < 5; ++l)
					        		{
					        			if((sock_identifier + k)->group[l] == 0)break;
					        		}
					        		P(socket_identifier_lock);
					        		(sock_identifier + k)->group[l] = groupId;
					        		V(socket_identifier_lock);
					        		break;

					        	}
					        }
						}
						break;
					}
							        			        	
		        }

		        (*no_present_group)++;
		        V(no_present_group_lock);
      }
}//.........................................................................................................................
     





/*....................This function makes the connection between source and destination..................................*/

void make_connection( int sourceSocket, int destIdentifier, char *message) 
{
      int no_byte, position, findFlag=0, destSockId, source_position, destination_position,sourceIdentifier, i;
      char buffer[4096], numToString[4096];
      
      memset(buffer, 0, 4096);
      memset(numToString, 0, 4096);
  
      for ( position=0; position<(*no_client); position++)
      {
           if ((sock_identifier + position) -> identifier == destIdentifier )
          {
                  findFlag =1;
                  break;
          }
      }
      
      if ( findFlag == 1)
      {
               destSockId = (sock_identifier + position) -> socketId;

               /*....Checking if you are trying to connect to yourself...*/              
               if( sourceSocket == destSockId)
               {
                     strncpy(buffer,"->Please provide a different userID.", strlen("->Please provide a different userID.") + 1);
                     if((no_byte = send(sourceSocket, buffer , strlen(buffer) + 1, 0)) <= 0) // got error or connection closed by client
                     { 
                              if (no_byte == 0)
                              {
                                printf("SERVER: Connection with client  %d is closed . \n", sourceSocket);
                                close(sourceSocket);
                                exit(EXIT_FAILURE);
                              }
                              else
                              {
                                perror("SERVER: \n");
                                close(sourceSocket);
                                exit(EXIT_FAILURE);
                              }
                     }// ..............end of send() if.................................
                     return;
              }

              for (destination_position = 0; destination_position < (*no_client); destination_position++ )
                if ((client_protocol + destination_position)->client_sockId == destSockId)break;

              for(source_position = 0; source_position < (*no_client); source_position++)
                if ((client_protocol + source_position)->client_sockId == sourceSocket)break;

              /*....Updating required table for making the successful connection...*/    
              P(client_protocol_lock);
               (client_protocol + source_position) -> rule_two = 0;
               (client_protocol + destination_position) -> rule_one = 0;
               (client_protocol + destination_position) -> rule_two = 0;
              V(client_protocol_lock); 
                      
              /*....Updating required table for making the successful connection...*/ 
              P(conn_hst_lock);
               i = (conn_history + source_position)->no_present_connection;
               switch(i + 1 )
               {
                    case 1:
                      (conn_history + source_position)-> dest1 = destSockId; 
                    case 2:
                      (conn_history + source_position)-> dest2 = destSockId;
                    case 3:
                      (conn_history + source_position)-> dest3 = destSockId;
                    case 4:
                      (conn_history + source_position)-> dest4 = destSockId;      
               }
               (conn_history + source_position)->no_present_connection++;
               i = (conn_history + destination_position)->no_present_connection;
               switch(i + 1 )
               {
                    case 1:
                      (conn_history + destination_position)-> dest1 = sourceSocket; 
                    case 2:
                      (conn_history + destination_position)-> dest2 = sourceSocket;
                    case 3:
                      (conn_history + destination_position)-> dest3 = sourceSocket;
                    case 4:
                      (conn_history + destination_position)-> dest4 = sourceSocket;      
               }
               (conn_history + destination_position)->no_present_connection++;
              V(conn_hst_lock);

              for (  i = 0; i<(*no_client); i++ )
                  if ((sock_identifier + i)->socketId == sourceSocket)break;
               
              /*....Relaying the messages to the clients...*/  
              sourceIdentifier = (sock_identifier + i) -> identifier;
              sprintf(buffer,"You are connected to the user:%d", sourceIdentifier);
              if((no_byte = send(destSockId, buffer , strlen(buffer) + 1, 0)) <= 0) // got error or connection closed by client
              { 
                      if (no_byte == 0)
                      {
                        printf("SERVER: Connection with client  %d is closed . \n", destSockId);
                        close(destSockId);
                        exit(EXIT_FAILURE);
                      }
                      else
                      {
                        perror("SERVER: \n");
                        close(destSockId);
                        exit(EXIT_FAILURE);
                      }
              }// ..............end of send() if.................................
              memset(buffer, 0, 4096);
              sprintf(buffer,"You are connected to the user:%d", destIdentifier);
              if((no_byte = send(sourceSocket, buffer , strlen(buffer) + 1, 0)) <= 0) // got error or connection closed by client
              { 
                      if (no_byte == 0)
                      {
                        printf("SERVER: Connection with client  %d is closed . \n", sourceSocket);
                        close(sourceSocket);
                        exit(EXIT_FAILURE);
                      }
                      else
                      {
                        perror("SERVER: \n");
                        close(sourceSocket);
                        exit(EXIT_FAILURE);
                      }
              }// ..............end of send() if................................. 

              dataTransfer(sourceSocket, destSockId, message);  //Main application calls data transfer
             
                  
      }
      else
      {
            sprintf(buffer,"->User is offline");
            if((no_byte = send(sourceSocket, buffer , strlen(buffer) + 1, 0)) <= 0) // got error or connection closed by client
            { 
                      if (no_byte == 0)
                      {
                        printf("SERVER: Connection with client  %d is closed . \n", sourceSocket);
                        close(sourceSocket);
                        exit(EXIT_FAILURE);
                      }
                      else
                      {
                        perror("SERVER: \n");
                        close(sourceSocket);
                        exit(EXIT_FAILURE);
                      }
            }// ..............end of send() if................................. 
      }/*,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,end of findflag check else block,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*/
     
} 
/*........................................................................................................................*/






/*................................This function displays all groups........................................................*/
void show_all_groups (int dest)
{
     
     char message[4096];
     char temp[4096];
     int position, ClientId, i;
     int no_byte; // this holds return value of send() 
     
     /*...Initializing the memory..*/
     memset(message, 0, 4096);
     memset(temp, 0, 4096);
     strncpy(message,"Reply From Server:->Active groups:", strlen("Reply From Server:->Active groups:") + 1);

     for ( i = 0; i < (*no_present_group);i++)
     {
          sprintf(temp,"\ngroup no.%d",(group_detail + i)->group_id);
          strncat(message, temp, strlen(temp) + 1);
     }
     for ( i = 0; i < (*no_present_group_req);i++)
     {
          sprintf(temp,"\ngroup no.%d",(group_req_detail + i)->group_id);
          strncat(message, temp, strlen(temp) + 1);
     }
     if((no_byte = send(dest, message , strlen(message) + 1, 0)) <= 0) // got error or connection closed by client
    { 
            if (no_byte == 0)
            {
              printf("SERVER: Connection with client  %d is closed . \n", dest);
              close(dest);
              exit(EXIT_FAILURE);
            }
            else
            {
              
            }

    }// ..............end of send() if.................................
     
    
}    
/*.............................................................................................................*/






/*..........................Here we handle /makegroup utility............................................*/
void make_groupreq(int socketId,char* next)
{ 
      int groupId, position,i, j,k, l ,okFlag = 1, Clientid, no_byte, temp_sock, present_group_member;
      char temp[4096], message[4096],buffer[4096], *token, *str;

      groupId = generate_random_num_6();
      strncpy(temp, next, strlen(next) + 1);
      str = temp;
      for (position = 0; position < (*no_client); ++position)
      {
         if((sock_identifier + position)->socketId == socketId)
          Clientid = (sock_identifier + position)->identifier;
      }
      //printf("%d\n",Clientid );
      P(no_present_group_req_lock);
      (group_req_detail + (*no_present_group_req))->group_member[0] = Clientid;
      (group_req_detail + (*no_present_group_req))->no_member = 1;

      
      for(i = 1; (token = strtok_r(str, " ", &str)) != NULL ; i++)
      {
            if (i > 4)// no of client can not be more than 5 because, we are only allowing atmost 5 client simultaneously.
            {
              okFlag = 0;
              break;
            }
            if (!isInt(token) || strlen(token)!= 5  )// to check the ClientId int or not and its length 5 or not
            {
              okFlag = 0;
              break;
            }
            for ( j =0; j < (*no_client) ; ++j)// to check the Clientid id is valid or not
            {
                  okFlag = 0;
                  if ((sock_identifier + j)->identifier == atoi(token))
                  {
                    okFlag = 1;
                    break;
                  }   
            }
            if (okFlag == 0)break;
            for ( k = 0; k < (group_req_detail + (*no_present_group_req))->no_member; ++k)// check whether ClientId is repeating or not
            {
                 if((group_req_detail + (*no_present_group_req))->group_member[k] == atoi(token))
                 {
                    okFlag = 0;
                    break;
                 }
            }
            if (okFlag == 0)break;
            if(okFlag == 1)
            {
                  for ( l = 0; l < 5; ++l)
                  {
                    if((sock_identifier + j)->request[l] == 0)break;
                  }
                  P(socket_identifier_lock);
                  (sock_identifier + j)->request[l] = groupId;
                  V(socket_identifier_lock);  
                  /*
                    present_group_member = (group_req_detail + (*no_present_group_req))->no_member;
                    (group_req_detail + (*no_present_group_req))->group_member[present_group_member] = atoi(token);
                    (group_req_detail + (*no_present_group_req))->no_member++;*/
                  memset(buffer, 0, 4096);
                  sprintf(buffer,"Reply From Server:->\nClient %d want to add you in group ID.%d", Clientid, groupId);
                  temp_sock = (sock_identifier + j)->socketId;
                  if((no_byte = send(temp_sock, buffer , strlen(buffer) + 1, 0)) <= 0) // got error or connection closed by client
                  { 
                        memset(buffer, 0, 4096);
                        sprintf(buffer,"Reply From Server:->\nclient id.%d closed", (sock_identifier + j)->identifier);
                        if((no_byte = send(socketId, buffer , strlen(buffer) + 1, 0)) <= 0) // got error or connection closed by client
                        { 
                            if (no_byte == 0)
                            {
                              printf("SERVER: Connection with client %d is closed ", Clientid);
                              close(socketId);
                                  exit(EXIT_FAILURE);

                            }
                            else
                            {
                              
                            } 
                        }  
                  }
            }
      }

      if(okFlag == 0 )
      {   
          strncpy(message,"PROTOCOL_FAILURE", strlen("PROTOCOL_FAILURE") + 1);
          P(outstanding_msg_sem);
          (shared_buffer + (*outstanding_msg)) -> dest = socketId;
          (shared_buffer + (*outstanding_msg)) -> source = -1; 
          strncpy((shared_buffer + (*outstanding_msg)) ->message, message, strlen(message) + 1);
          (*outstanding_msg)++;
          V(outstanding_msg_sem);
            
          V(no_present_group_req_lock);
      }
      else
      {
            (group_req_detail + (*no_present_group_req))->group_id = groupId;
            memset(buffer, 0, 4096);
            sprintf(buffer,"Reply From Server:->\nGROUP SUCCESSFULLY CREATED.ID.%d", groupId);
            if((no_byte = send(socketId, buffer , strlen(buffer) + 1, 0)) <= 0) // got error or connection closed by client
            { 
                  if (no_byte == 0)
                  {
                    printf("SERVER: Connection with client %d is closed ", Clientid);
                    close(socketId);
                    exit(EXIT_FAILURE);

                  }
                  else
                  {}
            }
            for (k = 0; k < (*no_client); k++)
            {
                  if((sock_identifier + k)->socketId == socketId)
                  {

                    for ( l = 0; l < 5; l++)
                    {
                      if((sock_identifier + k)->group_req[l] == 0)break;
                    }

                    P(socket_identifier_lock);
                    (sock_identifier + k)->group_req[l] = groupId;
                    //printf("%d %d %d\n",(sock_identifier + k)->group[l],(sock_identifier + k)->socketId, (sock_identifier + k)->identifier );
                    V(socket_identifier_lock);
                    break;

                  }
            }

            (*no_present_group_req)++;
            V(no_present_group_req_lock);
        }
}//.........................................................................................................................








/*...................................This function  displays the active groups_req.....................................*/

void show_groups_req (int dest) 
{
     
     char message[4096];
     char temp[4096];
     int position, ClientId, i;
     int no_byte; // this holds return value of send() 
     
     /*...Initializing the memory..*/
     memset(message, 0, 4096);
     memset(temp, 0, 4096);
     strncpy(message,"Reply From Server:->Active groups with request option:", strlen("Reply From Server:->Active groups with request option:") + 1);

     for(position = 0; position < (*no_client); position++)
     {
          if((sock_identifier + position)->socketId == dest) break;
     }
     
     /*..For loop goes through each of the active clients..*/
     for ( i=0; (sock_identifier + position)->group_req[i] != 0;i++)
     {
          sprintf(temp,"\ngroup no.%d",(sock_identifier + position)->group_req[i]);
          strncat(message, temp, strlen(temp) + 1);
     }
     if ( i == 0)
      {
      		memset(message,0, 4096);
      		sprintf(message, "\nReply From Server:->\nYOU ARE NOT PART OF ANY GROUP.");
	      	if((no_byte = send(dest, message , strlen(message) + 1, 0)) <= 0) // got error or connection closed by client
	   		{ 
			        if (no_byte == 0)
			        {
			          printf("SERVER: Connection with client  %d is closed . \n", dest);
			          close(dest);
			          exit(EXIT_FAILURE);
			        }
			        else
			        {
			          
			        }     
			} 
	 }
	 else
	 {
	 		if((no_byte = send(dest, message , strlen(message) + 1, 0)) <= 0) // got error or connection closed by client
		    { 
		            if (no_byte == 0)
		            {
		              printf("SERVER: Connection with client  %d is closed . \n", dest);
		              close(dest);
		              exit(EXIT_FAILURE);
		            }
		            else
		            {
		              
		            }

		    }// ..............end of send() if.................................
	 }	
    
}    
/*.............................................................................................................*/






/*...................................This function  displays the active groups.....................................*/

void show_groups (int dest) 
{
     
     char message[4096];
     char temp[4096];
     int position, ClientId, i;
     int no_byte; // this holds return value of send() 
     
     /*...Initializing the memory..*/
     memset(message, 0, 4096);
     memset(temp, 0, 4096);
     strncpy(message,"Reply From Server:->Active groups:", strlen("Reply From Server:->Active groups:") + 1);

     for(position = 0; position < (*no_client); position++)
     {
          if((sock_identifier + position)->socketId == dest) break;
     }
     
     /*..For loop goes through each of the active clients..*/
     for ( i=0; (sock_identifier + position)->group[i] != 0;i++)
     {
          sprintf(temp,"\ngroup no.%d",(sock_identifier+position)->group[i]);
          strncat(message, temp, strlen(temp) + 1);
     }
     if ( i == 0)
      {
      		memset(message,0, 4096);
      		sprintf(message, "\nReply From Server:->\nYOU ARE NOT PART OF ANY GROUP.");
	      	if((no_byte = send(dest, message , strlen(message) + 1, 0)) <= 0) // got error or connection closed by client
	   		{ 
			        if (no_byte == 0)
			        {
			          printf("SERVER: Connection with client  %d is closed . \n", dest);
			          close(dest);
			          exit(EXIT_FAILURE);
			        }
			        else
			        {
			          
			        }     
			} 
	 }
	 else
	 {
	 		if((no_byte = send(dest, message , strlen(message) + 1, 0)) <= 0) // got error or connection closed by client
		    { 
		            if (no_byte == 0)
		            {
		              printf("SERVER: Connection with client  %d is closed . \n", dest);
		              close(dest);
		              exit(EXIT_FAILURE);
		            }
		            else
		            {
		              
		            }

		    }// ..............end of send() if.................................
	 }	
    
}    
/*.............................................................................................................*/





/*...................................This function  displays the active users.....................................*/

void show_users (int dest) 
{
     
     char message[4096];
     char temp[4096];
     int position;
     int no_byte; // this holds return value of send() 
     
     /*...Initializing the memory..*/
     memset(message, 0, 4096);
     memset(temp, 0, 4096);
     strncpy(message,"Reply From Server:->Active User:", strlen("Reply From Server:->Active User:") + 1);
     
     /*..For loop goes through each of the active clients..*/
     for ( int i=0;i<(*no_client);i++)
     {
           if ((sock_identifier+i) -> socketId == dest )
            {
               sprintf(temp,"\nClient no.%d\tYou->%d", i+1, (sock_identifier+i)->identifier);
            }
           else
            {
               sprintf(temp,"\nClient no.%d\t%d", i+1, (sock_identifier+i)->identifier);
            } 
           strncat(message, temp, strlen(temp) + 1);
     } 
    if((no_byte = send(dest, message , strlen(message) + 1, 0)) <= 0) // got error or connection closed by client
    { 
            if (no_byte == 0)
            {
              printf("SERVER: Connection with client  %d is closed . \n", dest);
              close(dest);
              exit(EXIT_FAILURE);
            }
            else
            {
              
            }

    }// ..............end of send() if.................................
      
    for (position=0; position < (*no_client); position++)
      if ( ((client_protocol + position) -> client_sockId) == dest)break;

      
    P(client_protocol_lock);
    (client_protocol + position) -> rule_one = 0; 
    V(client_protocol_lock);   

}
/*.............................................................................................................*/





/*............This funtion handle all send() operation................................*/
void write_handler()
{
          char buffer[4096], message[4096];
          int source, dest, no_byte;
          if ((*outstanding_msg) > 0)
          {   
              P( outstanding_msg_sem );
              (*outstanding_msg)--;
              for(;(*outstanding_msg) >= 0;)
              {     /*..Initializing the memory..*/
                   memset(message,0, 4096);
                   memset(buffer, 0, 4096);
                   
                   source = (shared_buffer+(*outstanding_msg)) -> source;
                   dest   = (shared_buffer+(*outstanding_msg)) -> dest;
                  //  sem_post(sharedMemoryLock);
                   strncpy( message, (shared_buffer +(*outstanding_msg))-> message, 4096 );
                   if (source == -1)
                   {
                            
                            
                            if(strncmp(message,"/activegroups", strlen("/activegroups")) == 0 && strlen(message) == 13) //Main process calls Show Users
                            {
                                   show_groups(dest);
                                   show_groups_req(dest);
                            }
                            else if(strncmp(message,"/activeallgroups", strlen("/activeallgroups")) == 0 && strlen(message) == 16) //Main process calls Show Users
                            {
                                   show_all_groups(dest);
                                   
                            }
                            else if((strncmp(message,"/active", strlen("/active")) == 0) &&  strlen(message) == 7) //Main process calls Show Users
                            {
                                   show_users(dest);
                            }
                            else if(strncmp(message,"/accept", 7) == 0 && message[7] == ' ')//Main process calls connect()
                            {
                                  char temp[4096], *token1, *token2, *next;
                                  int unique_no;
                                  strncpy(temp, message, 4096);
                                  token1 = strtok_r(temp, " ", &next);
                                  token2 = strtok_r(NULL, " ", &next);
                                  unique_no = atoi(token2);
                                  accept_request( dest, unique_no, atoi(next));  
                            }
                            else if(strncmp(message,"/send", 5) == 0 && message[5] == ' ')//Main process calls connect()
                            {
                                  char temp[4096], *token1, *token2, *next;
                                  int unique_no;
                                  strncpy(temp, message, 4096);
                                  token1 = strtok_r(temp, " ", &next);
                                  token2 = strtok_r(NULL, " ", &next);
                                  unique_no = atoi(token2);
                                  make_connection( dest, unique_no, next);  
                            }
                            else if(strncmp(message,"/makegroupreq", 13) == 0 && message[13] == ' ')//Main process calls connect()
                            {
                                  char temp[4096], *token1, *next;
                                  int unique_no;
                                  strncpy(temp, message, 4096);
                                  token1 = strtok_r(temp, " ", &next);                                 
                                  make_groupreq( dest, next); 
                            }
                            else if(strncmp(message,"/makegroup", 10) == 0 &&message[10] ==' ')//Main process calls connect()
                            {
                                  char temp[4096], *token1, *next;
                                  int unique_no;
                                  strncpy(temp, message, 4096);
                                  token1 = strtok_r(temp, " ", &next);                                 
                                  make_group( dest, next);  
                            }
                            else if( strncmp(message, "PROTOCOL_FAILURE", 16)==0 )//Main process calls Protocol Failure
                            {
                                  memset(buffer,0, 4096);
                                  strncpy(buffer,"Reply From Server->One of the following things has happened\n1.you are no member of group which you are sending message to.\n2.You are trying to message without seeing active users.\n3.You have entered an invalid command.\n",strlen("Reply From Server->One of the following things has happened\n1.you are no member of group which you are sending message to.\n2.You are trying to message without seeing active users.\n3.You have entered an invalid command.\n") + 1);
                                  if((no_byte = send(dest, buffer , strlen(buffer) + 1, 0)) <= 0) // got error or connection closed by client
                                  { 
                                          if (no_byte == 0)
                                          {
                                            printf("SERVER: Connection with client  %d is closed . \n", dest);
                                            close(dest);
                                            exit(EXIT_FAILURE);
                                          }
                                          else
                                          {
                                            
                                          }

                                  }// ..............end of send() if.................................
                            }
                            else if (strncmp(message,"/quit", 5)==0)
                            {
                                 int destSocket;

                                 memset(buffer,0,4096);
                                 sprintf(buffer,"->One user has gone offline ID:%d\n",dest);
                                 for (int i = 0;i < (*no_client); i++)
                                 {
                                       destSocket = (sock_identifier + i)->socketId;
                                       if((no_byte = send(destSocket, buffer , strlen(buffer) + 1, 0)) <= 0) // got error or connection closed by client
                                       { 
                                          if (no_byte == 0)
                                          {
                                            printf("SERVER: Connection with client  %d is closed . \n", dest);
                                            close(dest);
                                            exit(EXIT_FAILURE);
                                          }
                                          else
                                          {
                                            
                                          }
                                       }// ..............end of send() if.................................
                                 }
                           }
                           else if (strncmp(message,"/broadcast", 10)==0)//Main process calls Broadcast
                           {
                                  char temp[4096], *token1, *next;
                                  strncpy(temp, message, 4096);
                                  token1 = strtok_r(temp, " ", &next);
                                  printf("%s\n", next);
                                  broadcast_message(dest, next);
                           }   
                    }
                    else
                    {       if(strncmp(message,"/sendgroup", 10) == 0)//Main process calls connect()
                            {
                                  char temp[4096], *token1, *token2, *next;
                                  int unique_no;
                                  strncpy(temp, message, 4096);
                                  token1 = strtok_r(temp, " ", &next);
                                  token2 = strtok_r(NULL, " ", &next);
                                  unique_no = atoi(token2);
                                  group_message( source, dest, next);  
                            }
                            else if(strncmp(message,"/sendfile", 9) == 0)//Main process calls connect()
                            {
                                  char temp[4096], *token1, *token2, *next;
                                  int unique_no;
                                  strncpy(temp, message, 4096);
                                  token1 = strtok_r(temp, " ", &next);
                                  token2 = strtok_r(NULL, " ", &next);
                                  unique_no = atoi(token2);
                                  //printf("BUg_point2\n");
                                  file_transfer( source, atoi(token2), message);  
                            }
                            else if(strncmp(message,"/joingroup", 10) == 0)//Main process calls connect()
                            {
                                  char temp[4096], *token1, *token2, *next;
                                  int unique_no;
                                  strncpy(temp, message, 4096);
                                  token1 = strtok_r(temp, " ", &next);
                                  token2 = strtok_r(NULL, " ", &next);
                                  unique_no = atoi(token2);
                                  join_group( source, dest);  
                            }
                            else
                            {
                                  source = (shared_buffer + (*outstanding_msg)) -> source;
                                  dest   = (shared_buffer + (*outstanding_msg)) -> dest;
                                  strncpy( message, (shared_buffer + (*outstanding_msg))-> message, strlen((shared_buffer + (*outstanding_msg))-> message) + 1 );
                                  dataTransfer(source, dest, message);  //Main application calls data transfer
                            }
                    }
                   (*outstanding_msg)--;             
              }
              (*outstanding_msg)++; 
              V(outstanding_msg_sem);
          }           
   /*.........................................*/
}




/* After receiving /quit message from client, we have to delete all the entry from all related shared memory. 
   And that is done by this function,*/
void delete_entry(int ClientId, int socketId)
{
      int position, dest, source_position, destination_position, i, j, k, l;
      char message[4096];



      /*................................deleting all group which this client is admin of................................*/
      for(position = 0; position < (*no_present_group); position++)
      {
           if((group_detail + position)->group_member[0] == ClientId)//checking where this client is admin or not.In every group 0th client is admin.
           {
                for (i = 1; i < (group_detail + position)->no_member; ++i)  // after finding one of group of which the present leaving client is admin of,
                {                                                              // we have to traverse the group member one by one, to change that group
                      int member;                                              // member, sock_identifier struct.
                      member = (group_detail + position)->group_member[i];
                      for ( j = 0; j < (*no_client); ++i)                    //Here, we searching the struct_identifier struct of the group member 
                      {
                            if((sock_identifier + j)->identifier == member)break;
                      }
                      for(int k = 0; (sock_identifier + j)->group[k] != 0; k++) // After finding, the sock_identifier struct, we traverse the group array.
                      {
                          if((sock_identifier + j)->group[k] == (group_detail + position)->group_id)break;//when we find, a match in group array,which is 
                      }                                                  //same as our group Id , we delete that entry, because we are going to delete this group
                      P(socket_identifier_lock);                           //because, remember, the admin of group is leaving.
                      for( l= k; (sock_identifier + j)->group[l + 1] != 0; l++)
                      {
                        
                        (sock_identifier + j)->group[l] = (sock_identifier + j)->group[l + 1];
                      }
                      (sock_identifier + j)->group[l] = 0;
                      V(socket_identifier_lock);
                }
                /* After changing every individual group member struct sock_identifier, Now we delete the group*/
                P(no_present_group_lock);
                for (int i = position; i < (*no_present_group); ++i)
                {
                      (group_detail + position)->group_id = (group_detail + (position + 1))->group_id;
                      (group_detail + position)->no_member = (group_detail + (position + 1))->no_member;
                      memmove(&(group_detail + position)->group_member[0],&(group_detail + (position + 1))->group_member[0], 5*sizeof(int));
                }
                (*no_present_group)--;
                V(no_present_group_lock);    
            } 
      }/*................................................................................................................................*/


      /*.....................Now, we handle all the group, where this client is member of.................................................*/ 
       /* position = storing position which struct sock_identifier is corresponding to this ClientId.
          i = storing group id in  (sock_identifier + position)->group[] array one by one, and going tothat group_detail struct, and remove this client membership.
          j = storing which group_detail is corresponds to (sock_identifier + position)->group[i].
          k = storing value from (group_detail + j)->group_member[k] , to check which is match for ClientId.When find match, we delete and update that
              that array, and decrease the (group_detail + j)->no_member.
        */
      for(position = 0; position < (*no_client); position++)
      {
            if((sock_identifier + position)->identifier == ClientId)break;
      }
      for(i = 0; (sock_identifier + position)->group[i] != 0; i++)
      {
            for(j = 0; j< (* no_present_group); j++)
            {
                if((group_detail + j)->group_id == (sock_identifier + position)->group[i])break;
            }
            if((group_detail + j)->group_member[0] == ClientId)continue;// this means this client is admin, and that is already handled.
            for(k = 0; k < (group_detail + j)->no_member; k++)
            {
              if((group_detail + j)->group_member[k] == ClientId)break;
            }
            P(no_present_group_lock);// we update struct (group_detail + j)->group_member[] i.e deleting client.
            for(l = k; k < ((group_detail + j)->no_member)-1; l++)
            {
                (group_detail +j)->group_member[l] = (group_detail +j)->group_member[l + 1];
            }
            (group_detail +j)->no_member--;
            V(no_present_group_lock);
      }
      /*..............................END OF HANDLING OF GROUP FOR THIS CLIENT...................................................*/



      /*................................deleting all group_REQ which this client is admin of................................*/
      for(position = 0; position < (*no_present_group); position++)
      {
           if((group_req_detail + position)->group_member[0] == ClientId)//checking where this client is admin or not.In every group 0th client is admin.
           {
                for (i = 1; i < (group_req_detail + position)->no_member; ++i)  // after finding one of group of which the present leaving client is admin of,
                {                                                              // we have to traverse the group member one by one, to change that group
                      int member;                                              // member, sock_identifier struct.
                      member = (group_req_detail + position)->group_member[i];
                      for ( j = 0; j < (*no_client); ++i)                    //Here, we searching the struct_identifier struct of the group member 
                      {
                            if((sock_identifier + j)->identifier == member)break;
                      }
                      for(int k = 0; (sock_identifier + j)->group_req[k] != 0; k++) // After finding, the sock_identifier struct, we traverse the group array.
                      {
                          if((sock_identifier + j)->group_req[k] == (group_req_detail + position)->group_id)break;//when we find, a match in group array,which is 
                      }                                                  //same as our group Id , we delete that entry, because we are going to delete this group
                      P(socket_identifier_lock);                           //because, remember, the admin of group is leaving.
                      for( l= k; (sock_identifier + j)->group_req[l + 1] != 0; l++)
                      {
                        
                        (sock_identifier + j)->group_req[l] = (sock_identifier + j)->group_req[l + 1];
                      }
                      (sock_identifier + j)->group_req[l] = 0;
                      V(socket_identifier_lock);
                }
                /* After changing every individual group member struct sock_identifier, Now we delete the group*/
                P(no_present_group_req_lock);
                for (int i = position; i < (*no_present_group); ++i)
                {
                      (group_req_detail + position)->group_id = (group_req_detail + (position + 1))->group_id;
                      (group_req_detail + position)->no_member = (group_req_detail + (position + 1))->no_member;
                      memmove(&(group_req_detail + position)->group_member[0],&(group_req_detail + (position + 1))->group_member[0], 5*sizeof(int));
                }
                (*no_present_group_req)--;
                V(no_present_group_lock);    
           } 
      }/*................................................................................................................................*/


      /*.....................Now, we handle all the group_REQ, where this client is member of.................................................*/ 
       /* position = storing position which struct sock_identifier is corresponding to this ClientId.
          i = storing group id in  (sock_identifier + position)->group[] array one by one, and going tothat group_req_detail struct, and remove this client membership.
          j = storing which group_req_detail is corresponds to (sock_identifier + position)->group[i].
          k = storing value from (group_req_detail + j)->group_member[k] , to check which is match for ClientId.When find match, we delete and update that
              that array, and decrease the (group_req_detail + j)->no_member.
        */
      for(position = 0; position < (*no_client); position++)
      {
            if((sock_identifier + position)->identifier == ClientId)break;
      }
      for(i = 0; (sock_identifier + position)->group[i] != 0; i++)
      {
            for(j = 0; j< (* no_present_group); j++)
            {
                if((group_req_detail + j)->group_id == (sock_identifier + position)->group[i])break;
            }
            if((group_req_detail + j)->group_member[0] == ClientId)continue;// this means this client is admin, and that is already handled.
            for(k = 0; k < (group_req_detail + j)->no_member; k++)
            {
              if((group_req_detail + j)->group_member[k] == ClientId)break;
            }
            P(no_present_group_req_lock);// we update struct (group_req_detail + j)->group_member[] i.e deleting client.
            for(l = k; k < ((group_req_detail + j)->no_member)-1; l++)
            {
                (group_req_detail +j)->group_member[l] = (group_req_detail +j)->group_member[l + 1];
            }
            (group_req_detail +j)->no_member--;
            V(no_present_group_req_lock);
      }
      /*..............................END OF HANDLING OF GROUP_REQ FOR THIS CLIENT...................................................*/



      /*.................................Deleting entry from Client Protocol..........................................*/
      for (position = 0; position < (*no_client); position++)
          if ( ((client_protocol + position) -> client_sockId) == socketId)break;

      for (int i = position ;i < (*no_client)-1;i++)
      {
           P(client_protocol_lock);
           (client_protocol+i) -> client_sockId = (client_protocol+(i+1)) -> client_sockId;
           (client_protocol+i) -> rule_one = (client_protocol+(i+1)) -> rule_one;
           (client_protocol+i) -> rule_two = (client_protocol+(i+1)) -> rule_two;
           V(client_protocol_lock); 
      }    
       /*...........,..................................................................................................*/





      /*.........................we are deleting entry from socket identifier shared memory..............................*/
      for ( i = position; i < (*no_client)-1 ; i++)
      {
             P(socket_identifier_lock);
             (sock_identifier+i) -> socketId = (sock_identifier+(i+1)) -> socketId;
             (sock_identifier+i) -> identifier = (sock_identifier+(i+1)) -> identifier;
             memmove( &(sock_identifier + i)->group[0] , &( (sock_identifier + (i + 1))->group[0] ), 10*sizeof(int));
             memmove( &(sock_identifier + i)->group_req[0] , &( (sock_identifier + (i + 1))->group_req[0] ), 10*sizeof(int));
             V(socket_identifier_lock);
      }
      /*...................................................................................................................*/





      /*...........................Deleting entry from Connection History....................................*/
      for(source_position = 0; (conn_history + source_position)->source != socketId; source_position++ )
        //if()break;
        for (int i = 0; i < 5; ++i)
        {
          dest = (conn_history + source_position)->dest1;
        }
      
      if (dest != -1)
      {
              for(destination_position = 0; destination_position < (*no_client); destination_position++)
                  if ( (conn_history + destination_position)->source == dest)break;

              if ((conn_history + destination_position)->dest1 == socketId)
              {
                      P(conn_hst_lock);
                      (conn_history + destination_position)->dest1 = -1;
                      (conn_history + destination_position)-> no_present_connection--;
                      V(conn_hst_lock);
              }
              else if ((conn_history + destination_position)->dest2 == socketId)
              {
                      P(conn_hst_lock);
                      (conn_history + destination_position)->dest2 = -1;
                      (conn_history + destination_position)-> no_present_connection--;
                      V(conn_hst_lock);
              }
              else if ((conn_history + destination_position)->dest3 == socketId)
              {
                      P(conn_hst_lock);
                      (conn_history + destination_position)->dest3 = -1;
                      (conn_history + destination_position)->no_present_connection--;
                      V(conn_hst_lock);
              }
              else if ((conn_history + destination_position)->dest4 == socketId)
              {
                      P(conn_hst_lock);
                      (conn_history + destination_position)->dest4 = -1;
                      (conn_history + destination_position)-> no_present_connection--;
                      V(conn_hst_lock);
              }

              if((conn_history + destination_position)->no_present_connection == 0)
              {
                    for (position = 0; position < (*no_client); position++)
                       if ( ((client_protocol + position) -> client_sockId) == dest) break;
                     P(client_protocol_lock);                    
                     (client_protocol + position)->rule_one = -1;
                     (client_protocol + position)->rule_two = -1;
                     V(client_protocol_lock); 
              } 
      }
      dest = (conn_history + source_position)->dest2;
      if (dest != -1)
      {
              for(destination_position = 0; destination_position < (*no_client); destination_position++)
                  {if ( (conn_history + destination_position)->source == dest)break;}
              if ((conn_history + destination_position)->dest1 == socketId)
              {
                      P(conn_hst_lock);
                      (conn_history + destination_position)->dest1 = -1;
                      (conn_history + destination_position)-> no_present_connection--;
                      V(conn_hst_lock);
              }
              else if ((conn_history + destination_position)->dest2 == socketId)
              {
                      P(conn_hst_lock);
                      (conn_history + destination_position)->dest2 = -1;
                      (conn_history + destination_position)-> no_present_connection--;
                      V(conn_hst_lock);
              }
              else if ((conn_history + destination_position)->dest3 == socketId)
              {
                      P(conn_hst_lock);
                      (conn_history + destination_position)->dest3 = -1;
                      (conn_history + destination_position)->no_present_connection--;
                      V(conn_hst_lock);
              }
              else if((conn_history + destination_position)->dest4 == socketId)
              {
                      P(conn_hst_lock);
                      (conn_history + destination_position)->dest4 = -1;
                      (conn_history + destination_position)-> no_present_connection--;
                      V(conn_hst_lock);
              }  

              if((conn_history + destination_position)->no_present_connection == 0)
              {
                    for (position = 0; position < (*no_client); position++)
                       if ( ((client_protocol + position) -> client_sockId) == dest) break;
                                         
                     P(client_protocol_lock);                    
                     (client_protocol + position)->rule_one = -1;
                     (client_protocol + position)->rule_two = -1;
                     V(client_protocol_lock); 
              }
      }
      dest = (conn_history + source_position)->dest3;
      if (dest != -1)
      {
              for(destination_position = 0; destination_position < (*no_client); destination_position++)
                  {if ( (conn_history + destination_position)->source == dest)break;}
              if ((conn_history + destination_position)->dest1 == socketId)
              {
                      P(conn_hst_lock);
                      (conn_history + destination_position)->dest1 = -1;
                      (conn_history + destination_position)-> no_present_connection--;
                      V(conn_hst_lock);
              }
              else if ((conn_history + destination_position)->dest2 == socketId)
              {
                      P(conn_hst_lock);
                      (conn_history + destination_position)->dest2 = -1;
                      (conn_history + destination_position)-> no_present_connection--;
                      V(conn_hst_lock);
              }
              else if ((conn_history + destination_position)->dest3 == socketId)
              {
                      P(conn_hst_lock);
                      (conn_history + destination_position)->dest3 = -1;
                      (conn_history + destination_position)->no_present_connection--;
                      V(conn_hst_lock);
              }
              else if((conn_history + destination_position)->dest4 == socketId)
              {
                      P(conn_hst_lock);
                      (conn_history + destination_position)->dest4 = -1;
                      (conn_history + destination_position)-> no_present_connection--;
                      V(conn_hst_lock);
              } 
              if((conn_history + destination_position)->no_present_connection == 0)
              {
                    for (position = 0; position < (*no_client); position++)
                       if ( ((client_protocol + position) -> client_sockId) == dest) break;
                                         
                     P(client_protocol_lock);                    
                     (client_protocol + position)->rule_one = -1;
                     (client_protocol + position)->rule_two = -1;
                     V(client_protocol_lock); 
              } 
      }
      dest = (conn_history + source_position)->dest4;
      if (dest != -1)
      {
              for(destination_position = 0; destination_position < (*no_client); destination_position++)
                  {if ( (conn_history + destination_position)->source == dest){break;}}
              if ((conn_history + destination_position)->dest1 == socketId)
              {
                      P(conn_hst_lock);
                      (conn_history + destination_position)->dest1 = -1;
                      (conn_history + destination_position)-> no_present_connection--;
                      V(conn_hst_lock);
              }
              else if ((conn_history + destination_position)->dest2 == socketId)
              {
                      P(conn_hst_lock);
                      (conn_history + destination_position)->dest2 = -1;
                      (conn_history + destination_position)-> no_present_connection--;
                      V(conn_hst_lock);
              }
              else if ((conn_history + destination_position)->dest3 == socketId)
              {
                      P(conn_hst_lock);
                      (conn_history + destination_position)->dest3 = -1;
                      (conn_history + destination_position)->no_present_connection--;
                      V(conn_hst_lock);
              }
              else if((conn_history + destination_position)->dest4 == socketId)
              {
                      P(conn_hst_lock);
                      (conn_history + destination_position)->dest4 = -1;
                      (conn_history + destination_position)-> no_present_connection--;
                      V(conn_hst_lock);
              } 

              if((conn_history + destination_position)->no_present_connection == 0)
              {
                    for (position = 0; ((client_protocol + position) -> client_sockId) != dest; position++)
                       //if ( () break;
                    for (int i = 0; i < 5; ++i)
                    {
                       P(client_protocol_lock);                    
                       (client_protocol + position)->rule_one = -1;
                       (client_protocol + position)->rule_two = -1;
                       V(client_protocol_lock); 
                    }                     
                     
              } 
      }
       for (int i = source_position ; i < 4; i++)
       {
           P(conn_hst_lock);
           (conn_history + i) -> source = (conn_history+(i+1)) -> source;
           (conn_history+i) -> dest1 = (conn_history+(i+1)) -> dest1;
           (conn_history+i) -> dest2 = (conn_history+(i+1)) -> dest2;
           (conn_history+i) -> dest3 = (conn_history+(i+1)) -> dest3;
           (conn_history+i) -> dest4 = (conn_history+(i+1)) -> dest4;
           V(conn_hst_lock);
       }
       /*............................................*/

   
   

      /*....Closing the connection...*/ 
       P(no_clients_sem);
       (*no_client)--;
       V(no_clients_sem);
       close(socketId);
      /*...........................*/ 


      /*...Writing appropriate message in shared memory...*/
       memset(message, 0, 4096);
      sprintf(message,"/quit");
      P(outstanding_msg_sem);
      (shared_buffer+(*outstanding_msg))->dest = ClientId;
      (shared_buffer+(*outstanding_msg))->source = -1; 
      strncpy((shared_buffer+(*outstanding_msg))->message, message, strlen(message) + 1);
      (*outstanding_msg)++;
      V(outstanding_msg_sem);
      exit(EXIT_SUCCESS);     
}
 /*........................................................................................................*/








void handle_client(int ClientId, int socketId)   //This module handles each of the clients separately
{
      char buffer[4096], message[4096], temp[4096];
      int position, client_protocol_position, no_byte, dest;

      for(;;)
      {

              /*...........................Initializing memory...................................*/
              memset(buffer, 0, 4096);
              memset(message, 0, 4096);
              memset(temp, 0, 4096);
              if( (no_byte =recv(socketId, buffer, 4096, 0)) > 0)
              {
                    for(position = 0; position < (*no_client); position++)// get the position of this client in the client_protocol shared memory 
                    {
                        if((client_protocol + position)->client_sockId == socketId)break; 
                    }//............end of for loop of position checking.....................


                    char *str, *token1, *next;
                    strncpy(temp, buffer, 4096);
                    str = temp;
                    token1 = strtok_r(str, " ", &next);

                    if((strncmp(buffer, "/quit", 5 ) == 0) && (strlen(buffer) == 5))break;

                    if((client_protocol + position)->rule_one == -1) // client has not seen online user list yet
                    {

                          
                          if(strncmp(buffer, "/activegroups", strlen("/activegroups") ) == 0 && strlen(buffer) == 13)
                          {
                                strncpy(message,"/activegroups", strlen("/activegroups") + 1);

                                P(outstanding_msg_sem);
                                (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                strncpy((shared_buffer+(*outstanding_msg)) ->message, message, strlen(message) + 1);
                                (*outstanding_msg)++;
                                V(outstanding_msg_sem);           
                          }//.........end of comparison of /active command if..............................
                          else if(strncmp(buffer, "/activeallgroups", strlen("/activeallgroups") ) == 0 && strlen(buffer) == 16)
                          {
                                strncpy(message,"/activeallgroups", strlen("/activeallgroups") + 1);

                                P(outstanding_msg_sem);
                                (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                strncpy((shared_buffer+(*outstanding_msg)) ->message, message, strlen(message) + 1);
                                (*outstanding_msg)++;
                                V(outstanding_msg_sem);           
                          }//.........end of comparison of /active command if..............................
                          else if(strncmp(buffer, "/active", strlen("/active") ) == 0 && strlen(buffer) == 7)
                          {
                                strncpy(message,"/active", strlen("/active") + 1);

                                P(outstanding_msg_sem);
                                (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                strncpy((shared_buffer+(*outstanding_msg)) ->message, message, strlen(message) + 1);
                                (*outstanding_msg)++;
                                V(outstanding_msg_sem);           
                          }//.........end of comparison of /active command if..............................
                          else if(strncmp(buffer, "/sendgroup", strlen("/sendgroup")) == 0 && strlen(buffer) > 11)  //Handling the CONNECT command
                          {
                              
                                char *token1, *token2, *next, *str;
                                int okFlag = 0, group_type = 0;
                                strncpy(temp, buffer, 4096);
                                str = temp;
                                token1 = strtok_r(str, " ", &next);
                                token2 = strtok_r(NULL, " ", &next);

                                /* we have to check 
                                   1. that 2nd string is int or not (overflow)
                                   2, 3rd string empty or not
                                   3, 2nd string length is 5 or not
                                */   
                                if (isInt(token2) && strlen(next) && (strlen(token2) == 4 || strlen(token2) == 6))
                                {
                                      if(strlen(token2) == 4)
                                      {
                                        for(position = 0; position < (*no_present_group); position++)
                                          if((group_detail + position)->group_id == atoi(token2))
                                            {
                                              dest = (group_detail + position)->group_id;
                                              group_type = 0;
                                              break;
                                            }  
                                      }
                                      else if(strlen(token2) == 6)
                                      {
                                        for(position = 0; position < (*no_present_group_req); position++)
                                          if((group_req_detail + position)->group_id == atoi(token2))
                                            {
                                              group_type = 1;
                                              dest = (group_req_detail + position)->group_id;
                                              break;
                                            }  
                                      }

                                      if ( group_type == 0)
                                      {
                                            for(int i = 0; i < (group_detail + position)->no_member; i++)
                                            {
                                                if((group_detail + position)->group_member[i] == ClientId)
                                                  okFlag =1;
                                                break;
                                            }
                                      }
                                      else if(group_type == 1)
                                      {
                                            for(int i = 0; i < (group_req_detail + position)->no_member; i++)
                                            {
                                                if((group_req_detail + position)->group_member[i] == ClientId)
                                                  okFlag =1;
                                                break;
                                            }
                                      }

                                      

                                      if(okFlag ==1)
                                      {
                                      	//printf("i am here1\n");
                                            /*....................Updating the required table................................*/
                                            P(outstanding_msg_sem);
                                            (shared_buffer + (*outstanding_msg)) -> dest = dest;
                                            (shared_buffer + (*outstanding_msg)) -> source = socketId; 
                                            strncpy((shared_buffer + (*outstanding_msg)) ->message, buffer, 4096);
                                            (*outstanding_msg)++;
                                            V(outstanding_msg_sem); 
                                      }
                                       
                                }//.................end of isInt if..............................
                                else  //Handling the invalid command
                                {
                                        strncpy(message,"PROTOCOL_FAILURE", strlen("PROTOCOL_FAILURE") + 1);
                                        P(outstanding_msg_sem);
                                        (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                        (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                        strncpy((shared_buffer + (*outstanding_msg)) ->message, message, strlen(message) + 1);
                                        (*outstanding_msg)++;
                                        V(outstanding_msg_sem);
                                }
                          } //.......................end of comparison /sendgroup else if........................//  
                          else if(strncmp(buffer, "/joingroup", strlen("/joingroup")) == 0 && strlen(buffer) > 11)  //Handling the CONNECT command
	                        {
    	                          
    	                          char *token1, *token2, *next, *str;
    	                          int okFlag = 0, group_type = 0;
    	                          strncpy(temp, buffer, 4096);
    	                          str = temp;
    	                          token1 = strtok_r(str, " ", &next);
    	                          token2 = strtok_r(NULL, " ", &next);

    	                          /* we have to check 
    	                             1. that 2nd string is int or not (overflow)
    	                             2, 3rd string empty or not
    	                             3, 2nd string length is 5 or not
    	                          */   
    	                          if (isInt(token2) && !strlen(next) && (strlen(token2) == 4 || strlen(token2) == 6))
    	                          {
    	                                if(strlen(token2) == 4)
    	                                {
    	                                  for(position = 0; position < (*no_present_group); position++)
    	                                    if((group_detail + position)->group_id == atoi(token2))
    	                                      {
    	                                        dest = (group_detail + position)->group_id;
    	                                        group_type = 0;
    	                                        break;
    	                                      }  
    	                                }
    	                                else if(strlen(token2) == 6)
    	                                {
    	                                  for(position = 0; position < (*no_present_group_req); position++)
    	                                    if((group_req_detail + position)->group_id == atoi(token2))
    	                                      {
    	                                        group_type = 1;
    	                                        dest = (group_req_detail + position)->group_id;
    	                                        break;
    	                                      }  
    	                                }

    	                                if ( group_type == 0)
    	                                {
    	                                      for(int i = 0; i < (group_detail + position)->no_member; i++)
    	                                      {
    	                                          if((group_detail + position)->group_member[i] != ClientId)
    	                                            okFlag =1;
    	                                          break;
    	                                      }
    	                                }
    	                                else if(group_type == 1)
    	                                {
    	                                      for(int i = 0; i < (group_req_detail + position)->no_member; i++)
    	                                      {
    	                                          if((group_req_detail + position)->group_member[i] != ClientId)
    	                                            okFlag =1;
    	                                          break;
    	                                      }
    	                                }

    	                                

    	                                if(okFlag ==1)
    	                                {
    	                                	//printf("i am here1\n");
    	                                      /*....................Updating the required table................................*/
    	                                      P(outstanding_msg_sem);
    	                                      (shared_buffer + (*outstanding_msg)) -> dest = dest;
    	                                      (shared_buffer + (*outstanding_msg)) -> source = socketId; 
    	                                      strncpy((shared_buffer + (*outstanding_msg)) ->message, buffer, 4096);
    	                                      (*outstanding_msg)++;
    	                                      V(outstanding_msg_sem); 
    	                                }
    	                                 
    	                          }//.................end of isInt if..............................
                                else  //Handling the invalid command
                                {
                                        strncpy(message,"PROTOCOL_FAILURE", strlen("PROTOCOL_FAILURE") + 1);
                                        P(outstanding_msg_sem);
                                        (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                        (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                        strncpy((shared_buffer + (*outstanding_msg)) ->message, message, strlen(message) + 1);
                                        (*outstanding_msg)++;
                                        V(outstanding_msg_sem);
                                }
                          } //.......................end of comparison /joingroup else if........................//  
                          else if( strncmp(buffer,"/broadcast", strlen("/broadcast")) == 0 && strlen(buffer)>11 && strlen(token1) ==10 ) //Handling BROADCASTING
                          {
                         
                                P(outstanding_msg_sem);
                                (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                strncpy((shared_buffer+(*outstanding_msg)) ->message, buffer, strlen(buffer) + 1);
                                (*outstanding_msg)++;
                                V(outstanding_msg_sem);        
                              
                          }//.........end of comparison of broadcast command else if..............................
                          else      //Handling the user error
                          {
                          
                                strncpy(message,"PROTOCOL_FAILURE", strlen("PROTOCOL_FAILURE") + 1);
                                P(outstanding_msg_sem);
                                (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                strncpy((shared_buffer+(*outstanding_msg)) ->message, message, strlen(message) + 1);
                                (*outstanding_msg)++;
                                V(outstanding_msg_sem);            
                          }//.........end of comparison of protocol failure command else..............................

                    }//...........................................end of checking rule no1 if........................    
                    else if ( (client_protocol + position)->rule_two == -1) //The client knows the users but is not connected yet
                    {
                          
                          
                          if(strncmp(buffer, "/accept", strlen("/accept")) == 0 && strlen(buffer) > 8 && strlen(token1)==7)  
                          {
                                  
                                  char *token1, *token2, *next, *str;
                                  strncpy(temp, buffer, 4096);
                                  str = temp;
                                  token1 = strtok_r(str, " ", &next);
                                  token2 = strtok_r(NULL, " ", &next);

                                  /* we have to check 
                                     1. that 2nd string is int or not (overflow)
                                     2, 3rd string empty or not
                                     3, 2nd string length is 5 or not
                                  */   
                                  if (isInt(token2) && isInt(next)&& strlen(next)==6 && (strlen(token2) == 5))
                                  {
                                 
                                        strncpy(message, buffer, 4096);

                                        /*....................Updating the required table................................*/
                                        P(outstanding_msg_sem);
                                        (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                        (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                        strncpy((shared_buffer + (*outstanding_msg)) ->message, message, 4096);
                                        (*outstanding_msg)++;
                                        V(outstanding_msg_sem);  
                                  }//.................end of isInt if..............................
                                  else  //Handling the invalid command
                                  {
                                        strncpy(message,"PROTOCOL_FAILURE", strlen("PROTOCOL_FAILURE") + 1);
                                        P(outstanding_msg_sem);
                                        (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                        (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                        strncpy((shared_buffer + (*outstanding_msg)) ->message, message, strlen(message) + 1);
                                        (*outstanding_msg)++;
                                        V(outstanding_msg_sem);
                                  }//...................end of isInt else..............................

                          } //.......................end of comparison /accept if........................
                          else if(strncmp(buffer, "/activegroups", strlen("/activegroups") ) == 0 && strlen(buffer) == 13)
                          {
                                strncpy(message,"/activegroups", strlen("/activegroups") + 1);

                                P(outstanding_msg_sem);
                                (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                strncpy((shared_buffer+(*outstanding_msg)) ->message, message, strlen(message) + 1);
                                (*outstanding_msg)++;
                                V(outstanding_msg_sem);           
                          }//.........end of comparison of /active command if..............................
                          else if(strncmp(buffer, "/activeallgroups", strlen("/activeallgroups") ) == 0 && strlen(buffer) == 16)
                          {
                                strncpy(message,"/activeallgroups", strlen("/activeallgroups") + 1);

                                P(outstanding_msg_sem);
                                (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                strncpy((shared_buffer+(*outstanding_msg)) ->message, message, strlen(message) + 1);
                                (*outstanding_msg)++;
                                V(outstanding_msg_sem);           
                          }//.........end of comparison of /active command if..............................
                          else if( strncmp(buffer, "/active", strlen("/active")) == 0 && strlen(buffer) == 7)    //Handling /active Command 
                          {
                    
                                strncpy(message,"/active", strlen("/active") + 1);

                                P(outstanding_msg_sem);
                                (shared_buffer + (*outstanding_msg))-> dest = socketId;
                                (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                strncpy((shared_buffer+(*outstanding_msg)) ->message, message, strlen(message) + 1);
                               
                                (*outstanding_msg)++;
                                V(outstanding_msg_sem);           
                          } //.........end of comparison of /active command else if..............................
                          else if(strncmp(buffer, "/sendgroup", strlen("/sendgroup")) == 0 && strlen(buffer) > 11)  //Handling the CONNECT command
                          {
                              
                                char *token1, *token2, *next, *str;
                                int okFlag = 0, group_type = 0;
                                strncpy(temp, buffer, 4096);
                                str = temp;
                                token1 = strtok_r(str, " ", &next);
                                token2 = strtok_r(NULL, " ", &next);

                                /* we have to check 
                                   1. that 2nd string is int or not (overflow)
                                   2, 3rd string empty or not
                                   3, 2nd string length is 5 or not
                                */   
                                if (isInt(token2) && strlen(next) && (strlen(token2) == 4 || strlen(token2) == 6))
                                {
                                      if(strlen(token2) == 4)
                                      {
                                        for(position = 0; position < (*no_present_group); position++)
                                          if((group_detail + position)->group_id == atoi(token2))
                                            {
                                              dest = (group_detail + position)->group_id;
                                              group_type = 0;
                                              break;
                                            }  
                                      }
                                      else if(strlen(token2) == 6)
                                      {
                                        for(position = 0; position < (*no_present_group_req); position++)
                                          if((group_req_detail + position)->group_id == atoi(token2))
                                            {
                                              group_type = 1;
                                              dest = (group_req_detail + position)->group_id;
                                              break;
                                            }  
                                      }

                                      if ( group_type == 0)
                                      {
                                            for(int i = 0; i < (group_detail + position)->no_member; i++)
                                            {
                                                if((group_detail + position)->group_member[i] == ClientId)
                                                  okFlag =1;
                                                break;
                                            }
                                      }
                                      else if(group_type == 1)
                                      {
                                            for(int i = 0; i < (group_req_detail + position)->no_member; i++)
                                            {
                                                if((group_req_detail + position)->group_member[i] == ClientId)
                                                  okFlag =1;
                                                break;
                                            }
                                      }


                                      

                                      if(okFlag ==1)
                                      {
                                            /*....................Updating the required table................................*/
                                            P(outstanding_msg_sem);
                                            (shared_buffer + (*outstanding_msg)) -> dest = dest;
                                            (shared_buffer + (*outstanding_msg)) -> source = socketId;
                                            strncpy((shared_buffer + (*outstanding_msg)) ->message, buffer, 4096);
                                            (*outstanding_msg)++;
                                            V(outstanding_msg_sem); 
                                      }
                                       
                                }//.................end of isInt if..............................
                                else  //Handling the invalid command
                                {
                                        strncpy(message,"PROTOCOL_FAILURE", strlen("PROTOCOL_FAILURE") + 1);
                                        P(outstanding_msg_sem);
                                        (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                        (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                        strncpy((shared_buffer + (*outstanding_msg)) ->message, message, strlen(message) + 1);
                                        (*outstanding_msg)++;
                                        V(outstanding_msg_sem);
                                }
                          } //.......................end of comparison /sendgroup else if........................//
                          else if(strncmp(buffer, "/sendfile", strlen("/sendfile")) == 0 && strlen(buffer) > 10)  //Handling the CONNECT command
                          {//printf("BUg_point3\n");
                              
                                char *token1, *token2, *next, *str;
                                int okFlag = 0, group_type = 0;
                                strncpy(temp, buffer, 4096);
                                str = temp;
                                token1 = strtok_r(str, " ", &next);
                                token2 = strtok_r(NULL, " ", &next);

                                /* we have to check 
                                   1. that 2nd string is int or not (overflow)
                                   2, 3rd string empty or not
                                   3, 2nd string length is 5 or not
                                */   
                                if (isInt(token2) && strlen(next) && (strlen(token2) == 4 || strlen(token2) == 6 || strlen(token2) == 5))
                                {
                                      if(strlen(token2) == 4)
                                      {
                                        for(position = 0; position < (*no_present_group); position++)
                                          if((group_detail + position)->group_id == atoi(token2))
                                            {
                                              dest = (group_detail + position)->group_id;
                                              group_type = 0;
                                              break;
                                            }  
                                      }
                                      else if(strlen(token2) == 6)
                                      {
                                        for(position = 0; position < (*no_present_group_req); position++)
                                          if((group_req_detail + position)->group_id == atoi(token2))
                                            {
                                              group_type = 1;
                                              dest = (group_req_detail + position)->group_id;
                                              break;
                                            }  
                                      }
                                      else if(strlen(token2) == 5)
                                      {
                                        for(position = 0; position < (*no_client); position++)
                                          if((sock_identifier + position)->identifier == atoi(token2))
                                            {
                                              group_type = 2;
                                              dest = (sock_identifier + position)->socketId;
                                              break;
                                            }  
                                      }

                                      if ( group_type == 0)
                                      {
                                            for(int i = 0; i < (group_detail + position)->no_member; i++)
                                            {
                                                if((group_detail + position)->group_member[i] == ClientId)
                                                  okFlag =1;
                                                break;
                                            }
                                      }
                                      else if(group_type == 1)
                                      {
                                            for(int i = 0; i < (group_req_detail + position)->no_member; i++)
                                            {
                                                if((group_req_detail + position)->group_member[i] == ClientId)
                                                  okFlag =1;
                                                break;
                                            }
                                      }
                                      else
                                      {
                                        okFlag =1;
                                      }

                                      

                                      if(okFlag ==1)
                                      {
                                            /*....................Updating the required table................................*/
                                            P(outstanding_msg_sem);
                                            (shared_buffer + (*outstanding_msg)) -> dest = dest;
                                            (shared_buffer + (*outstanding_msg)) -> source = socketId; 
                                            //printf("BUg_point4\n");
                                            strncpy((shared_buffer + (*outstanding_msg)) ->message, buffer, 4096);
                                            (*outstanding_msg)++;
                                            V(outstanding_msg_sem); 
                                      }
                                       
                                }//.................end of isInt if..............................
                                else  //Handling the invalid command
                                {
                                        strncpy(message,"PROTOCOL_FAILURE", strlen("PROTOCOL_FAILURE") + 1);
                                        P(outstanding_msg_sem);
                                        (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                        (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                        strncpy((shared_buffer + (*outstanding_msg)) ->message, message, strlen(message) + 1);
                                        (*outstanding_msg)++;
                                        V(outstanding_msg_sem);
                                }
                          } //.......................end of comparison /sendgroup else if........................//  
                          else if(strncmp(buffer, "/joingroup", strlen("/joingroup")) == 0 && strlen(buffer) > 11)  //Handling the CONNECT command
                          {
                            
                                char *token1, *token2, *next, *str;
                                int okFlag = 0, group_type = 0;
                                strncpy(temp, buffer, 4096);
                                str = temp;
                                token1 = strtok_r(str, " ", &next);
                                token2 = strtok_r(NULL, " ", &next);

                                /* we have to check 
                                   1. that 2nd string is int or not (overflow)
                                   2, 3rd string empty or not
                                   3, 2nd string length is 5 or not
                                */   
                                if (isInt(token2) && !strlen(next) && (strlen(token2) == 4 || strlen(token2) == 6))
                                {
                                      if(strlen(token2) == 4)
                                      {
                                        for(position = 0; position < (*no_present_group); position++)
                                          if((group_detail + position)->group_id == atoi(token2))
                                            {
                                              dest = (group_detail + position)->group_id;
                                              group_type = 0;
                                              break;
                                            }  
                                      }
                                      else if(strlen(token2) == 6)
                                      {
                                        for(position = 0; position < (*no_present_group_req); position++)
                                          if((group_req_detail + position)->group_id == atoi(token2))
                                            {
                                              group_type = 1;
                                              dest = (group_req_detail + position)->group_id;
                                              break;
                                            }  
                                      }

                                      if ( group_type == 0)
                                      {
                                            for(int i = 0; i < (group_detail + position)->no_member; i++)
                                            {
                                                if((group_detail + position)->group_member[i] != ClientId)
                                                  okFlag =1;
                                                break;
                                            }
                                      }
                                      else if(group_type == 1)
                                      {
                                            for(int i = 0; i < (group_req_detail + position)->no_member; i++)
                                            {
                                                if((group_req_detail + position)->group_member[i] != ClientId)
                                                  okFlag =1;
                                                break;
                                            }
                                      }

                                      

                                      if(okFlag ==1)
                                      {
                                        //printf("i am here1\n");
                                            /*....................Updating the required table................................*/
                                            P(outstanding_msg_sem);
                                            (shared_buffer + (*outstanding_msg)) -> dest = dest;
                                            (shared_buffer + (*outstanding_msg)) -> source = socketId; 
                                            strncpy((shared_buffer + (*outstanding_msg)) ->message, buffer, 4096);
                                            (*outstanding_msg)++;
                                            V(outstanding_msg_sem); 
                                      }
                                       
                                }//.................end of isInt if..............................
                                else  //Handling the invalid command
                                {
                                        strncpy(message,"PROTOCOL_FAILURE", strlen("PROTOCOL_FAILURE") + 1);
                                        P(outstanding_msg_sem);
                                        (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                        (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                        strncpy((shared_buffer + (*outstanding_msg)) ->message, message, strlen(message) + 1);
                                        (*outstanding_msg)++;
                                        V(outstanding_msg_sem);
                                }
                          } //.......................end of comparison /joingroup else if........................//
                          else if(strncmp(buffer, "/send", strlen("/send")) == 0 && strlen(buffer) > 6 && strlen(token1)==5)  //Handling the CONNECT command
                          {
                              		
                                  char *token1, *token2, *next, *str;
                                  strncpy(temp, buffer, 4096);
                                  str = temp;
                                  token1 = strtok_r(str, " ", &next);
                                  token2 = strtok_r(NULL, " ", &next);

                                  /* we have to check 
                                     1. that 2nd string is int or not (overflow)
                                     2, 3rd string empty or not
                                     3, 2nd string length is 5 or not
                                  */   
                                  if (isInt(token2) && strlen(next) && (strlen(token2) == 5))
                                  {
                                 
                                        strncpy(message, buffer, 4096);

                                        /*....................Updating the required table................................*/
                                        P(outstanding_msg_sem);
                                        (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                        (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                        strncpy((shared_buffer + (*outstanding_msg)) ->message, message, 4096);
                                        (*outstanding_msg)++;
                                        V(outstanding_msg_sem);  
                                  }//.................end of isInt if..............................
                                  else  //Handling the invalid command
                                  {
                                        strncpy(message,"PROTOCOL_FAILURE", strlen("PROTOCOL_FAILURE") + 1);
                                        P(outstanding_msg_sem);
                                        (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                        (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                        strncpy((shared_buffer + (*outstanding_msg)) ->message, message, strlen(message) + 1);
                                        (*outstanding_msg)++;
                                        V(outstanding_msg_sem);
                                  }//...................end of isInt else..............................

                          } //.......................end of comparison /send if........................
                          else if( strncmp(buffer,"/broadcast", strlen("/broadcast")) == 0 && strlen(buffer)>11 && strlen(token1) == 10) //Handling BROADCASTING
                          {
                             
                                    P(outstanding_msg_sem);
                                    (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                    (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                    strncpy((shared_buffer+(*outstanding_msg)) ->message, buffer, strlen(buffer) + 1);
                                    (*outstanding_msg)++;
                                    V(outstanding_msg_sem);        
                                  
                          }//.........end of comparison of broadcast command else if..............................
                          else if( strncmp(buffer, "/makegroupreq", 13) ==0 && strlen(buffer) > 14 && strlen(token1) == 13)
                          {
                            P(outstanding_msg_sem);
                            (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                            (shared_buffer + (*outstanding_msg)) -> source = -1; 
                            sprintf((shared_buffer + (*outstanding_msg))->message, "%s", buffer);
                            (*outstanding_msg)++;
                            V(outstanding_msg_sem);
                          }//.......................end of comparison of /makegroupreq else if block.......................
                          else if( strncmp(buffer, "/makegroup", 10) ==0 && strlen(buffer) > 11 && strlen(token1) == 10)
                          {
                            P(outstanding_msg_sem);
                            (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                            (shared_buffer + (*outstanding_msg)) -> source = -1; 
                            sprintf((shared_buffer + (*outstanding_msg))->message, "%s", buffer);
                            (*outstanding_msg)++;
                            V(outstanding_msg_sem);
                          }//.......................end of comparison of /makegroup else if block.......................
                          else  //Handling user exception
                          {
                                    strncpy(message,"PROTOCOL_FAILURE", strlen("PROTOCOL_FAILURE") + 1);
                                    P(outstanding_msg_sem);
                                    (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                    (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                    strncpy((shared_buffer + (*outstanding_msg)) ->message, message, strlen(message) + 1);
                                    (*outstanding_msg)++;
                                    V(outstanding_msg_sem);
                          }//.........end of comparison of /send else..............................       

                  }// ..............end of comparison of rule no2 if else block......................... 
                  else   //Handling normal message transfer
                  {   
                          int dest;
                          if(strncmp(buffer, "/accept", strlen("/accept")) == 0 && strlen(buffer) > 8 && strlen(token1)==7)  
                          {
                                  
                                  char *token1, *token2, *next, *str;
                                  strncpy(temp, buffer, 4096);
                                  str = temp;
                                  token1 = strtok_r(str, " ", &next);
                                  token2 = strtok_r(NULL, " ", &next);

                                  /* we have to check 
                                     1. that 2nd string is int or not (overflow)
                                     2, 3rd string empty or not
                                     3, 2nd string length is 5 or not
                                  */   
                                  if (isInt(token2) && isInt(next)&& strlen(next)==6 && (strlen(token2) == 5))
                                  {
                                 
                                        strncpy(message, buffer, 4096);

                                        /*....................Updating the required table................................*/
                                        P(outstanding_msg_sem);
                                        (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                        (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                        strncpy((shared_buffer + (*outstanding_msg)) ->message, message, 4096);
                                        (*outstanding_msg)++;
                                        V(outstanding_msg_sem);  
                                  }//.................end of isInt if..............................
                                  else  //Handling the invalid command
                                  {
                                        strncpy(message,"PROTOCOL_FAILURE", strlen("PROTOCOL_FAILURE") + 1);
                                        P(outstanding_msg_sem);
                                        (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                        (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                        strncpy((shared_buffer + (*outstanding_msg)) ->message, message, strlen(message) + 1);
                                        (*outstanding_msg)++;
                                        V(outstanding_msg_sem);
                                  }//...................end of isInt else..............................

                          } //.......................end of comparison /accept if........................
                          else if(strncmp(buffer, "/activegroups", strlen("/activegroups") ) == 0 && strlen(buffer) == 13)
                          {
                                strncpy(message,"/activegroups", strlen("/activegroups") + 1);

                                P(outstanding_msg_sem);
                                (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                strncpy((shared_buffer+(*outstanding_msg)) ->message, message, strlen(message) + 1);
                                (*outstanding_msg)++;
                                V(outstanding_msg_sem);           
                          }//.........end of comparison of /active command if..............................
                          else if(strncmp(buffer, "/activeallgroups", strlen("/activeallgroups") ) == 0 && strlen(buffer) == 16)
                          {
                                strncpy(message,"/activeallgroups", strlen("/activeallgroups") + 1);

                                P(outstanding_msg_sem);
                                (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                strncpy((shared_buffer+(*outstanding_msg)) ->message, message, strlen(message) + 1);
                                (*outstanding_msg)++;
                                V(outstanding_msg_sem);           
                          }//.........end of comparison of /active command if.............................. 
                          else if( strncmp(buffer, "/active", strlen("/active")) == 0 && strlen(buffer) == 7)    //Handling /active Command 
                          {
                        
                                    strncpy(message,"/active", strlen("/active") + 1);

                                    P(outstanding_msg_sem);
                                    (shared_buffer + (*outstanding_msg))-> dest = socketId;
                                    (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                    strncpy((shared_buffer+(*outstanding_msg)) ->message, message, strlen(message) + 1);
                                   
                                    (*outstanding_msg)++;
                                    V(outstanding_msg_sem);           
                          } //.........end of comparison of /active command else if.............................. 
                          else if(strncmp(buffer, "/sendgroup", strlen("/sendgroup")) == 0 && strlen(buffer) > 11)  //Handling the CONNECT command
                          { 
                      
                                    char *token1, *token2, *next, *str;
                                    int okFlag = 0, group_type = 0;
                                    strncpy(temp, buffer, 4096);
                                    str = temp;
                                    token1 = strtok_r(str, " ", &next);
                                    token2 = strtok_r(NULL, " ", &next);

                                    /* we have to check 
                                       1. that 2nd string is int or not (overflow)
                                       2, 3rd string empty or not
                                       3, 2nd string length is 5 or not
                                    */   
                                    if (isInt(token2) && strlen(next) && (strlen(token2) == 4 || strlen(token2) == 6))
                                    {
                                          if(strlen(token2) == 4)
                                          {
                                            for(position = 0; position < (*no_present_group); position++)
                                              if((group_detail + position)->group_id == atoi(token2))
                                                {
                                                  dest = (group_detail + position)->group_id;
                                                  group_type = 0;
                                                  break;
                                                }  
                                          }
                                          else if(strlen(token2) == 6)
                                          {
                                            for(position = 0; position < (*no_present_group_req); position++)
                                              if((group_req_detail + position)->group_id == atoi(token2))
                                                {
                                                  group_type = 1;
                                                  dest = (group_req_detail + position)->group_id;
                                                  break;
                                                }  
                                          }

                                          if ( group_type == 0)
                                          {
                                                for(int i = 0; i < (group_detail + position)->no_member; i++)
                                                {
                                                    if((group_detail + position)->group_member[i] == ClientId)
                                                      okFlag =1;
                                                    break;
                                                }
                                          }
                                          else if(group_type == 1)
                                          {
                                                for(int i = 0; i < (group_req_detail + position)->no_member; i++)
                                                {
                                                    if((group_req_detail + position)->group_member[i] == ClientId)
                                                      okFlag =1;
                                                    break;
                                                }
                                          }

                                          

                                          if(okFlag ==1)
                                          {
                                                /*....................Updating the required table................................*/
                                                P(outstanding_msg_sem);
                                                (shared_buffer + (*outstanding_msg)) -> dest = dest;
                                                (shared_buffer + (*outstanding_msg)) -> source = socketId; 
                                                strncpy((shared_buffer + (*outstanding_msg)) ->message, buffer, 4096);
                                                (*outstanding_msg)++;
                                                V(outstanding_msg_sem); 
                                          }
                                           
                                    }//.................end of isInt if..............................
                                    else  //Handling the invalid command
                                    {
                                            strncpy(message,"PROTOCOL_FAILURE", strlen("PROTOCOL_FAILURE") + 1);
                                            P(outstanding_msg_sem);
                                            (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                            (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                            strncpy((shared_buffer + (*outstanding_msg)) ->message, message, strlen(message) + 1);
                                            (*outstanding_msg)++;
                                            V(outstanding_msg_sem);
                                    }
                          } //.......................end of comparison /sendgroup else if........................// 
                          else if(strncmp(buffer, "/sendfile", strlen("/sendfile")) == 0 && strlen(buffer) > 10)  //Handling the CONNECT command
                          {
                              
                                char *token1, *token2, *next, *str;
                                int okFlag = 0, group_type = 0;
                                strncpy(temp, buffer, 4096);
                                str = temp;
                                token1 = strtok_r(str, " ", &next);
                                token2 = strtok_r(NULL, " ", &next);

                                /* we have to check 
                                   1. that 2nd string is int or not (overflow)
                                   2, 3rd string empty or not
                                   3, 2nd string length is 5 or not
                                */   
                                if (isInt(token2) && strlen(next) && (strlen(token2) == 4 || strlen(token2) == 6 || strlen(token2) == 5))
                                {
                                      if(strlen(token2) == 4)
                                      {
                                        for(position = 0; position < (*no_present_group); position++)
                                          if((group_detail + position)->group_id == atoi(token2))
                                            {
                                              dest = (group_detail + position)->group_id;
                                              group_type = 0;
                                              break;
                                            }  
                                      }
                                      else if(strlen(token2) == 6)
                                      {
                                        for(position = 0; position < (*no_present_group_req); position++)
                                          if((group_req_detail + position)->group_id == atoi(token2))
                                            {
                                              group_type = 1;
                                              dest = (group_req_detail + position)->group_id;
                                              break;
                                            }  
                                      }
                                      else if(strlen(token2) == 5)
                                      {
                                        for(position = 0; position < (*no_client); position++)
                                          if((sock_identifier + position)->identifier == atoi(token2))
                                            {
                                              group_type = 2;
                                              dest = (sock_identifier + position)->socketId;
                                              break;
                                            }  
                                      }

                                      if ( group_type == 0)
                                      {
                                            for(int i = 0; i < (group_detail + position)->no_member; i++)
                                            {
                                                if((group_detail + position)->group_member[i] == ClientId)
                                                  okFlag =1;
                                                break;
                                            }
                                      }
                                      else if(group_type == 1)
                                      {
                                            for(int i = 0; i < (group_req_detail + position)->no_member; i++)
                                            {
                                                if((group_req_detail + position)->group_member[i] == ClientId)
                                                  okFlag =1;
                                                break;
                                            }
                                      }

                                      

                                      if(okFlag ==1)
                                      {
                                            /*....................Updating the required table................................*/
                                            P(outstanding_msg_sem);
                                            (shared_buffer + (*outstanding_msg)) -> dest = dest;
                                            (shared_buffer + (*outstanding_msg)) -> source = socketId; 
                                            
                                            strncpy((shared_buffer + (*outstanding_msg)) ->message, buffer, 4096);
                                            (*outstanding_msg)++;
                                            V(outstanding_msg_sem); 
                                      }
                                       
                                }//.................end of isInt if..............................
                                else  //Handling the invalid command
                                {
                                        strncpy(message,"PROTOCOL_FAILURE", strlen("PROTOCOL_FAILURE") + 1);
                                        P(outstanding_msg_sem);
                                        (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                        (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                        strncpy((shared_buffer + (*outstanding_msg)) ->message, message, strlen(message) + 1);
                                        (*outstanding_msg)++;
                                        V(outstanding_msg_sem);
                                }
                          } //.......................end of comparison /sendgroup else if........................//  
                          else if(strncmp(buffer, "/joingroup", strlen("/joingroup")) == 0 && strlen(buffer) > 11)  //Handling the CONNECT command
                          {
                            
                                  char *token1, *token2, *next, *str;
                                  int okFlag = 0, group_type = 0;
                                  strncpy(temp, buffer, 4096);
                                  str = temp;
                                  token1 = strtok_r(str, " ", &next);
                                  token2 = strtok_r(NULL, " ", &next);

                                  /* we have to check 
                                     1. that 2nd string is int or not (overflow)
                                     2, 3rd string empty or not
                                     3, 2nd string length is 5 or not
                                  */   
                                  if (isInt(token2) && !strlen(next) && (strlen(token2) == 4 || strlen(token2) == 6))
                                  {
                                        if(strlen(token2) == 4)
                                        {
                                          for(position = 0; position < (*no_present_group); position++)
                                            if((group_detail + position)->group_id == atoi(token2))
                                              {
                                                dest = (group_detail + position)->group_id;
                                                group_type = 0;
                                                break;
                                              }  
                                        }
                                        else if(strlen(token2) == 6)
                                        {
                                          for(position = 0; position < (*no_present_group_req); position++)
                                            if((group_req_detail + position)->group_id == atoi(token2))
                                              {
                                                group_type = 1;
                                                dest = (group_req_detail + position)->group_id;
                                                break;
                                              }  
                                        }

                                        if ( group_type == 0)
                                        {
                                              for(int i = 0; i < (group_detail + position)->no_member; i++)
                                              {
                                                  if((group_detail + position)->group_member[i] != ClientId)
                                                    okFlag =1;
                                                  break;
                                              }
                                        }
                                        else if(group_type == 1)
                                        {
                                              for(int i = 0; i < (group_req_detail + position)->no_member; i++)
                                              {
                                                  if((group_req_detail + position)->group_member[i] != ClientId)
                                                    okFlag =1;
                                                  break;
                                              }
                                        }

                                        

                                        if(okFlag ==1)
                                        {
                                          //printf("i am here1\n");
                                              /*....................Updating the required table................................*/
                                              P(outstanding_msg_sem);
                                              (shared_buffer + (*outstanding_msg)) -> dest = dest;
                                              (shared_buffer + (*outstanding_msg)) -> source = socketId; 
                                              strncpy((shared_buffer + (*outstanding_msg)) ->message, buffer, 4096);
                                              (*outstanding_msg)++;
                                              V(outstanding_msg_sem); 
                                        }
                                         
                                  }//.................end of isInt if..............................
                                  else  //Handling the invalid command
                                  {
                                          strncpy(message,"PROTOCOL_FAILURE", strlen("PROTOCOL_FAILURE") + 1);
                                          P(outstanding_msg_sem);
                                          (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                          (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                          strncpy((shared_buffer + (*outstanding_msg)) ->message, message, strlen(message) + 1);
                                          (*outstanding_msg)++;
                                          V(outstanding_msg_sem);
                                  }
                          } //.......................end of comparison /joingroup else if........................//  
                          else if(strncmp(buffer, "/send", strlen("/send")) == 0 && strlen(buffer) > 6)  //Handling the CONNECT command
                          {
                                    char *token1, *token2, *next, *str;
                                    strncpy(temp, buffer, 4096);
                                    str = temp;
                                    token1 = strtok_r(str, " ", &next);
                                    token2 = strtok_r(NULL, " ", &next);
                                    
                                    /* we have to check 
                                       1. that 2nd string is int or not (overflow)
                                       2, 3rd string empty or not
                                       3, 2nd string length is 5 or not
                                    */   
                                    if (isInt(token2) && strlen(next) && (strlen(token2) == 5))
                                    {

                                          for(position = 0; position < (*no_client); position++)
                                            if((conn_history + position)->source == socketId)break;

                                          for(int j = 0; j < (*no_client); j++)
                                                if ((sock_identifier + j)->identifier == atoi(token2))
                                                    dest = (sock_identifier + j)-> socketId; 

                                          if( ((conn_history + position)->dest1 == dest) || ((conn_history + position)->dest2 == dest) || ((conn_history + position)->dest3 == dest) || ((conn_history + position)->dest4 == dest))    
                                          {

                                                /*....................Updating the required table................................*/
                                                P(outstanding_msg_sem);
                                                (shared_buffer + (*outstanding_msg)) -> dest = dest;
                                                (shared_buffer + (*outstanding_msg)) -> source = socketId; 
                                                strncpy((shared_buffer + (*outstanding_msg)) ->message, next, 4096);
                                                (*outstanding_msg)++;
                                                V(outstanding_msg_sem); 
                                          }     
                                     }//.................end of isInt if..............................
                                     else  //Handling the invalid command
                                  {
                                        strncpy(message,"PROTOCOL_FAILURE", strlen("PROTOCOL_FAILURE") + 1);
                                        P(outstanding_msg_sem);
                                        (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                        (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                        strncpy((shared_buffer + (*outstanding_msg)) ->message, message, strlen(message) + 1);
                                        (*outstanding_msg)++;
                                        V(outstanding_msg_sem);
                                  }//...................end of isInt else..............................
                          } 
                          else if(strncmp(buffer,"/broadcast", strlen("/broadcast")) == 0 && strlen(buffer)>11 ) //Handling BROADCASTING
                          {
                                  P(outstanding_msg_sem);
                                  (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                  (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                  strncpy((shared_buffer + (*outstanding_msg)) ->message, buffer, strlen(buffer) + 1);
                                  (*outstanding_msg)++;
                                  V(outstanding_msg_sem);        
                                    
                          }//.........end of comparison of broadcast command if..............................
                          
                          else if( strncmp(buffer, "/makegroupreq", 13) ==0 && strlen(buffer) > 14 && strlen(token1) == 13)
                          {
                                  P(outstanding_msg_sem);
                                  (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                  (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                  sprintf((shared_buffer + (*outstanding_msg))->message, "%s", buffer);
                                  (*outstanding_msg)++;
                                  V(outstanding_msg_sem);
                          }//.......................end of comparison of /makegroupreq else if block.......................
                          else if( strncmp(buffer, "/makegroup", 10) ==0 && strlen(buffer) > 11 && strlen(token1) == 10)
                          {
                                  P(outstanding_msg_sem);
                                  (shared_buffer + (*outstanding_msg)) -> dest = socketId;
                                  (shared_buffer + (*outstanding_msg)) -> source = -1; 
                                  sprintf((shared_buffer + (*outstanding_msg))->message, "%s", buffer);
                                  (*outstanding_msg)++;
                                  V(outstanding_msg_sem);
                          }//.......................end of comparison of /makegroup else if block.......................
	               		      else  //Handling the invalid command
	                	      {
        	                        strncpy(message,"PROTOCOL_FAILURE", strlen("PROTOCOL_FAILURE") + 1);
        	                        P(outstanding_msg_sem);
        	                        (shared_buffer + (*outstanding_msg)) -> dest = socketId;
        	                        (shared_buffer + (*outstanding_msg)) -> source = -1; 
        	                        strncpy((shared_buffer + (*outstanding_msg)) ->message, message, strlen(message) + 1);
        	                        (*outstanding_msg)++;
        	                        V(outstanding_msg_sem);
	                	      }
                  } //..................end of Handling normal message transfer 

              }//...................................end of recv() if block............ 
              else
              { 
                  if (no_byte == 0)
                  {
                          printf("SERVER: Connection with client %d is closed. \n", ClientId);
                          close(socketId);
                          exit(EXIT_FAILURE);

                  }
                  else
                  {
                          
                  }

              }//................................ end of recv() else................................

        
    	} // ..................END OF MAIN FOR LOOP................................... 



      /**********USER HAS KEYED IN "/quit"****************/
      delete_entry(ClientId, socketId);
      
        
        exit(EXIT_SUCCESS);  //Client is leaving    
}//............................................................END OF FUNTION .................................................................







int main()
{
	int random_number; // this is used to hold unique number of connection
	int processId; 
	int source, dest; // this two socket descriptor will be used for message transfer.
	
	/*...................................................................................*/


	char display_user1[4096]; //This will contain rules
  	char display_user2[4096]; //This will contain rules
  	/*..............................................................................*/
	
	struct addrinfo hints, *ref, *p;  // these are for getaddrinfo()
	int status;                      // To hold return value of getaddrinfo()
	/*........................................................*/

	int flags; // to use in fcntl
	int listener;         // This is the socket descriptor, server will be listening on
	int new_fd;			  // Newly accep()ed socket descriptor
	struct sockaddr_storage client_addr; // client address will be saved in this struct
	socklen_t addr_len;      // client address length will be saved in it.
	char client_IP[INET6_ADDRSTRLEN]; //inet_ntop will save client ip address in here
	/*..........................................................*/

	char buffer[4096];    //For messages from the clients
	char message[4096];  // this will be used when client limit will be exceeded
	ssize_t no_byte;      // this will hold return value of read(), write()
	int yes = 1;          // for setsockop()
	/*.........................................................................*/

	struct sigaction for_sigint, for_sigchild; // This is the struct, we have to pass to sigaction()

	 
	time_t client_time;



  /*.............................Semaphores....................................*/ 
 	client_protocol_lock = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
	socket_identifier_lock = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
	conn_hst_lock = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
	outstanding_msg_sem = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
	no_clients_sem = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
  group_lock = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
  group_req_lock = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
  no_present_group_lock = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
  no_present_group_req_lock = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
	/*.........................................................*/


	// ..............setting the value of semaphores..........
	semctl(client_protocol_lock, 0, SETVAL, 1);
	semctl(socket_identifier_lock, 0, SETVAL, 1);
	semctl(conn_hst_lock, 0, SETVAL, 1);
	semctl(outstanding_msg_sem, 0, SETVAL, 1);
	semctl(no_clients_sem, 0, SETVAL, 1);
  semctl(group_lock, 0, SETVAL, 1);
  semctl(group_req_lock, 0, SETVAL, 1);
  semctl(no_present_group_lock, 0, SETVAL, 1);
  semctl(no_present_group_req_lock, 0, SETVAL, 1);
	/*..........................................................*/


	/*  We now initialize the sembufs pop and vop so that pop is used
	    for P(semid) and vop is used for V(semid).  Going
	    by the semantics of the P and V operations, we see that
	    pop.sem_op should be -1 and vop.sem_op should be 1.
	*/
	pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1 ;
  /*..............................................................................................*/



  /*.......................we acquire shared memory segment here...............................................*/
  shm_id1 = shmget(IPC_PRIVATE, 4096 * sizeof (SHARED_MEMORY), 0777|IPC_CREAT);
  shm_id2 = shmget(IPC_PRIVATE, 4096 * sizeof (CLIENT_PROTOCOL), 0777|IPC_CREAT);
  shm_id3 = shmget(IPC_PRIVATE, sizeof(int), 0777|IPC_CREAT);
  shm_id4 = shmget(IPC_PRIVATE, sizeof(int), 0777|IPC_CREAT);
  shm_id5 = shmget(IPC_PRIVATE, 20 * sizeof(CONNECTION_HISTORY), 0777|IPC_CREAT);
  shm_id6 = shmget(IPC_PRIVATE, 5 * sizeof(CONNECTION_IDENTIFIER), 0777|IPC_CREAT); 
  shm_id7 = shmget(IPC_PRIVATE, 10 * sizeof(GROUP), 0777|IPC_CREAT); 
  shm_id8 = shmget(IPC_PRIVATE, 10 * sizeof(GROUP_REQ), 0777|IPC_CREAT); 
  shm_id9 = shmget(IPC_PRIVATE, sizeof(int), 0777|IPC_CREAT); 
  shm_id10 = shmget(IPC_PRIVATE, sizeof(int), 0777|IPC_CREAT);  


  /*.........we are attching shared memory with its local data segment.....................................*/
  shared_buffer = (SHARED_MEMORY *) shmat(shm_id1, 0, 0);
  client_protocol = (CLIENT_PROTOCOL *) shmat(shm_id2, 0, 0);
  no_client = (int *) shmat(shm_id3, 0, 0);
  outstanding_msg = (int *) shmat(shm_id4, 0, 0);
  conn_history = (CONNECTION_HISTORY *) shmat(shm_id5, 0, 0);
  sock_identifier = (CONNECTION_IDENTIFIER *) shmat(shm_id6, 0, 0);
  group_detail = (GROUP *)shmat(shm_id7, 0, 0);
  group_req_detail = (GROUP_REQ *)shmat(shm_id8, 0, 0);
  no_present_group = (int *)shmat(shm_id9, 0, 0);
  no_present_group_req = (int *)shmat(shm_id10, 0, 0);
  /*.................................................................................................*/

  (*no_client) = (*outstanding_msg)  = (* no_present_group) = (* no_present_group_req) = 0;


	/*............get us a socket and bind it..........................*/
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	// getaddrinfo() return a pointer to a linked list of addrinfo structure 
	if((status = getaddrinfo(NULL, PORT, &hints, &ref)) != 0)
	{
		fprintf(stderr, "SERVER(getaddrinfo): %s\n", gai_strerror(status));
		exit(EXIT_FAILURE);
	}





	// ................Now server will be traversing the return linked list by getaddrinfo() one by ony until server get a successful socket and binding.........
	for ( p = ref; p != NULL; p = p->ai_next)
	{
    	 	if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
    	 	{
    	 		continue;
    	 	}/*......end of socket() if.........*/



    	 	// To avoid "address already in use " error message
    	 	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0)
    	 	{
    	 		perror("SERVER(setsockopt): ");
    	 		exit(EXIT_FAILURE);
    	 	}/*.............end of setsockopt() if......*/




    	 	 /*....Setting the socket in non-blocking mode...*/
    		if ((flags = fcntl(listener, F_GETFL, 0)) < 0) 
    		{ 
        		perror("SERVER(flags = fcntl):");
        		continue;
    		} 

    		if (fcntl(listener, F_SETFL, flags | O_NONBLOCK) < 0) 
    		{ 
    			perror("SERVER( fcntl):");
        		continue;
    		} 
    		/*..............................................*/	




    	 	if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
    	 	{
    	 		close(listener);
    	 		continue;	
    	 	}/*...........end of bind() if.........*/




    	 	break;//...........We get successful socket and binding, so server does not need to traverse the linked list anymor...........	


	} /*.....................end of for loop for traversing the linked list returned by getaddrinfo()................................*/







	 /*.........If we do not get successful binding ..................*/
	if (p == NULL)
	{
	 	perror("SERVER: we failed to get successfull socket or geting bound\n");
	 	exit(EXIT_FAILURE);
	}	
	/*................................................................*/
	 
	freeaddrinfo(ref);// We are done with it.




  /*...............................Here we are handling SIGINT signal............................*/
  for_sigint.sa_handler = sigint_handler;
  sigemptyset(&for_sigint.sa_mask);
  for_sigint.sa_flags = 0;


  if (sigaction(SIGINT, &for_sigint, NULL) < 0)
  {
      perror("SERVER(sigaction): ");
      exit(EXIT_FAILURE);
  } 
  /*.........................................................................................*/



	/*..........................................Here we are handling sigchild signal.................*/
	for_sigchild.sa_handler = sigchild_handler;
	for_sigchild.sa_flags = SA_RESTART;
	sigemptyset(&for_sigchild.sa_mask);

	if(sigaction(SIGCHLD, &for_sigchild, NULL) < 0)
	{
	  	perror("SERVER(sigaction-sigchild):");
	  	exit(EXIT_FAILURE);
	}
	/*...........................................................................................*/









	/*...................................we are listening here....................................*/
	if (listen(listener, 10) < 0)
	{
	  	perror("SERVER(listen):");
	  	exit(EXIT_FAILURE);
	}/*...............................................................................*/
  printf("SERVER: we are waiting for connections....................................\n");








	/*...................................MAIN FOR LOOP................................*/

	for(;;) //this loop will run in infinite loop for reading from shared memory
	{



      	/*.............................................we are accepting here.........................*/

        addr_len = sizeof client_addr;
      	if ((new_fd = accept(listener, (struct sockaddr *)&client_addr, &addr_len)) == -1)
      	{
      		//Nope, we are not doing anything anything here
      	}//............end of accept() if block........................................
      	else if (*no_client < 5)
      	{
              inet_ntop(client_addr.ss_family, convert((struct sockaddr *)&client_addr), client_IP, INET6_ADDRSTRLEN);
              printf("SERVER: we got connection from %s \n",client_IP );
              




          		random_number = get_rand_num();

          		/*...........................Making entry into required tables....................................*/

              // Updating socket identifier table here .
        			P(socket_identifier_lock);
              (sock_identifier + (*no_client))->socketId = new_fd;
              (sock_identifier + (*no_client))->identifier = random_number;
              V(socket_identifier_lock);


              // Updating connection history table here.
              P(conn_hst_lock);
              (conn_history + (*no_client))-> no_present_connection = 0;
              (conn_history + (*no_client))-> source = new_fd;
              (conn_history + (*no_client))-> dest1 = -1;
              (conn_history + (*no_client))-> dest2 = -1;
              (conn_history + (*no_client))-> dest3 = -1;
              (conn_history + (*no_client))-> dest4 = -1;
              V(conn_hst_lock);


              // Updating client protocol table here.
              P(client_protocol_lock);
              (client_protocol + (*no_client))->client_sockId = new_fd;
              (client_protocol + (*no_client))->rule_one = -1; 
              (client_protocol + (*no_client))->rule_two = -1;
              V(client_protocol_lock);
              /*..........................................*/


              //Updating no_client here.
              P(no_clients_sem);
              (*no_client)++;
              V(no_clients_sem);

              
              
              memset(buffer, 0, 4096);
              sprintf(buffer, "Reply From Server:\n->Welcome to CHAT BOX\n->Please note that your ID No.:%d",random_number);
              //strncat(buffer, display_user1, strlen(display_user1));

                /*...Sending the welcome message to the client....*/
        		  if((no_byte = send(new_fd, buffer , strlen(buffer) + 1, 0)) <= 0) // got error or connection closed by client
         			{ 
             				if (no_byte == 0)
             				{
             					printf("SERVER: Connection with client %s is closed : Process Id %d \n", client_IP, getpid());
             					close(new_fd);
                      continue;

             				}
             				else
             				{
             					
                      continue;
         				    }

         	    }// ..............end of send() if.................................
              else
              {
              	if ( (processId = fork()) == 0)   //Child processed will be created for each of the clients
                {
                  handle_client(random_number, new_fd);
                  exit(EXIT_SUCCESS);          
                }     
              }//..........end of send() else block......................         
        }//..........end of else if() block of checking of whether client no less than 5 or not
        else  // Handling the connection limit error
      	{
          		memset(message, 0, 4096);
          		strncpy(message,"Reply From Server->Connection Limit Exceeded!!", strlen("Reply From Server->Connection Limit Exceeded!!") + 1);
            	if((no_byte = send(new_fd, message , strlen(message) + 1, 0)) <= 0) // got error or connection closed by client
         			{ 
             				if (no_byte == 0)
             				{
             					printf("SERVER: Connection with client %s is already down. \n",client_IP);
             					close(new_fd);
                      continue;

             				}
             				else
             				{
             					
                      continue;
             				}

         			}// ..............end of send() if.................................
              else
              {
              	close(new_fd);
               
              }//..........end of send() else block...................... 

        }//..........end of else  block of checking of whether client no less than 5 or not
        write_handler();
        	
  }





}