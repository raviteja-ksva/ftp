/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

void get_request(int);
void put_request(int);

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	get_request(sockfd);
	// put_request(sockfd);
	close(sockfd);
	printf("socket closed\n");
	exit(1);
	return 0;
}

void put_request(int sockfd)
{
	int numbytes, i;
	long filesize;
	size_t result;
	char buffer[MAXDATASIZE];
	char *request= (char *)"put hello3.txt";

	printf("sending put request: %s\n", request);
	printf(" lenght = %d\n", strlen(request));
	if (send(sockfd, request, strlen(request), 0) == -1)
        perror("send");

    FILE * filefd;
    if ( (filefd = fopen("recv.txt", "r")) == NULL)
    {
        fprintf(stderr, "Could not open destination file.\n");
		exit(0);
    }
    // obtain file size:
  	fseek (filefd , 0 , SEEK_END);
  	filesize = ftell (filefd);
  	rewind (filefd);
  	printf("The size of file is %ld\n", filesize);

  	for(i = 0; i < filesize - MAXDATASIZE; i = i + MAXDATASIZE)
  	{
  		result = fread(buffer, 1 , MAXDATASIZE, filefd);
	  	if (result != MAXDATASIZE) {
	  		fprintf(stderr, "Reading error\n");
	  		exit (0);
	  	}
	  	if (send(sockfd, buffer, MAXDATASIZE, 0) == -1)
			perror("send");
		printf("File sending...\n");					
  	}
  	result = fread(buffer, 1, filesize - i, filefd);
  	if (result != filesize - i)
  	{
  		fprintf(stderr, "Reading error2 \n");
  		exit (0);
  	}
  	if (send(sockfd, buffer, filesize - i, 0) == -1)
		perror("send");
	printf("File sent successfully\n");
	fclose (filefd);
	printf("File closed\n");
}

void get_request(int sockfd)
{
	// send request to server
	int numbytes;
	char buf[MAXDATASIZE];	
	char *request= (char *)"get hello.txt";

	printf("sending get request\n");
	if (send(sockfd, request, strlen(request), 0) == -1)
        perror("send");

	FILE * filefd;
	filefd = fopen("recv.txt", "w");
	while(1){
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) == -1) {
		    perror("recv");
		    printf("error in receiveing data\n");
		    fclose(filefd);
		    exit(1);
		}
		else if (numbytes == 0) {
		    printf("socket closed\n");
		    exit(1);
		}
		if(numbytes == MAXDATASIZE)
		{
			printf("file receiving...\n");
			fwrite (buf , 1, numbytes , filefd);
		}else{
			fwrite (buf , 1, numbytes , filefd);
			printf("File received.\n");
			fclose(filefd);
			printf("File closed\n");
			return;
		}
	}
}
