/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold
#define MAXDATASIZE 100 // max number of bytes we can be sent at once

void send_file(int, char*);
void accept_file(int, char*);

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener

			char buf[MAXDATASIZE];
			char filename[15];
			int numbytes;
			if ((numbytes = recv(new_fd, buf, MAXDATASIZE, 0)) == -1) {
		        perror("recv");
		        exit(1);
		    }
		    printf("numbytes = %d\n", numbytes);
		    buf[numbytes] = '\0';
		    printf("Received '%s'\n",buf);
		    if(strncmp(buf,"get ", 4) == 0){
		    	strcpy(filename, buf+4);
		    	printf("sending file %s\n", filename);
		    	send_file(new_fd, filename);
		    }
		   	else if(strncmp(buf,"put ", 4) == 0){
		    	strcpy(filename, buf+4);
		    	printf("saving file %s\n", filename);
		    	accept_file(new_fd, filename);
		    }
		    printf("Invalid command\n");
		    close(new_fd);  // parent doesn't need this
		}
	}
	return 0;
}

void accept_file(int new_fd, char* filename)
{
	// need to save file here....
	int numbytes, i =0;	
	char *buffer[MAXDATASIZE];
	size_t result;

	FILE * filefd;
	filefd = fopen(filename, "w");
	while(1)
	{
		if ((numbytes = recv(new_fd, buffer, MAXDATASIZE, 0)) == -1) {
		    perror("recv");
		    printf("error in receiveing data\n");
		    fclose(filefd);
		    exit(1);
		}
		else if (numbytes == 0) {
		    exit(1);
		}
		if(numbytes == MAXDATASIZE)
		{
			printf("file receiving...\n");
			fwrite (buffer , 1, numbytes , filefd);
		}else{
			fwrite (buffer , 1, numbytes , filefd);
			printf("File received.\n");
			fclose(filefd);
			printf("File closed\n");
			exit(1);
		}
	}
}

void send_file(int new_fd, char* filename)
{
	// need to send file here....
	char *buffer[MAXDATASIZE];
	FILE * filefd;
	long filesize;
	size_t result;
	int i =0;

	// open the file in read mode -- get
	if ( (filefd = fopen(filename, "r")) == NULL)
    {
        fprintf(stderr, "Could not open destination file.\n");
        if (send(new_fd, "Error-1", 7, 0) == -1)
		perror("send");
		// close(new_fd);
		// printf("closed socket\n");
		exit(0);
    }else {
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
		  	if (send(new_fd, buffer, MAXDATASIZE, 0) == -1)
				perror("send");
			printf("File sending...\n");					
	  	}
	  	result = fread(buffer, 1, filesize - i, filefd);
	  	if (result != filesize - i)
	  	{
	  		fprintf(stderr, "Reading error2 \n");
	  		exit (0);
	  	}
	  	if (send(new_fd, buffer, filesize - i, 0) == -1)
			perror("send");
		printf("File sent successfully\n");
		fclose (filefd);
		printf("File closed\n");
		// close(new_fd);
		// printf("closed socket\n");
	}
	exit(0);
}
