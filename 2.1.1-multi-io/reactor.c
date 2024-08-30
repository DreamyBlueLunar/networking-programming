#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/time.h>

#define CONN_LEN 	1048576
#define PORT_CNT 	20
#define BUFFER_LEN 	512
#define DEBUG_LEVEL 1
#define MOD 		1
#define ADD			0
#define TIME_SUB_MS(tv1, tv2) ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)

typedef int (*RCALLBACK)(int fd);

struct conn_item {
	int fd;
	
	char rbuffer[BUFFER_LEN];
	int rlen;
	char wbuffer[BUFFER_LEN];
	int wlen;

	union {
		RCALLBACK accept_callback;
		RCALLBACK recv_callback;
	} recv_t;
	RCALLBACK send_callback;
};

int epfd = 0;
struct conn_item connlist[CONN_LEN] = {0};

#if DEBUG_LEVEL >= 1
	struct timeval tv_sta;
#endif

int init_server(unsigned short port);
int accept_cb(int fd);
int recv_cb(int fd);
int send_cb(int fd);
int set_event(int fd, int op, int event);

/*
 * set event
 * params:
 * fd: sockfd or clientfd
 * op: operations, add or modify
 * event: EPOLLIN or EPOLLOUT
 * returns 1(succeeded) or -1(failed)
 */
int set_event(int fd, int op, int event) {
	struct epoll_event ev;
	if (op == ADD) {
		ev.events = event;
		ev.data.fd = fd;
		if (-1 == epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev)) {
			#if DEBUG_LEVEL >= 1
				printf("failed to add event\n");
			#endif

			return -1;			
		}
	} else {
		ev.events = event;
		ev.data.fd = fd;
		if (-1 == epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev)) {
			#if DEBUG_LEVEL >= 1
				printf("failed to modify event\n");
			#endif

			return -1;			
		}
	}

	return 1;
}


int init_server(unsigned short port) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sockfd) {
	#if DEBUG_LEVEL >= 1
			printf("failed to create sockfd\n");
	#endif
		
		return -1;
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);

	if (-1 == bind(sockfd, (struct sockaddr*)&server_addr, sizeof (struct sockaddr))) {
	#if DEBUG_LEVEL >= 1
			printf("failed to bind sockfd and server_addr\n");
	#endif

		return -1;
	}

	listen(sockfd, 10);

	return sockfd;
}


/*
 * accept's callback func
 * params:
 * fd: connfd
 * returns clientfd or -1
 */
int accept_cb(int fd) {
	struct sockaddr_in client_addr;
	socklen_t len = sizeof client_addr;
	int clientfd = accept(fd, (struct sockaddr*)&client_addr, &len);
	if (-1 == clientfd) {
		#if DEBUG_LEVEL >= 1
			printf("failed to create clientfd\n");
		#endif
		
		return -1;
	}

	connlist[clientfd].fd = clientfd;
	memset(connlist[clientfd].rbuffer, 0, BUFFER_LEN);
	connlist[clientfd].rlen = 0;
	memset(connlist[clientfd].rbuffer, 0, BUFFER_LEN);	
	connlist[clientfd].wlen = 0;
	connlist[clientfd].recv_t.recv_callback = recv_cb;
	connlist[clientfd].send_callback = send_cb;

	set_event(clientfd, ADD, EPOLLIN);

	#if DEBUG_LEVEL >= 1
		if (999 == clientfd % 1000) {
			struct timeval tv_cur;
			gettimeofday(&tv_cur, NULL);
			int time_used = TIME_SUB_MS(tv_cur, tv_sta);
			memcpy(&tv_sta, &tv_cur, sizeof (struct timeval));
			printf("connections: %d, time_used: %d\n", clientfd, time_used);
		}
	#endif

	return clientfd;
}


/*
 * recv's callback func
 * params:
 * fd: connfd
 * returns cnt or -1
 */
int recv_cb(int fd) {
	char *buffer = connlist[fd].rbuffer;
	int idx = connlist[fd].rlen;
	
	int cnt = recv(fd, buffer + idx, BUFFER_LEN - idx, 0);
	if (cnt == 0) {
		#if DEBUG_LEVEL >= 1
			printf("disconnect\n");
		#endif
		epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);		
		close(fd);
		
		return -1;
	}
	connlist[fd].rlen += cnt;

	memcpy(connlist[fd].wbuffer, connlist[fd].rbuffer, connlist[fd].rlen);
	connlist[fd].wlen = connlist[fd].rlen;
	connlist[fd].rlen -= connlist[fd].rlen;
	
	set_event(fd, MOD, EPOLLOUT);

	
	return cnt;
}


/*
 * send's callback func
 * params:
 * fd: connfd
 * returns cnt
 */
int send_cb(int fd) {
	char *buffer = connlist[fd].wbuffer;
	int idx = connlist[fd].wlen;
	int cnt = send(fd, buffer, idx, 0);
	set_event(fd, MOD, EPOLLIN);

	return cnt;
}


int main(void) {

	int port_cnt = PORT_CNT;
	unsigned short port = 2048;

	epfd = epoll_create(1);
	if (-1 == epfd) {
		#if DEBUG_LEVEL >= 1
			printf("failed to create epfd\n");
		#endif
				
		return -1;
	}

	for (int i = 0; i < port_cnt; i ++) {
		int sockfd = init_server(port + i);
		connlist[sockfd].fd = sockfd;
		connlist[sockfd].recv_t.accept_callback = accept_cb;
		set_event(sockfd, ADD, EPOLLIN);
	}

	#if DEBUG_LEVEL >= 1
		gettimeofday(&tv_sta, NULL);
	#endif

	struct epoll_event events[1024] = {0};
	memset(events, 0, sizeof events);

	while (1) {
//		printf("fuk\n");

		int nready = epoll_wait(epfd, events, 1024, -1);		
		for (int i = 0; i < nready; i ++) {
			int connfd = events[i].data.fd;
			
			if (events[i].events & EPOLLIN) {

				int count = connlist[connfd].recv_t.recv_callback(connfd);
				#if DEBUG_LEVEL >= 2
					printf("recv count: %d <-- buffer: %s\n", count, connlist[connfd].rbuffer);
				#endif
			} else if (events[i].events & EPOLLOUT) { 
				#if DEBUG_LEVEL >= 2
					printf("send --> buffer: %s\n",  connlist[connfd].wbuffer);
				#endif
				int count = connlist[connfd].send_callback(connfd);

			}
		}
	}

	return 0;
}
