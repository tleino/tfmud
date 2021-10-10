/*
 * ISC License
 *
 * Copyright (c) 2021, Tommi Leino <namhas@gmail.com>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <syslog.h>
#include <strings.h>

int tcpbind(const char *ip, int port)
{
	int fd, opt;
	struct sockaddr_in a;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		syslog(LOG_ERR, "failed to create tcp socket");
		return -1;
	}

	opt = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		syslog(LOG_ERR, "failed to set socket options: %m");
		return -1;
	}

	bzero(&a, sizeof(a));
	if (ip[0] == '*')
		a.sin_addr.s_addr = INADDR_ANY;
	else
		a.sin_addr.s_addr = inet_addr(ip);
	a.sin_port = htons(port);
	a.sin_family = AF_INET;
	if (bind(fd, (struct sockaddr *) &a, sizeof(a)) < 0) {
		syslog(LOG_ERR, "binding to address %s:%d failed: %m",
		    ip, port);
		return -1;
	}

#define LISTENQ 1024
	if (listen(fd, LISTENQ) < 0) {
		syslog(LOG_ERR, "failed to set listen queue of %d", LISTENQ);
		return -1;
	}

	return fd;
}
