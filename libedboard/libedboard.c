#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#include "libedboard.h"

int ledboard_connect(const char *host)
{
	struct addrinfo hints, *res, *p;
	int sockfd;
	int ret;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((ret = getaddrinfo(host, "1337", &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		return -1;
	}
	for (p = res; p; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			continue;
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0)
			break;
		close(sockfd);
	}
	freeaddrinfo(res);
	if (p == NULL) {
		fprintf(stderr, "Unable to connect to %s\n", host);
		return -1;
	}
	return sockfd;
}
