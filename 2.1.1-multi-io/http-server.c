#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFFER_LEN 1024
#define DEBUG_LEVEL 0
#define ADD 1
#define MOD 0
#define ENABLE_HTTP_RESPONSE 1
#define ROOT "/home/telecaster/network_programming/2.1.1-multi-io"

typedef int (*RCALLBACK)(int fd);

// conn, fd, buffer, callback
struct conn_item {
	int fd;
	
	char rbuffer[BUFFER_LEN]; // read
	int rlen;
	
	char wbuffer[BUFFER_LEN]; // write
	int wlen;

	char resource[BUFFER_LEN]; // abc.html...

	union {
		RCALLBACK accept_callback;
		RCALLBACK recv_callback;
	} recv_t;
	RCALLBACK send_callback;
};

typedef struct conn_item connection_t;

struct conn_item conn_list[1024];
struct epoll_event ev;

int epfd = 0, sockfd = 0;

/* sockfd triggers EPOLLIN -> accept_cb()
 * params:
 * fd: sockfd
 */
int accept_cb(int fd);

/* clientfd triggers EPOLLIN -> recv_cb()
 * params:
 * fd: clientfd
 */
int recv_cb(int fd);

/* clientfd triggers EPOLLOUT -> send_cb()
 * params:
 * fd: clientfd
 */
int send_cb(int fd);

/* set event
 * params:
 * fd: file describtor
 * op: operation
 * event: EPOLLIN / EPOLLOUT
 * return:
 * success -> return 1
 * add error -> return -1
 * mod error -> return 0
 */
int set_event(int fd, int op, int event);

// http://192.168.113.128/index.html
// GET /index.html HTTP/1.1
// http://192.168.113.128/abc.html
// GET /abc.html HTTP/1.1
int http_request(connection_t* conn) {

	// conn->rbuffer
	// conn->rlen
	
	return 0;
}

#if ENABLE_HTTP_RESPONSE
int http_response(connection_t* conn) {
#if 0
	conn->wlen = sprintf(conn->wbuffer, 
		"HTTP/1.1 301 Moved Permanently\r\n"
		"Server: JSP3/2.0.14\r\n"
		"Date: Thu, 22 Aug 2024 07:45:31 GMT\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 168\r\n"
		"Connection: keep-alive\r\n"
		"Location: https://programmercarl.com/\r\n"
		"X-Cache-Status: MISS\r\n"
		"\r\n"
		"[HTTP response 1/1]"
		"[Time since request: 0.016910000 seconds]"
		"[Request in frame: 46]"
		"[Request URI: http://programmercarl.com/]"
		"File Data: 168 bytes");
#else

	int filefd = open("index.html", O_RDONLY);

	struct stat stat_buf;
	fstat(filefd, &stat_buf);
	
	conn->wlen = sprintf(conn->wbuffer, 
			"HTTP/1.1 200 OK\r\n"
			"Accept-Ranges: bytes\r\n"
			"Content-Length: %ld\r\n"
			"Content-Type: text/html\r\n"
			"Date: Thu, 22 Aug 2024 07:45:31 GMT\r\n\r\n", stat_buf.st_size);
	int cnt = read(filefd, conn->wbuffer + conn->wlen, BUFFER_LEN - conn->wlen);
	conn->wlen += cnt;

#endif

	return conn->wlen;
}
#endif

/*
 * http server implementation
 */
int main(void) {
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sockfd) {
		perror("create sockfd");
		return -1;
	}
	
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(struct sockaddr_in));
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(2048);

	if (-1 == bind(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr))) {
		perror("bind");
 		return -1;
	}

	listen(sockfd, 128);

	conn_list[sockfd].fd = sockfd;
	conn_list[sockfd].recv_t.accept_callback = accept_cb;

	epfd = epoll_create(1);
	if (-1 == epfd) {
		perror("create epoll");
		return -1;
	}
	
	if (1 != set_event(sockfd, ADD, EPOLLIN)) {
		perror("add sockfd");
		return -1;
	}

	struct epoll_event events[1024] = {0};
	memset(&events, 0, sizeof events);
	
	printf("this is http-server-test.\n");

	while (1) { // main loop
		printf("http_server@test:/$ ");
		fflush(stdout);

		int nready = epoll_wait(epfd, events, 1024, -1);
		for (int i = 0; i < nready; i ++) {
			int connfd = events[i].data.fd;
			
			if (events[i].events & EPOLLIN) { // if sockfd triggers EPOLLIN, 
						//we call accept_cb() successfully here,cause we set the union's value correctly.

				int count = conn_list[connfd].recv_t.recv_callback(connfd);

			} else if (events[i].events & EPOLLOUT) {

				int count = conn_list[connfd].send_callback(connfd);
			 
			}
		}
	}

	return 0;
}


int accept_cb(int fd) {
	struct sockaddr_in client_addr;
	socklen_t len = sizeof client_addr;

	int clientfd = accept(fd, (struct sockaddr*)&client_addr, &len);
	if (0 > clientfd) {
		perror("accept");
		return -1;
	}
	
	if (1 != set_event(clientfd, ADD, EPOLLIN)) {
		perror("add clientfd");
		return -1;
	}

	conn_list[clientfd].fd = clientfd;
	memset(conn_list[clientfd].rbuffer, 0, BUFFER_LEN);
	memset(conn_list[clientfd].wbuffer, 0, BUFFER_LEN);
	conn_list[clientfd].rlen = 0;
	conn_list[clientfd].wlen = 0;
	conn_list[clientfd].recv_t.recv_callback = recv_cb;
	conn_list[clientfd].send_callback = send_cb;

#if DEBUG_LEVEL 

	printf("sockfd: %d, clientfd: %d\n", fd, clientfd);

#else

	printf("accept done\n");

#endif

	return clientfd;
}


int recv_cb(int fd) {
	// recv_cb();
	char *buffer = conn_list[fd].rbuffer;
	int idx = conn_list[fd].rlen;
	int cnt = recv(fd, buffer + idx, BUFFER_LEN - idx, 0); // if client calls close(), recv() returns 0.

	if (0 == cnt) {
		perror("client disconnection");
		epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
		close(fd);
		return -1;
	}
	conn_list[fd].rlen += cnt;

	// operations
	http_request(&conn_list[fd]);
	http_response(&conn_list[fd]);
	
	if (1 != set_event(fd, MOD, EPOLLOUT)) {
		perror("modify clientfd EPOLLOUT");
		return -1;
	}

#if DEBUG_LEVEL

	printf("recv <-- sockfd: %d, clientfd: %d, cnt: %d, buffer: %s\n",
			sockfd, fd, cnt, conn_list[fd].rbuffer);

#else
		
	printf("recv done\n");

#endif

	return cnt;
}


int send_cb(int fd) {
	int cnt = send(fd, conn_list[fd].wbuffer, conn_list[fd].wlen, 0);
	if (-1 == cnt) {
		perror("send");
		return -1;
	}
	
	if (1 != set_event(fd, MOD, EPOLLIN)) {
		perror("modify clientfd EPOLLOUT");
		return -1;
	}


#if DEBUG_LEVEL

	printf("send --> sockfd: %d, clientfd: %d, cnt: %d, buffer: %s\n",
			sockfd, fd, cnt, conn_list[fd].wbuffer);
#else
			
	printf("send done\n");

#endif	
	
	return cnt;
}


int set_event(int fd, int op, int event) {
	if (op == 1) { // 1 add, 0 mod

		ev.events = event;
		ev.data.fd = fd;
		
		if (-1 == epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) ) {
			return -1;
		}

	} else {

		ev.events = event;
		ev.data.fd = fd;
		
		if (-1 == epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) ) {
			return 0;
		}

	}

	return 1;
}

