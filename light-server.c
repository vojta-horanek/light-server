#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <wiringPi.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 2148
#define SA struct sockaddr 

#define CODE_QUIT 4
#define CODE_STATE 3
#define CODE_SWITCH 2
#define CODE_ON 1
#define CODE_OFF 0

char state() {
	FILE *f = fopen("/etc/light/state.txt", "r");
	char state = fgetc(f);
	fclose(f);
	return state;
}

void set_state(char state) {

	FILE *f = fopen("/etc/light/state.txt", "w");
	fputc(state, f);	
	fclose(f);
}

char light(char code) {
	if (code == CODE_SWITCH)
		code = (state() == 1) ? 0 : 1;
	digitalWrite(0, code == 1 ? 0 : 1);
	set_state(code);
	return code;
}

int client(int sockfd) 
{ 
	char request;
	char msg;

	read(sockfd, &request, 1); 

	switch (request) {
	
		case CODE_OFF:
		case CODE_ON:
		case CODE_SWITCH:
			msg = light(request);
			break;
		default:
		case CODE_STATE:
			msg = state();
			break;
		case CODE_QUIT:
			close(sockfd);
			return 1;
	}

	write(sockfd, &msg, 1); 
	close(sockfd);
	return 0;
} 

int main() 
{ 
	int sockfd, connfd;
	socklen_t len; 
	struct sockaddr_in servaddr, cli; 

	if(wiringPiSetup() == -1)
		return -1;
	pinMode(0, OUTPUT);
	digitalWrite(0, 1);

	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		fprintf(stderr, "socket creation failed...\n"); 
		exit(1); 
	} 
	bzero(&servaddr, sizeof(servaddr)); 

	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servaddr.sin_port = htons(PORT); 

	int enable = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		    fprintf(stderr, "setsockopt(SO_REUSEADDR) failed");

	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
		fprintf(stderr, "socket bind failed...\n"); 
		exit(1); 
	} 
	
	if ((listen(sockfd, 5)) != 0) { 
		fprintf(stderr ,"listen failed...\n"); 
		exit(1); 
	} 
	
	len = sizeof(cli); 

	int quit_requested = 0;
	while (quit_requested != 1) {
		connfd = accept(sockfd, (SA*)&cli, &len); 
		if (connfd < 0) { 
			fprintf(stderr, "server acccept failed...\n"); 
			exit(1); 
		} 
		quit_requested = client(connfd); 
	}
	close(sockfd); 
} 
