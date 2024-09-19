/*
 * 这个代码目前没法执行，除非改一下read函数的定义
 * 侧重于描述利用hook和协程实现webserver的一个思路
 */



#define _GNU_SOURCE

#include <dlfcn.h>

#include <stdio.h>
#include <ucontext.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/epoll.h>


// hook
typedef ssize_t (*read_t)(int fd, void *buf, size_t count);
read_t read_f = NULL;

typedef ssize_t (*write_t)(int fd, const void *buf, size_t count);
write_t write_f = NULL;

ssize_t read(int fd, void *buf, size_t count) {
#if 0
	struct pollfd fds[1] = {0};

	fds[0].fd = fd;
	fds[0].events = POLLIN;

	int res = poll(fds, 1, 0);
	if (res <= 0) { //


		// fd --> epoll_ctl();

		swapcontext(); // fd --> ctx
		
	}
	// io
#endif

	ssize_t ret = read_f(fd, buf, count);
	printf("hook-read: %s\n", (char*)buf);
	return ret;
}

ssize_t write(int fd, const void *buf, size_t count) {
	printf("hook-write: %s\n", (const char*)buf);
	return write_f(fd, buf, count);
}

void init_hook(void) {
	if (!read_f) {
		read_f = dlsym(RTLD_NEXT, "read");
	}

	if (!write_f) {
		write_f = dlsym(RTLD_NEXT, "write");
	}
}

int main(void) {

	init_hook();

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

	struct sockaddr_in client_addr;
	socklen_t len = sizeof client_addr;
	int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &len);
	printf("accepted\n");

	while (1) {
		char buffer[128] = {0};
		int cnt = read(clientfd, buffer, sizeof buffer);

		if (0 == cnt) {
			perror("client disconnection");
			break;
		}

		write(clientfd, buffer, cnt);
	}
	
	getchar();
	close(clientfd);

    return 0;
}
