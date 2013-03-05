#include "../definitions.h"

int main(int argc, char *argv[])
{
	pthread_mutex_init(&mutexsum, NULL);		//initialising mutexsum
	
	head=NULL;
	
	int sockfd, newsockfd, portno;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;		//standard variables for client-server communication
	
	if (argc < 2)
	{
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");
	
	memset((char *) &serv_addr,0,sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;											//serv_addr structure will contain sever's full address
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	
	if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) //standard binding function to bind to a port
		error("ERROR on binding");
	
	listen(sockfd,5);														//standard listening function which starts listening on that port
	clilen = sizeof(cli_addr);
	int counter=0;
	while(1)
	{
		newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);	//accepting a connection made to the open port
		if (newsockfd < 0) 
			error("ERROR on accept");
		
		remove_expired_events();											//removing expired events
		
		n=pthread_create(&pt[counter++],NULL,handler_function,&newsockfd);	//creating thread corresponding to client request/query
		if (n != 0)
			cout << "error\n" ;
	}
	close(sockfd);					//closing the opened socket
return 0; 
}
