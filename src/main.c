#include <assert.h>

#include <errno.h>
#include <string.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <sys/epoll.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include <unistd.h>

typedef int			gsh_fd_t;
typedef uint32_t	gsh_return_t;
typedef int			gsh_unix_return_t;

const gsh_return_t	gsh_return_success = 0;

const in_port_t g_port = 29987;
const char *	g_s_port = "29987";

#define	gassert(condition) do { \
	if (!(condition)) { \
		fprintf(stderr, "%s\n", strerror(errno)); \
		assert((condition)); \
	} \
 } while (0)

gsh_return_t socket_and_bind(const char *host, const char *port, gsh_fd_t *ret_sockfd)
{
	gsh_unix_return_t	r;

	gsh_fd_t				sockfd;

	struct addrinfo		hints;
	struct addrinfo		*p_addrinfo, *p_next;

	memset(&hints, 0x0, sizeof(struct addrinfo));
	hints.ai_family		= AF_UNSPEC;
	hints.ai_socktype	= SOCK_STREAM;
	hints.ai_flags		= AI_PASSIVE;

	r = getaddrinfo(host, port, &hints, &p_addrinfo);
	gassert(r == 0);

	for (p_next = p_addrinfo; p_next != NULL; p_next = p_next->ai_next) {
	
		sockfd = socket(p_next->ai_family, p_next->ai_socktype | SOCK_NONBLOCK | SOCK_CLOEXEC, p_next->ai_protocol);
		gassert(sockfd != -1);

		r = bind(sockfd, p_next->ai_addr, p_next->ai_addrlen);
		gassert(r == 0);

		if (r == 0) {
			*ret_sockfd = sockfd;
			break;
		}

		close(sockfd);
	}

	freeaddrinfo(p_addrinfo);

	return gsh_return_success;
}

gsh_return_t create_epoll(gsh_fd_t *ret_epollfd)
{
	gsh_fd_t epfd;

	epfd = epoll_create1(EPOLL_CLOEXEC);
	gassert(epfd != -1);

	*ret_epollfd = epfd;

	return gsh_return_success;
}

int main(const int argc, const char * argv[])
{
	gsh_unix_return_t	r;
	gsh_fd_t	sockfd, epollfd;
	struct epoll_event event;
	struct epoll_event *p_event;

	socket_and_bind("127.0.0.1", "21036", &sockfd);
	printf("%d\n", sockfd);

	r = listen(sockfd, 100);
	gassert(r == 0);

	create_epoll(&epollfd);

	event.data.fd = sockfd;
	event.events = EPOLLIN;

	r = epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event);
	gassert(r == 0);

	p_event = calloc(1024, sizeof(struct epoll_event));
	gassert(p_event != NULL);

	int	n;
	const static int INDEFINITE = -1;
	while (1) {
		n = epoll_wait(epollfd, p_event, 1024, INDEFINITE);
		gassert(n != -1);

		for (size_t i = 0; i < n; i++) {

			if ((p_event[i].events & EPOLLERR)
				|| (p_event[i].events & EPOLLHUP)
				|| !(p_event[i].events & EPOLLIN)) {

				close(p_event[i].data.fd);
				continue;
			} else if (sockfd == p_event[i].data.fd) {

				while (1) {

					struct sockaddr	accept_addr;
					socklen_t	addrlen;

					gsh_fd_t	accept_fd;

					char	host_buffer[1024], service_buffer[1024];

					addrlen = sizeof(accept_addr);
					accept_fd = accept(sockfd, &accept_addr, &addrlen);
					gassert(accept_fd != -1);

					r = getnameinfo(&accept_addr, addrlen, 
						host_buffer, sizeof(host_buffer), 
						service_buffer, sizeof(service_buffer), 
						NI_NUMERICHOST | NI_NUMERICSERV);

					gassert(r == 0);
					printf("%s %s\n", host_buffer, service_buffer);
				}
			}
		}
	}

	return EXIT_SUCCESS;
}
