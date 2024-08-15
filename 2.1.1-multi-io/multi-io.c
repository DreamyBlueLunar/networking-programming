#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/epoll.h>

#define SINGLE_MODE 	0
#define LOOP_MODE   	0
#define MULTI_PTHREAD	0
#define SELECT_MODE		0
#define POLL_MODE		0
#define EPOLL_MODE		1

void* client_thread(void* arg); // create linux thread!!!!!!!!!!

/*
 * tcp socket programming
 */
// tcp server
int main(void) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(struct sockaddr_in));
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(2048);

	if (-1 == bind(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr))) {
		perror("bind");
 		return -1;
	}

	listen(sockfd, 10);

#if SINGLE_MODE

	struct sockaddr_in client_addr;
	socklen_t len = sizeof client_addr;
	int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &len);
	printf("accepted\n");

	char buffer[128] = {0};
	int cnt = recv(clientfd, buffer, sizeof buffer, 0);

	printf("sockfd: %d, clientfd: %d, cnt: %d, buffer: %s\n", 
			sockfd, clientfd, cnt, buffer);

	send(clientfd, buffer, cnt, 0);

	printf("now we are before getchar().\n");
	getchar();
	close(clientfd);


#elif LOOP_MODE

	while (1) { // obviously, the logic is wrong.
		char buffer[128] = {0};
		int cnt = recv(clientfd, buffer, sizeof buffer, 0); // if client calls close(), recv() returns 0.

		if (0 == cnt) {
			perror("client disconnection");
			break;
		}

		send(clientfd, buffer, cnt, 0);
		printf("sockfd: %d, clientfd: %d, cnt: %d, buffer: %s\n", 
				sockfd, clientfd, cnt, buffer);
	}
	
	printf("now we are before getchar().\n");
	getchar();
	close(clientfd);

# elif MULTI_PTHREAD

	while (1) { // obviously, the logic is wrong.
		struct sockaddr_in client_addr;
		socklen_t len = sizeof client_addr;
		int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &len);

		pthread_t thid;

		printf("accepted\n");
		pthread_create(&thid, NULL, client_thread, &clientfd);
	}

	printf("now we are before getchar().\n");
	getchar();

// io multiplexing
# elif SELECT_MODE // select - 5 param ->  maxfd,    rset,    wset,    eset, timeout
//							              最大fd值 可读集合 可写集合 错误集合 超时
//	int nready = select(maxfd, rset, wset, eset, timeout);

	fd_set rfds, rset;
	FD_ZERO(&rfds);
	FD_SET(sockfd, &rfds);

	int maxfd = sockfd;

	printf("this is tcp-server-select.\n");

	while (1) {
		printf("tcp_server@select:/$ ");
		fflush(stdout);
		rset = rfds;

		int nready = select(maxfd + 1, &rset, NULL, NULL, NULL); // nready -> numbers of events

		if (FD_ISSET(sockfd, &rset)) {
			struct sockaddr_in client_addr;
			socklen_t len = sizeof client_addr;

			int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &len);

			printf("sockfd: %d\n", sockfd);

			FD_SET(clientfd, &rfds);
			maxfd = clientfd;
		}

		for (int i = sockfd + 1; i <= maxfd; i ++) {
			if (FD_ISSET(i, &rset)) {
				char buffer[128] = {0};
				int cnt = recv(i, buffer, sizeof buffer, 0); // if client calls close(), recv() returns 0.

				if (0 == cnt) {
					perror("client disconnection");
					FD_CLR(i, &rfds);
					close(i);
					continue;
				}

				send(i, buffer, cnt, 0);
				printf("sockfd: %d, clientfd: %d, cnt: %d, buffer: %s\n", 
						sockfd, i, cnt, buffer);
			}
		}
	}

#elif POLL_MODE // very similar to select

	struct pollfd fds[1024] = {0};
	fds[sockfd].fd  = sockfd;
	fds[sockfd].events = POLLIN;

	int maxfd = sockfd; // not neccessary, just to avoid extra 

	printf("this is tcp-server-poll.\n");

	while (1) {
		printf("tcp_server@poll:/$ ");
		fflush(stdout);
		int nready = poll(fds, maxfd + 1, -1); // similar to select, 

		if (fds[sockfd].revents & POLLIN) {
			struct sockaddr_in client_addr;
			socklen_t len = sizeof client_addr;

			int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &len);

			printf("sockfd: %d\n", sockfd);

			fds[clientfd].fd	= clientfd;
			fds[clientfd].events = POLLIN;

			maxfd = clientfd;
		}

		for (int i = sockfd + 1; i <= maxfd; i ++) {
			if (fds[i].revents & POLLIN) {
				char buffer[128] = {0};
				int cnt = recv(i, buffer, sizeof buffer, 0); // if client calls close(), recv() returns 0.

				if (0 == cnt) {
					perror("client disconnection");

					fds[i].fd = -1;
					fds[i].events = POLLIN;

					close(i);
					continue;
				}

				send(i, buffer, cnt, 0);
				printf("sockfd: %d, clientfd: %d, cnt: %d, buffer: %s\n", 
						sockfd, i, cnt, buffer);			
			}
		}
	}

	
#elif EPOLL_MODE // just focus on how to code, LT and ET 
//	int epoll_create(int size);
// 		-> size: size of queue
	int epfd = epoll_create(1);

	struct epoll_event ev;
//	ev.events = EPOLLIN | EPOLLET; 	// ET
	ev.events = EPOLLIN;			// LT
	ev.data.fd = sockfd;
	
	epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

	struct epoll_event events[1024] = {0};
	
	printf("this is tcp-server-epoll.\n");

	while (1) {
		printf("tcp_server@epoll:/$ ");
		fflush(stdout);

		int nready = epoll_wait(epfd, events, 1024, -1);
		for (int i = 0; i < nready; i ++) {
			int connfd = events[i].data.fd;
			if (sockfd == connfd) {

				struct sockaddr_in client_addr;
				socklen_t len = sizeof client_addr;

				int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &len);
				ev.events = EPOLLIN;
				ev.data.fd = clientfd;
				
				epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);

				printf("sockfd: %d, clientfd: %d\n", sockfd, clientfd);
			
			} else if (events[i].events & EPOLLIN) {
				char buffer[128] = {0};
				int cnt = recv(connfd, buffer, sizeof buffer, 0); // if client calls close(), recv() returns 0.

				if (0 == cnt) {
					perror("client disconnection");
					epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL);					
					close(i);
					continue;
				}

				send(i, buffer, cnt, 0);
				printf("sockfd: %d, clientfd: %d, cnt: %d, buffer: %s\n", 
						sockfd, connfd, cnt, buffer);
			}
		}
	}

		
#else

	printf("There are 6 versions of tcp server's implementation.\nModify the macros to run it.\nciao~");

#endif

	return 0;
}


void* client_thread(void* arg) {
	int clientfd = *(int*)arg;

	while (1) { // obviously, the logic is wrong.
		char buffer[128] = {0};
		int cnt = recv(clientfd, buffer, sizeof buffer, 0); // if client calls close(), recv() returns 0.

		if (0 == cnt) {
			perror("client disconnection");
			break;
		}

		send(clientfd, buffer, cnt, 0);
		printf("clientfd: %d, cnt: %d, buffer: %s\n", clientfd, cnt, buffer);
	}

	close(clientfd);
}





