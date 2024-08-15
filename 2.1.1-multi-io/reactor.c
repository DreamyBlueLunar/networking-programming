#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/epoll.h>

#define BUFFER_LEN 128

struct conn_item {
	int fd;
	char buffer[BUFFER_LEN];
	int idx;
};

struct conn_item conn_list[1024];

/*
 * reactor implementation
 */
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

				conn_list[clientfd].fd = clientfd;
				memset(conn_list[clientfd].buffer, 0, BUFFER_LEN);
				conn_list[clientfd].idx = 0;

				printf("sockfd: %d, clientfd: %d\n", sockfd, clientfd);
			
			} else if (events[i].events & EPOLLIN) {
				char *buffer = conn_list[connfd].buffer;
				int idx = conn_list[connfd].idx;
				int cnt = recv(connfd, buffer, BUFFER_LEN - idx, 0); // if client calls close(), recv() returns 0.

				if (0 == cnt) {
					perror("client disconnection");
					epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL);					
					close(i);
					continue;
				}
				conn_list[connfd].idx += cnt;

				send(i, buffer, cnt, 0);
				printf("sockfd: %d, clientfd: %d, cnt: %d, buffer: %s\n", 
						sockfd, connfd, cnt, buffer);
			}
		}
	}

	return 0;
}
