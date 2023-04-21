#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<netdb.h>
#include<string.h>
#include<signal.h>

#define QUEUE_SIZE 3
#define BUFSIZE 256
#define PORTSIZE 10
#define HOSTSIZE 100

volatile int active = 1;

void handler(int signum){
	active = 0;
}

void install_handlers(void){
	struct sigaction act;
	act.sa_handler = handler;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);

	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL); 
}

int open_listener(char *service, int queue_size){
	struct addrinfo hint, *info_list, *info;
	int error, sock;

	// initialize hints
	memset(&hint, 0, sizeof(struct addrinfo));
	hint.ai_family   = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_flags    = AI_PASSIVE;

	// obtain information for listening socket
	error = getaddrinfo(NULL, service, &hint, &info_list);
	if (error) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
		return -1;
	}

	// attempt to create socket
	for (info = info_list; info != NULL; info = info->ai_next){
		sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

		// could not create socket -> try next method
		if (sock == -1) continue;

		// bind socket to requested port
		error = bind(sock, info->ai_addr, info->ai_addrlen);
		if (error) {
			close(sock);
			continue;
		}

		// enable listening for incoming connection requests 
		error = listen(sock, queue_size);
		if (error) {
			close(sock);
			continue;
		}


		// if we got this far, we opened the socket and exit
		break;
	}

	freeaddrinfo(info_list);

	//info will be NULL if no method succedded 
	if (info == NULL) {
		fprintf(stderr, "Could not bind\n");
		return -1;
	}
	return sock;
}


void read_data(int sock, struct sockaddr *rem, socklen_t rem_len){
	char buf[BUFSIZE+1], host[HOSTSIZE], port[PORTSIZE];
	int bytes, error;

	error = getnameinfo(rem, rem_len, host, HOSTSIZE, port, PORTSIZE, NI_NUMERICSERV);

	if (error){
		fprintf(stderr, "getnameinfo: %s\n", gai_strerror(error));
		strcpy(host, "??");
		strcpy(port, "??");
	}

	printf("Connection from %s:%s\n", host, port);

	while (active && (bytes = read(sock, buf, BUFSIZE)))
	{
		buf[bytes] = '\0';
		printf("[%s:%s] read %d bytes |%s\n", host, port, bytes, buf);
	}

	if (bytes == 0){
			printf("[%s:%s] got EOF\n", host, port);
	} else if (bytes == -1){
		printf("[%s:%s] terminating: %s\n", host, port, strerror(error));
	} else{
		printf("[%s:%s] terminating\n", host, port);
	}

	close(sock);
}

int main(int argc, char ** argv){

	struct sockaddr_storage remote_host;
	socklen_t remote_host_len;
	char *service = argc == 2 ? argv[1] : "15000";

	install_handlers();

	int listener = open_listener(service, QUEUE_SIZE);
	if (listener < 0) exit(EXIT_FAILURE);

	printf("Listening for incoming connections on port [%s]\n", service);

	while (active) {
		remote_host_len = sizeof(remote_host);
		int sock = accept(listener, 
					(struct sockaddr *)&remote_host, 
					&remote_host_len);
		
		if (sock < 0){
			perror("accept");
			continue;
		}

		read_data(sock, (struct sockaddr *) &remote_host, remote_host_len);
	}

	puts("Shutting down");
	close(listener);

	return EXIT_SUCCESS;
}
