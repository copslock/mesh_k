/*

File         : appServer.c
Status       : Current
Description  : 

Author       : haopeng
Contact      : 376915244@qq.com

Description  : Primary released

## Please log your description here for your modication ##

Revision     : 
Modifier     : 
Description  : 
*/

#include <stdio.h>
#include <string.h>   
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> 
#include <unistd.h>   
#include <arpa/inet.h>    
#include <sys/types.h>
#include <errno.h>
#include <signal.h>


#include "appServer.h"

void handle_pipe(int sig)
{
	printf("catch pipe\n");
}

 int main(int argc , char *argv[])
{
    
    struct sockaddr_in address;
    int master_socket , addrlen ;
    int new_socket , client_socket[MAX_CLIENT] , max_clients = MAX_CLIENT ;
    int max_sd,activity, i , valread , sd;
    int opt = TRUE;
	int ret = 0;
    char buffer[4096];  
	char guest_mac[32] = {0};
      
    /*set of socket descriptors*/
    fd_set readfds;

	struct sigaction action;
	
	action.sa_handler = handle_pipe;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	sigaction(SIGPIPE, &action, NULL);

	printf("MESH APP start......\n");
	if (!apmib_init()) {
		printf("Initialize AP MIB failed !\n");
		return -1;
	}

	app_init();

    char *message = "accepted client connection request \r\n"; 
    for (i = 0; i < max_clients; i++) 
    {
        client_socket[i] = 0;
    }
      
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
  
    /*set master socket to allow multiple connections*/
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
  
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( LISTEN_PORT );
      
    /*bind the socket to localhost port 5000*/
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n",LISTEN_PORT);
     
    if (listen(master_socket, PENDING_CONNECTION_NUM) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
      
    addrlen = sizeof(address);
    printf("Waiting for incomming app client connections ...");
    while(TRUE) 
    {   
        struct timeval time_interval;
  		time_interval.tv_sec=TIME_INTERVAL;
      	time_interval.tv_usec=0;
		
        FD_ZERO(&readfds);
        FD_SET(master_socket, &readfds); 
        max_sd = master_socket;
         
        /*add child sockets to set,if valid socket descriptor then add to read list*/
        for ( i = 0 ; i < max_clients ; i++) 
        {
            sd = client_socket[i];
            if(sd > 0)
                FD_SET( sd , &readfds);            
            if(sd > max_sd)
                max_sd = sd;
        }
  
        /*
         *wait for an activity on one of the sockets , timeout is TIME_INTERVAL, 
         *which can be setted in appServer.h
        */
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
    
        if ((activity < 0) && (errno!=EINTR)) 
        {
            printf("select error");
        }
          
        /*If something happened on the master socket , then its an incoming connection*/
        if (FD_ISSET(master_socket, &readfds)) 
        {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                printf("accept failed\n");
                exit(EXIT_FAILURE);
            }
          
            /*inform user of socket number - used in app_event_handler()*/
            printf("New app client connection , socket fd is %d , ip is : %s \n" , new_socket , inet_ntoa(address.sin_addr) );			
			
			ret = refuse_guest_sta(inet_ntoa(address.sin_addr));
			if (1 == ret)
			{
				close(new_socket);
				printf("the sta(%s) is guest client, can't login APP\n", inet_ntoa(address.sin_addr));
				continue;
			}
			
            /*add new socket to array of sockets*/
            for (i = 0; i < max_clients; i++) 
            {
                if( client_socket[i] == 0 )
                {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n" , i);                    
                    break;
                }
            }
        }
          
        for (i = 0; i < max_clients; i++) 
        {
            sd = client_socket[i];
              
            if (FD_ISSET( sd , &readfds)) 
            {
                /*
                 *Check if it was for closing , and also read the incoming message
                 *if Some client disconnected , get his details and print,
			     *Close the socket and mark as 0 in list for reuse
				*/
				valread = read( sd , buffer, 4096);
                if ( valread == 0)
                {
                    //getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
                    //printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                    close( sd );
                    client_socket[i] = 0;
                }
                else
                {
					/*set the string terminating NULL byte on the end of the data read*/
					buffer[valread] = '\0';

					/*address the event from app client*/ 
					app_event_handler(sd, buffer);
					
                }
				 
            }
        }
    }
      
    return 0;
} 

