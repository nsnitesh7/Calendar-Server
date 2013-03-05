#include <stdio.h>				//Standard Input / Output Library
#include <stdlib.h>				//Standard General Utilities Library.
#include <unistd.h>
#include <string.h>
#include <sys/types.h>			//The header file contains definitions of a number of data types used in system calls
#include <sys/socket.h>			//The header file socket.h includes a number of definitions of structures needed for sockets.
#include <netinet/in.h>			//The header file in.h contains constants and structures needed for internet domain addresses.
#include <netdb.h>				//This header make available in_port_t and the type in_addr_t as defined in the description of netinet/in.h 
#include <iostream>				//Standard Input / Output Streams Library
#include <pthread.h>			//The header file contains threads related inbuilt routine like pthread_create, pthread_mutex_lock 
#include <time.h>				//This header file contains definitions of functions to get and manipulate date and time information.
#define add 1
#define remove 2
#define update 3
#define get_start 4
#define get 5
#define getall 6
using namespace std;						//using std namespace

//structure defining the contents of each message.
struct message
{
	char name[10];
	int type;
	char date[7];
	char start_time[5];
	char end_time[5];
	char type_of_event[20];
	message *next;							//next pointer poiting to next message in linkedlist
};

struct tm tm;								//time.h has this tm structure
message *head;								//head of central linked list stored (central data structure)
message typesix[100][100];					//used in getall functionality for select part and multithreading part

//This function is called when a system call fails. It displays a message about the error on stderr and then aborts the program.
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

//function used for debuging (printing contents of message)
void print_buffer(message *buffer)
{
	cout <<  buffer->name<<endl;
	cout <<  buffer->type<<endl;
	cout <<  buffer->date<<endl;
	cout <<  buffer->start_time<<endl;
	cout <<  buffer->end_time<<endl;
	cout <<  buffer->type_of_event<<endl;
}

//function checking inputed date fomat is valid or not
bool check_date(char ch[10])
{
	if (!strptime(ch, "%m%d%y", &tm))		//this will check the char string 'ch' is in "%m%d%y" format or not
	{
		cout <<"Syntax error : Date should be in MMDDYY format.\n";
		exit(0);
	}

	return true;
}

//function checking inputed time fomat is valid or not
bool check_time(char ch[10])
{
	if(strlen(ch)!=4)
		return false;
	int time=atoi(ch);
	int minutes=time%100;
	int hour=time/100;
	
	if ( hour>=24 || minutes>=60)
	{
		cout <<"Syntax error : Time should be in HHMM format.\n";
		exit(0);
	}

	return true;
}

//function return true if the event given in input has to be expired or not (helping function of remove_expired_events function)
bool expire(message *temp)
{
	char ch[13];
	time_t timer;
	ch[0]=temp->date[0];
	ch[1]=temp->date[1];
	ch[2]=temp->date[2];
	ch[3]=temp->date[3];
	ch[4]='2';
	ch[5]='0';
	ch[6]=temp->date[4];
	ch[7]=temp->date[5];
	ch[8]=temp->end_time[0];
	ch[9]=temp->end_time[1];
	ch[10]=temp->end_time[2];
	ch[11]=temp->end_time[3];
	ch[12]='\0';
	char temp1[3]={temp->date[4],temp->date[5],'\0'};
	char temp2[3];
	struct tm * timeinfo;

	timer = time(NULL);					//getting current time
	timeinfo = localtime (&timer);		//getting the current time in 'tm' time structure

	sprintf(temp2,"%d",(timeinfo->tm_year)%100);

	strptime(ch, "%m%d%Y%H%M", &tm);

	if(mktime(&tm)==-1)
	{
		if(strcmp(temp1,temp2)>0)
			return false;
	}

	if(difftime(timer,mktime(&tm))>=0)	//if event time is smaller that current time then true is returned
		return true;
	return false;
}

//function will remove all the expired events
void remove_expired_events()
{
	message *traverse=head;
	message *previous;
	while(traverse!=NULL)
	{
		if(expire(traverse))
		{
			if(traverse==head)
			{
				head=head->next;
				traverse=head;
			}
			else
			{
				previous->next=traverse->next;
				traverse=traverse->next;
			}
		}
		else
		{
			previous=traverse;
			traverse=traverse->next;
		}
	}
}

int i,n;
pthread_mutex_t mutexsum;		//mutex variable
pthread_t pt[1000];				//1000 threads are created
message *traverse_part;			//pointer to a message (used in type functioning routine)
message *previous;
message *current=NULL;
message *traverse;
message *temp=new message;

//the function that every created thread will perform
void *handler_function(void *newsockfd)
{
	pthread_mutex_lock (&mutexsum);		//aquiring the lock
	
	message *buffer=new message;
	n = read(*((int *)newsockfd),buffer,sizeof(message));		//receiving message from client on which one of inputed functions has to be done
	if (n < 0)
		error("ERROR reading from socket");
	
	//print_buffer(buffer);
	
	if(buffer->type==1)		//handling add functionality.
	{
		traverse=head;
		while(traverse!=NULL)
		{
			if(!strcmp(traverse->date,buffer->date) && !strcmp(traverse->name,buffer->name))
			{
				if((atoi(traverse->start_time)<=atoi(buffer->start_time) && atoi(traverse->end_time)>=atoi(buffer->start_time))||(atoi(traverse->start_time)<=atoi(buffer->end_time) && atoi(traverse->end_time)>=atoi(buffer->end_time)))
				{
					n = write(*(int *)(newsockfd),"Conflict detected!",18);	//sending message to client in case of conflict
					if (n < 0)
						error("ERROR writing to socket");
					break;
				}
			}
			current=traverse;
			traverse=traverse->next;
		}
		if(traverse==NULL)
		{
			if(head!=NULL)
			{
				message *newmessage=new message;
				newmessage=buffer;
				current->next=newmessage;
				current=current->next;
			}
			else
			{
				head=buffer;
				current=buffer;
			}
			n = write(*(int *)(newsockfd),"Event Added!!",13);				//sending message to client if the inputed event is added
			if (n < 0)
				error("ERROR writing to socket");
		}
	}
	else if(buffer->type==2)		//handling remove functionality
	{
		bool flag=0;
		traverse=head;
		while(traverse!=NULL)
		{
			if(!strcmp(traverse->date,buffer->date) && !strcmp(traverse->name,buffer->name) && (atoi(traverse->start_time)==atoi(buffer->start_time)))
			{
					if(traverse==head)
					{
						head=head->next;
						traverse=head;
						n = write(*(int *)(newsockfd),"Event Removed!!",15);			//writing to a file descriptor
						if (n < 0)
							error("ERROR writing to socket");
						break;
					}
					else
					{
						current->next=traverse->next;
						traverse=traverse->next;
						n = write(*(int *)(newsockfd),"Event Removed!!",15);			//writing to a file descriptor
						if (n < 0)
							error("ERROR writing to socket");
						flag=1;
						break;
					}
			}
			else
			{
				current=traverse;
				traverse=traverse->next;
			}
		}
		if(traverse==NULL && !flag)
		{
			n = write(*(int *)(newsockfd),"None of event removed!",22);				 //writing to a file descriptor
			if (n < 0)
				error("ERROR writing to socket");
		}
	}
	else if(buffer->type==3)		//handling update functionality.
	{
		traverse=head;
		while(traverse!=NULL)
		{
			if(!strcmp(traverse->date,buffer->date) && !strcmp(traverse->name,buffer->name))
			{
				if(atoi(traverse->start_time)==atoi(buffer->start_time))
				{
					break;
				}
			}
			previous=traverse;
			traverse=traverse->next;
		}
		if(traverse==NULL)
		{
			n = write(*(int *)(newsockfd),"None of event updated!",22);		//sending message to client if the inputed event is not found
			if (n < 0)
				error("ERROR writing to socket");
		}
		else
		{
			traverse_part=head;
			while(traverse_part!=NULL)
			{
				if(traverse_part==traverse)
				{
					traverse_part=traverse_part->next;
					continue;
				}
				if(!strcmp(traverse_part->date,buffer->date) && !strcmp(traverse_part->name,buffer->name))
				{
					if((atoi(traverse_part->start_time)<=atoi(buffer->start_time) && atoi(traverse_part->end_time)>=atoi(buffer->start_time))||(atoi(traverse_part->start_time)<=atoi(buffer->end_time) && atoi(traverse_part->end_time)>=atoi(buffer->end_time)))
					{
						n = write(*(int *)(newsockfd),"Conflict detected!",18);	//sending message to client in case of conflict
						if (n < 0)
							error("ERROR writing to socket");
						n = write(*(int *)(newsockfd),"None of event updated!",22);
						if (n < 0)
							error("ERROR writing to socket");
						break;
					}
				}
				traverse_part=traverse_part->next;
			}
			
			if(traverse_part==NULL)
			{
				message *newmessage=new message;
				newmessage=buffer;
				if(traverse==head)
				{
					newmessage->next=head->next;
					head=newmessage;
				}
				else if(head!=NULL)
				{
					previous->next=newmessage;
					newmessage->next=traverse->next;
				}
				n = write(*(int *)(newsockfd),"Event Updated!!",15);		//sending message to client if the inputed event is updated
				if (n < 0)
					error("ERROR writing to socket");
			}
		}
	}
	else if(buffer->type==4)	//handling get functionality of type 4 (type explained in client.c flie).
	{
		traverse=head;
		while(traverse!=NULL)
		{
			if(!strcmp(traverse->date,buffer->date) && !strcmp(traverse->name,buffer->name))
			{
				if(atoi(traverse->start_time)==atoi(buffer->start_time))
				{
					n = write(*(int *)(newsockfd),traverse,sizeof(message));//sending message corresponding to a user, a date and the time inputed
					if (n < 0)
						error("ERROR writing to socket");
					break;
				}
			}
			traverse=traverse->next;
		}
		strcpy(temp->date,"252525");
		n = write(*(int *)(newsockfd),temp,sizeof(message));	//sending the flag message
		if (n < 0)
			error("ERROR writing to socket");
	}
	else if(buffer->type==5)	//handling get functionality of type 5 (type explained in client.c flie).
	{
		traverse=head;
		while(traverse!=NULL)
		{
			if(!strcmp(traverse->date,buffer->date) && !strcmp(traverse->name,buffer->name))
			{
				//p();
				n = write(*(int *)(newsockfd),traverse,sizeof(message));	//sending messages corresponding to a user and a date inputed.
				if (n < 0)
					error("ERROR writing to socket");
			}
			traverse=traverse->next;
		}
		strcpy(temp->date,"252525");
		n = write(*(int *)(newsockfd),temp,sizeof(message));	//sending the flag message
		if (n < 0)
			error("ERROR writing to socket");
	}
	else if(buffer->type==6)		//handling getall functionality.
	{
		i=*(int *)(newsockfd);
		traverse=head;

		int count=0;
		while(traverse!=NULL)
		{
			if(!strcmp(traverse->name,buffer->name))
			{
				typesix[i][count++]=*traverse;		//storing the messages that is to be sent to client side in getall query from client.
			}
			traverse=traverse->next;
		}

		temp->type=count;
		n = write(i,temp,sizeof(message));			//sending the total entries in getall query that is to be sent to client
		if (n < 0)
			error("ERROR writing to socket");

		for(int g=0;g<count;g++)					
		{
			n = read(i,temp,sizeof(message));					//reading the entry number that is to be sent
			if (n < 0)
				error("ERROR reading from socket");

			int num=atoi(temp->name);
			n = write(i,&typesix[i][num],sizeof(message));		//sending the requested entry to client side.
			if (n < 0)
				error("ERROR writing to socket");
		}
	}
	pthread_mutex_unlock (&mutexsum);			//releasing the acquired lock
	void *h=NULL;
	return h;
}
