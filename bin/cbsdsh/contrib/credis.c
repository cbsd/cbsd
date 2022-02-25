// #define PRINTDEBUG

/* credis.c -- a C client library for Redis
 *
 * Copyright (c) 2009-2010, Jonas Romfelt <jonas at romfelt dot se>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Credis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#else
#ifdef __FreeBSD__
#include <sys/types.h>
#endif
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#endif
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "credis.h"

#ifdef WIN32
void
close(int fd)
{
	closesocket(fd);
}
#endif

#define _STRINGIF(arg) #arg
#define STRINGIFY(arg) _STRINGIF(arg)

#define CR_VERSION_STRING_SIZE_STR STRINGIFY(CREDIS_VERSION_STRING_SIZE)
#define CR_MULTIPLEXING_API_SIZE_STR STRINGIFY(CREDIS_MULTIPLEXING_API_SIZE)
#define CR_USED_MEMORY_HUMAN_SIZE_STR STRINGIFY(CREDIS_USED_MEMORY_HUMAN_SIZE)

#ifdef PRINTDEBUG
/* add -DPRINTDEBUG to CPPFLAGS in Makefile for debug outputs */
#define DEBUG(...)                                             \
	do {                                                   \
		printf("%s() @ %d: ", __FUNCTION__, __LINE__); \
		printf(__VA_ARGS__);                           \
		printf("\n");                                  \
	} while (0)
#else
#define DEBUG(...)
#endif

/* format warnings are GNU C specific */
#if !__GNUC__
#define __attribute__(x)
#endif

/* Returns pointer to the '\r' of the first occurence of "\r\n", or NULL
 * if not found */
static char *
cr_findnl(char *buf, int len)
{
	while (--len >= 0) {
		if (*(buf++) == '\r')
			if (*buf == '\n')
				return --buf;
	}
	return NULL;
}

/* Allocate at least `size' bytes more buffer memory, keeping content of
 * previously allocated memory untouched.
 * Returns:
 *   0  on success
 *  -1  on error, i.e. more memory not available */
static int
cr_moremem(cr_buffer *buf, int size)
{
	char *ptr;
	int total, n;

	n = size / CR_BUFFER_SIZE + 1;
	total = buf->size + n * CR_BUFFER_SIZE;

	DEBUG("allocate %d x CR_BUFFER_SIZE, total %d bytes", n, total);

	ptr = realloc(buf->data, total);
	if (ptr == NULL)
		return -1;

	buf->data = ptr;
	buf->size = total;
	return 0;
}

/* Allocate at least `size' more multibulk storage, keeping content of
 * previously allocated memory untouched.
 * Returns:
 *   0  on success
 *  -1  on error, i.e. more memory not available */
static int
cr_morebulk(cr_multibulk *mb, int size)
{
	char **cptr;
	int *iptr;
	int total, n;

	n = (size / CR_MULTIBULK_SIZE + 1) * CR_MULTIBULK_SIZE;
	total = mb->size + n;

	DEBUG("allocate %d x CR_MULTIBULK_SIZE, total %d (%lu bytes)", n, total,
	    total * ((sizeof(char *) + sizeof(int))));
	cptr = realloc(mb->bulks, total * sizeof(char *));
	iptr = realloc(mb->idxs, total * sizeof(int));

	if (cptr == NULL || iptr == NULL)
		return CREDIS_ERR_NOMEM;

	mb->bulks = cptr;
	mb->idxs = iptr;
	mb->size = total;
	return 0;
}

/*
// Splits string `str' on character `token' builds a multi-bulk array from
// the items. This function will modify the contents of what `str' points
// to.
// Returns:
//   0  on success
//  <0  on error, i.e. more memory not available
static int cr_splitstrtromultibulk(REDIS rhnd, char *str, const char token) {
  int i = 0;

  if (str != NULL) {
    rhnd->reply.multibulk.bulks[i++] = str;
    while ((str = strchr(str, token))) {
      *str++ = '\0';
      if (i >= rhnd->reply.multibulk.size)
	if (cr_morebulk(&(rhnd->reply.multibulk), 1))
	  return CREDIS_ERR_NOMEM;

      rhnd->reply.multibulk.bulks[i++] = str;
    }
  }
  rhnd->reply.multibulk.len = i;
  return 0;
}
*/

// Appends a printf style formatted to the end of buffer `buf'. If available
// memory in buffer is not enough to hold `str' more memory is allocated to
// the buffer.
// Returns:
//   0  on success
//  <0  on error, i.e. more memory not available
int
cr_appendstrf(cr_buffer *buf, const char *format, ...)
{
	int rc, avail;
	va_list ap;

	avail = buf->size - buf->len;

	va_start(ap, format);
	rc = vsnprintf(buf->data + buf->len, avail, format, ap);
	va_end(ap);

	if (rc < 0)
		return -1;

	if (rc >= avail) {
		if (cr_moremem(buf, rc - avail + 1))
			return CREDIS_ERR_NOMEM;

		va_start(ap, format);
		rc = vsnprintf(buf->data + buf->len, buf->size - buf->len,
		    format, ap);
		va_end(ap);
	}
	buf->len += rc;

	return 0;
}

/*

// Appends a zero-terminated string `str' to the end of buffer `buf'. If
// available memory in buffer is not enough to hold `str' more memory is
// allocated to the buffer. If `space' is not 0 `str' is padded with a space.
// Returns:
//   0  on success
//  <0  on error, i.e. more memory not available
static int cr_appendstr(cr_buffer *buf, const char *str, int space) {
  int avail, len, reqd;

  len = strlen(str);
  avail = buf->size - buf->len;

  // required memory: len, terminating zero and possibly a space
  reqd = len + 1;
  if (space)
    reqd++;

  if (reqd > avail)
    if (cr_moremem(buf, reqd - avail + 1))
      return CREDIS_ERR_NOMEM;

  if (space)
    buf->data[buf->len++] = ' ';

  memcpy(buf->data + buf->len, str, len);
  buf->len += len;

  buf->data[buf->len] = '\0';

  return 0;
}

// Appends an array of strings `strv' to the end of buffer `buf', each
// separated with a space. If `newline' is not 0 "\r\n" is added last
// to buffer.
// Returns:
//   0  on success
//  <0  on error, i.e. more memory not available
static int cr_appendstrarray(cr_buffer *buf, int strc, const char **strv, int
newline) { int rc, i;

  for (i = 0; i < strc; i++) {
    if ((rc = cr_appendstr(buf, strv[i], 1)) != 0)
      return rc;
  }

  if (newline) {
    if ((rc = cr_appendstr(buf, "\r\n", 0)) != 0)
      return rc;
  }

  return 0;
}
*/

/* Helper function for select that waits for `timeout' milliseconds
 * for `fd' to become readable (`readable' == 1) or writable.
 * Returns:
 *  >0  `fd' became readable or writable
 *   0  timeout
 *  -1  on error */
int
cr_select(int fd, int timeout, int readable)
{
	struct timeval tv;
	fd_set fds;

	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	if (readable == 1)
		return select(fd + 1, &fds, NULL, NULL, &tv);

	return select(fd + 1, NULL, &fds, NULL, &tv);
}
#define cr_selectreadable(fd, timeout) cr_select(fd, timeout, 1)
#define cr_selectwritable(fd, timeout) cr_select(fd, timeout, 0)

/* Receives at most `size' bytes from socket `fd' to `buf'. Times out after
 * `msecs' milliseconds if no data has yet arrived.
 * Returns:
 *  >0  number of read bytes on success
 *   0  server closed connection
 *  -1  on error
 *  -2  on timeout */
static int
cr_receivedata(int fd, unsigned int msecs, char *buf, int size)
{
	int rc = cr_selectreadable(fd, msecs);

	if (rc > 0)
		return recv(fd, buf, size, 0);
	else if (rc == 0)
		return -2;
	else
		return -1;
}

/* Sends `size' bytes from `buf' to socket `fd' and times out after `msecs'
 * milliseconds if not all data has been sent.
 * Returns:
 *  >0  number of bytes sent; if less than `size' it means that timeout occurred
 *  -1  on error */
static int
cr_senddata(int fd, unsigned int msecs, char *buf, int size)
{
	fd_set fds;
	struct timeval tv;
	int rc, sent = 0;

	/* NOTE: On Linux, select() modifies timeout to reflect the amount
	 * of time not slept, on other systems it is likely not the same */
	tv.tv_sec = msecs / 1000;
	tv.tv_usec = (msecs % 1000) * 1000;

	while (sent < size) {
		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		rc = select(fd + 1, NULL, &fds, NULL, &tv);

		if (rc > 0) {
			rc = send(fd, buf + sent, size - sent, 0);
			if (rc < 0)
				return -1;
			sent += rc;
		} else if (rc == 0) /* timeout */
			break;
		else
			return -1;
	}

	return sent;
}

/* Buffered read line, returns pointer to zero-terminated string
 * and length of that string. `start' specifies from which byte
 * to start looking for "\r\n".
 * Returns:
 *  >0  length of string to which pointer `line' refers. `idx' is
 *      an optional pointer for returning start index of line with
 *      respect to buffer.
 *   0  connection to Redis server was closed
 *  -1  on error, i.e. a string is not available */
static int
cr_readln(REDIS rhnd, int start, char **line, int *idx)
{
	cr_buffer *buf = &(rhnd->buf);
	char *nl;
	int rc, len, avail, more;

	/* do we need more data before we expect to find "\r\n"? */
	if ((more = buf->idx + start + 2 - buf->len) < 0)
		more = 0;

	while (more > 0 ||
	    (nl = cr_findnl(buf->data + buf->idx + start,
		 buf->len - (buf->idx + start))) == NULL) {
		avail = buf->size - buf->len;
		if (avail < CR_BUFFER_WATERMARK || avail < more) {
			DEBUG(
			    "available buffer memory is low, get more memory");
			if (cr_moremem(buf, more > 0 ? more : 1))
				return CREDIS_ERR_NOMEM;

			avail = buf->size - buf->len;
		}

		rc = cr_receivedata(rhnd->fd, rhnd->timeout,
		    buf->data + buf->len, avail);
		if (rc > 0) {
			DEBUG("received %d bytes: %s", rc,
			    buf->data + buf->len);
			buf->len += rc;
		} else if (rc == 0)
			return 0; /* EOF reached, connection terminated */
		else
			return -1; /* error */

		/* do we need more data before we expect to find "\r\n"? */
		if ((more = buf->idx + start + 2 - buf->len) < 0)
			more = 0;
	}

	*nl = '\0'; /* zero terminate */

	*line = buf->data + buf->idx;
	if (idx)
		*idx = buf->idx;
	len = nl - *line;
	buf->idx = (nl - buf->data) + 2; /* skip "\r\n" */

	DEBUG("size=%d, len=%d, idx=%d, start=%d, line=%s", buf->size, buf->len,
	    buf->idx, start, *line);

	return len;
}

static int
cr_receivemultibulk(REDIS rhnd, char *line)
{
	int bnum, blen, i, rc = 0, idx;

	bnum = atoi(line);

	if (bnum == -1) {
		rhnd->reply.multibulk.len = 0; /* no data or key didn't exist */
		return 0;
	} else if (bnum > rhnd->reply.multibulk.size) {
		DEBUG("available multibulk storage is low, get more memory");
		if (cr_morebulk(&(rhnd->reply.multibulk),
			bnum - rhnd->reply.multibulk.size))
			return CREDIS_ERR_NOMEM;
	}

	for (i = 0; bnum > 0 && (rc = cr_readln(rhnd, 0, &line, NULL)) > 0;
	     i++, bnum--) {
		if (*(line++) != CR_BULK)
			return CREDIS_ERR_PROTOCOL;

		blen = atoi(line);
		if (blen == -1)
			rhnd->reply.multibulk.idxs[i] = -1;
		else {
			if ((rc = cr_readln(rhnd, blen, &line, &idx)) != blen)
				return CREDIS_ERR_PROTOCOL;

			rhnd->reply.multibulk.idxs[i] = idx;
		}
	}

	if (bnum != 0) {
		DEBUG("bnum != 0, bnum=%d, rc=%d", bnum, rc);
		return CREDIS_ERR_PROTOCOL;
	}

	rhnd->reply.multibulk.len = i;
	for (i = 0; i < rhnd->reply.multibulk.len; i++) {
		if (rhnd->reply.multibulk.idxs[i] > 0)
			rhnd->reply.multibulk.bulks[i] = rhnd->buf.data +
			    rhnd->reply.multibulk.idxs[i];
		else
			rhnd->reply.multibulk.bulks[i] = NULL;
	}

	return 0;
}

static int
cr_receivebulk(REDIS rhnd, char *line)
{
	int blen;

	blen = atoi(line);
	if (blen == -1) {
		rhnd->reply.bulk = NULL; /* key didn't exist */
		return 0;
	}
	if (cr_readln(rhnd, blen, &line, NULL) >= 0) {
		rhnd->reply.bulk = line;
		return 0;
	}

	return CREDIS_ERR_PROTOCOL;
}

static int
cr_receiveinline(REDIS rhnd, char *line)
{
	rhnd->reply.line = line;
	return 0;
}

static int
cr_receiveint(REDIS rhnd, char *line)
{
	rhnd->reply.integer = atoi(line);
	return 0;
}

static int
cr_receiveerror(REDIS rhnd, char *line)
{
	rhnd->reply.line = line;
	return CREDIS_ERR_PROTOCOL;
}

static int
cr_receivereply(REDIS rhnd, char recvtype)
{
	char *line, prefix = 0;

	/* reset common send/receive buffer */
	rhnd->buf.len = 0;
	rhnd->buf.idx = 0;

	if (cr_readln(rhnd, 0, &line, NULL) > 0) {
		prefix = *(line++);

		if (prefix != recvtype && prefix != CR_ERROR)
			return CREDIS_ERR_PROTOCOL;

		switch (prefix) {
		case CR_ERROR:
			return cr_receiveerror(rhnd, line);
		case CR_INLINE:
			return cr_receiveinline(rhnd, line);
		case CR_INT:
			return cr_receiveint(rhnd, line);
		case CR_BULK:
			return cr_receivebulk(rhnd, line);
		case CR_MULTIBULK:
			return cr_receivemultibulk(rhnd, line);
		}
	}

	return CREDIS_ERR_RECV;
}

static void
cr_delete(REDIS rhnd)
{
	if (rhnd->reply.multibulk.bulks != NULL)
		free(rhnd->reply.multibulk.bulks);
	if (rhnd->reply.multibulk.idxs != NULL)
		free(rhnd->reply.multibulk.idxs);
	if (rhnd->buf.data != NULL)
		free(rhnd->buf.data);
	if (rhnd->ip != NULL)
		free(rhnd->ip);
	if (rhnd != NULL)
		free(rhnd);
}

REDIS
cr_new(void)
{
	REDIS rhnd;

	if ((rhnd = calloc(sizeof(cr_redis), 1)) == NULL ||
	    (rhnd->ip = malloc(32)) == NULL ||
	    (rhnd->buf.data = malloc(CR_BUFFER_SIZE)) == NULL ||
	    (rhnd->reply.multibulk.bulks = malloc(
		 sizeof(char *) * CR_MULTIBULK_SIZE)) == NULL ||
	    (rhnd->reply.multibulk.idxs = malloc(
		 sizeof(int) * CR_MULTIBULK_SIZE)) == NULL) {
		cr_delete(rhnd);
		return NULL;
	}

	rhnd->buf.size = CR_BUFFER_SIZE;
	rhnd->reply.multibulk.size = CR_MULTIBULK_SIZE;

	return rhnd;
}

/* Send message that has been prepared in message buffer prior to the call
 * to this function. Wait and receive reply. */
static int
cr_sendandreceive(REDIS rhnd, char recvtype)
{
	int rc;

	DEBUG("Sending message: len=%d, data=%s", rhnd->buf.len,
	    rhnd->buf.data);

	rc = cr_senddata(rhnd->fd, rhnd->timeout, rhnd->buf.data,
	    rhnd->buf.len);

	if (rc != rhnd->buf.len) {
		if (rc < 0)
			return CREDIS_ERR_SEND;
		return CREDIS_ERR_TIMEOUT;
	}

	return cr_receivereply(rhnd, recvtype);
}

int
credis_raw_sendandreceive(REDIS rhnd, char recvtype)
{
	return (cr_sendandreceive(rhnd, recvtype));
}

/* Prepare message buffer for sending using a printf()-style formatting. */
__attribute__((format(printf, 3, 4))) int
cr_sendfandreceive(REDIS rhnd, char recvtype, const char *format, ...)
{
	int rc;
	va_list ap;
	cr_buffer *buf = &(rhnd->buf);

	va_start(ap, format);
	rc = vsnprintf(buf->data, buf->size, format, ap);
	va_end(ap);

	if (rc < 0)
		return -1;

	if (rc >= buf->size) {
		DEBUG("truncated, get more memory and try again");
		if (cr_moremem(buf, rc - buf->size + 1))
			return CREDIS_ERR_NOMEM;

		va_start(ap, format);
		rc = vsnprintf(buf->data, buf->size, format, ap);
		va_end(ap);
	}

	buf->len = rc;

	return cr_sendandreceive(rhnd, recvtype);
}

char *
credis_errorreply(REDIS rhnd)
{
	return rhnd->reply.line;
}

void
credis_close(REDIS rhnd)
{
	if (rhnd) {
		if (rhnd->fd > 0)
			close(rhnd->fd);
#ifdef WIN32
		WSACleanup();
#endif
		cr_delete(rhnd);
	}
}

int
credis_reconnect(REDIS rhnd)
{
	int fd, flags, yes = 1, err;
	struct sockaddr_in sa;

	inet_aton(rhnd->ip, &sa.sin_addr);
	sa.sin_family = AF_INET;
	sa.sin_port = htons(rhnd->port);

#ifdef WIN32
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ||
	    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const char *)&yes,
		sizeof(yes)) == -1 ||
	    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&yes,
		sizeof(yes)) == -1)
		return (-1);
#else
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ||
	    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&yes,
		sizeof(yes)) == -1 ||
	    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&yes,
		sizeof(yes)) == -1)
		return (-1);
#endif

	/* connect with user specified timeout */
	flags = fcntl(fd, F_GETFL);
	if ((err = fcntl(fd, F_SETFL, flags | O_NONBLOCK)) < 0) {
		DEBUG("Setting socket non-blocking failed with: %d\n", err);
	}

	if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
		if (errno != EINPROGRESS)
			return (-1);

		if (cr_selectwritable(fd, rhnd->timeout) > 0) {
			unsigned int len = sizeof(err);
			if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) ==
				-1 ||
			    err)
				return (-1);
		} else
			return (-1); /* timeout or select error */
	}

	rhnd->fd = fd;

	return (0);
}

REDIS
credis_connect(const char *host, int port, int timeout)
{
	int use_he = 0;
	struct sockaddr_in sa;
	struct hostent *he;
	REDIS rhnd;

#ifdef WIN32
	unsigned long addr;
	WSADATA data;

	if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
		DEBUG("Failed to init Windows Sockets DLL\n");
		return NULL;
	}
#endif

	if ((rhnd = cr_new()) == NULL)
		return NULL;

	if (host == NULL)
		host = "127.0.0.1";
	if (port == 0)
		port = 6379;

#ifdef WIN32
	/* TODO use getaddrinfo() instead! */
	addr = inet_addr(host);
	if (addr == INADDR_NONE) {
		he = gethostbyname(host);
		use_he = 1;
	} else {
		he = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);
		use_he = 1;
	}
#else
	if (inet_aton(host, &sa.sin_addr) == 0) {
		he = gethostbyname(host);
		use_he = 1;
	}
#endif

	if (use_he) {
		if (he == NULL)
			goto error;
		memcpy(&sa.sin_addr, he->h_addr, sizeof(struct in_addr));
	}

	strcpy(rhnd->ip, inet_ntoa(sa.sin_addr));
	rhnd->port = port;
	rhnd->timeout = timeout;

	if (credis_reconnect(rhnd) != 0)
		goto error;

	/* We can receive 2 version formats: x.yz and x.y.z, where x.yz was only
	 * used prior first 1.1.0 release(?), e.g. stable releases 1.02
	 * and 1.2.6 */
	if (cr_sendfandreceive(rhnd, CR_BULK, "INFO\r\n") == 0) {
		int items = sscanf(rhnd->reply.bulk,
		    "redis_version:%d.%d.%d\r\n", &(rhnd->version.major),
		    &(rhnd->version.minor), &(rhnd->version.patch));

		if (items < 2)
			goto error;
		if (items == 2) {
			rhnd->version.patch = rhnd->version.minor;
			rhnd->version.minor = 0;
		}
		DEBUG("Connected to Redis version: %d.%d.%d\n",
		    rhnd->version.major, rhnd->version.minor,
		    rhnd->version.patch);
	}

	return rhnd;

error:
	if (rhnd->fd > 0)
		close(rhnd->fd);
	cr_delete(rhnd);

	return NULL;
}

void
credis_settimeout(REDIS rhnd, int timeout)
{
	rhnd->timeout = timeout;
}

/*
int credis_set(REDIS rhnd, const char *key, const char *val) {
  return cr_sendfandreceive(rhnd, CR_INLINE,
"*3\r\n$3SET\r\n$%zu\r\n%s\r\n$%zu\r\n%s\r\n", strlen(key), key, strlen(val),
val);
}

int credis_hset(REDIS rhnd, const char *hash, const char *key, const char *val)
{ return cr_sendfandreceive(rhnd, CR_INT,
"*4\r\n$4\r\nHSET\r\n$%zu\r\n%s\r\n$%zu\r\n%s\r\n$%zu\r\n%s\r\n", strlen(hash),
hash, strlen(key), key, strlen(val), val);
}

int credis_get(REDIS rhnd, const char *key, char **val) {
  int rc = cr_sendfandreceive(rhnd, CR_BULK, "*2\r\n$3\r\nGET\r\n%zu\r\n%s\r\n",
strlen(key), key);

  if (rc == 0 && (*val = rhnd->reply.bulk) == NULL)
    return -1;

  return rc;
}

int credis_hget(REDIS rhnd, const char *hash, const char *key, char **val)
{
  int rc = cr_sendfandreceive(rhnd, CR_BULK,
"*3\r\n$4\r\nHGET\r\n$%zu\r\n%s\r\n$%zu\r\n%s\r\n", strlen(hash), hash,
strlen(key), key); if (rc == 0 && (*val = rhnd->reply.bulk) == NULL) return -1;

  return rc;
}

int credis_getset(REDIS rhnd, const char *key, const char *set_val, char
**get_val){ int rc = cr_sendfandreceive(rhnd, CR_BULK,
"$3\r\n$6\r\nGETSET\r\n$%zu\r\n%s\r\n$%zu\r\n%s\r\n", strlen(key), key,
strlen(set_val), set_val);

  if (rc == 0 && (*get_val = rhnd->reply.bulk) == NULL) return -1;

  return rc;
}

int credis_ping(REDIS rhnd) {
  return cr_sendfandreceive(rhnd, CR_INLINE, "PING\r\n");
}
*/

int
credis_auth(REDIS rhnd, const char *password)
{
	int rc;
	char *buf = malloc(strlen(password) + 25);
	sprintf(buf, "*2\r\n$4\r\nAUTH\r\n$%zu\r\n%s\r\n", strlen(password),
	    password);

	rc = cr_senddata(rhnd->fd, rhnd->timeout, buf, strlen(buf));
	if (rc != strlen(buf)) {
		free(buf);

		if (rc < 0)
			return CREDIS_ERR_SEND;
		return CREDIS_ERR_TIMEOUT;
	}
	free(buf);

	return cr_receivereply(rhnd, CR_INLINE);
}

/*
static int cr_multikeybulkcommand(REDIS rhnd, const char *cmd, int keyc, const
char **keyv, char ***valv){ cr_buffer *buf = &(rhnd->buf); int rc;

  buf->len = 0;
  if ((rc = cr_appendstr(buf, cmd, 0)) != 0) return rc;

  if ((rc = cr_appendstrarray(buf, keyc, keyv, 1)) != 0) return rc;

  if ((rc = cr_sendandreceive(rhnd, CR_MULTIBULK)) == 0) {
    *valv = rhnd->reply.multibulk.bulks;
    rc = rhnd->reply.multibulk.len;
  }

  return rc;
}

static int cr_multikeybulkcommand2(REDIS rhnd, const char *cmd, const char
*hash, int keyc, const char **keyv, char ***valv){ cr_buffer *buf =
&(rhnd->buf); int rc, i;

  buf->len = 0;

  if ((rc = cr_appendstrf(buf, "*%zu\r\n$%zu\r\n%s\r\n$%zu\r\n%s\r\n", 4+keyc,
strlen(cmd), cmd, strlen(hash), hash)) != 0) return(rc);

  for (i = 0; i < keyc; i++) {
    if ((rc = cr_appendstrf(buf, "$%zu\r\n%s\r\n", strlen(keyv[i]), keyv[i])) !=
0) return rc;
  }

  if ((rc = cr_sendandreceive(rhnd, CR_MULTIBULK)) == 0) {
    *valv = rhnd->reply.multibulk.bulks;
    rc = rhnd->reply.multibulk.len;
  }

  return rc;
}

static int cr_multikeystorecommand(REDIS rhnd, const char *cmd, const char
*destkey, int keyc, const char **keyv)
{
  cr_buffer *buf = &(rhnd->buf);
  int rc;

  buf->len = 0;

  if ((rc = cr_appendstr(buf, cmd, 0)) != 0) return rc;
  if ((rc = cr_appendstr(buf, destkey, 1)) != 0) return rc;
  if ((rc = cr_appendstrarray(buf, keyc, keyv, 1)) != 0) return rc;

  return cr_sendandreceive(rhnd, CR_INLINE);
}

int credis_mget(REDIS rhnd, int keyc, const char **keyv, char ***valv) {
  return cr_multikeybulkcommand(rhnd, "MGET", keyc, keyv, valv);
}

int credis_mhget(REDIS rhnd, int keyc, const char *hash, const char **keyv, char
***valv) { return cr_multikeybulkcommand2(rhnd, "MGET", hash, keyc, keyv, valv);
}

int credis_setnx(REDIS rhnd, const char *key, const char *val) {
  int rc = cr_sendfandreceive(rhnd, CR_INT,
"*3\r\n$5\r\nSETNX\r\n$%zu\r\n%s\r\n$%zu\r\n%s\r\n", strlen(key), key,
strlen(val), val);

  if (rc == 0 && rhnd->reply.integer == 0) rc = -1;
  return rc;
}

static int cr_incr(REDIS rhnd, int incr, int decr, const char *key, int
*new_val) { int rc = 0;

  if (incr == 1 || decr == 1)
    rc = cr_sendfandreceive(rhnd, CR_INT, "%s %s\r\n", incr>0?"INCR":"DECR",
key); else if (incr > 1 || decr > 1) rc = cr_sendfandreceive(rhnd, CR_INT, "%s
%s %d\r\n", incr>0?"INCRBY":"DECRBY", key, incr>0?incr:decr);

  if (rc == 0 && new_val != NULL)
    *new_val = rhnd->reply.integer;

  return rc;
}

int credis_incr(REDIS rhnd, const char *key, int *new_val) {
  return cr_incr(rhnd, 1, 0, key, new_val);
}

int credis_decr(REDIS rhnd, const char *key, int *new_val) {
  return cr_incr(rhnd, 0, 1, key, new_val);
}

int credis_incrby(REDIS rhnd, const char *key, int incr_val, int *new_val) {
  return cr_incr(rhnd, incr_val, 0, key, new_val);
}

int credis_decrby(REDIS rhnd, const char *key, int decr_val, int *new_val) {
  return cr_incr(rhnd, 0, decr_val, key, new_val);
}

int credis_append(REDIS rhnd, const char *key, const char *val) {
  int rc = cr_sendfandreceive(rhnd, CR_INT,
"*3\r\n$6APPEND\r\n$%zu\r\n%s\r\n$%zu\r\n%s\r\n", strlen(key), key, strlen(val),
val);

  if (rc == 0) rc = rhnd->reply.integer;

  return rc;
}

int credis_substr(REDIS rhnd, const char *key, int start, int end, char
**substr){ int rc = cr_sendfandreceive(rhnd, CR_BULK, "SUBSTR %s %d %d\r\n",
key, start, end);

  if (rc == 0 && substr)
    *substr = rhnd->reply.bulk;

  return rc;
}

int credis_exists(REDIS rhnd, const char *key) {
  int rc = cr_sendfandreceive(rhnd, CR_INT,
"*2\r\n$6\r\nEXISTS\r\n$%zu\r\n%s\r\n", strlen(key), key);

  if (rc == 0 && rhnd->reply.integer == 0)
    rc = -1;

  return rc;
}

int credis_hexists(REDIS rhnd, const char *hash, const char *key) {
  int rc = cr_sendfandreceive(rhnd, CR_INT,
"*3\r\n$7\r\nHEXISTS\r\n$%zu\r\n%s\r\n$%zu\r\n%s\r\n", strlen(key), key,
strlen(hash), hash);

  if (rc == 0 && rhnd->reply.integer == 0) rc = -1;

  return rc;
}

int credis_del(REDIS rhnd, const char *key) {
  int rc = cr_sendfandreceive(rhnd, CR_INT, "*2\r\n$3\r\nDEL\r\n$%zu\r\n%s\r\n",
strlen(key), key);

  if (rc == 0 && rhnd->reply.integer == 0) rc = -1;

  return rc;
}

int credis_hdel(REDIS rhnd, const char *hash, const char *key){
  int rc = cr_sendfandreceive(rhnd, CR_INT,
"*2\r\n$4\r\nHDEL\r\n$%zu\r\n%s\r\n$%zu\r\n%s\r\n", strlen(key), key,
strlen(hash), hash);

  if (rc == 0 && rhnd->reply.integer == 0) rc = -1;

  return rc;
}

int credis_type(REDIS rhnd, const char *key) {
  int rc = cr_sendfandreceive(rhnd, CR_INLINE, "TYPE %s\r\n", key);

  if (rc == 0) {
    char *t = rhnd->reply.line;
    if (!strcmp("string", t)) rc = CREDIS_TYPE_STRING;
    else if (!strcmp("list", t)) rc = CREDIS_TYPE_LIST;
    else if (!strcmp("set", t)) rc = CREDIS_TYPE_SET;
    else rc = CREDIS_TYPE_NONE;
  }

  return rc;
}

int credis_keys(REDIS rhnd, const char *pattern, char ***keyv) {
  int rc = cr_sendfandreceive(rhnd, CR_BULK, "KEYS %s\r\n", pattern);

  if (rc == 0) {
    // server returns keys as space-separated strings, use multi-bulk
    // storage to store keys
    if ((rc = cr_splitstrtromultibulk(rhnd, rhnd->reply.bulk, ' ')) == 0) {
      *keyv = rhnd->reply.multibulk.bulks;
      rc = rhnd->reply.multibulk.len;
    }
  }

  return rc;
}

int credis_randomkey(REDIS rhnd, char **key)
{
  int rc = cr_sendfandreceive(rhnd, CR_INLINE, "RANDOMKEY\r\n");

  if (rc == 0 && key) *key = rhnd->reply.line;

  return rc;
}

int credis_rename(REDIS rhnd, const char *key, const char *new_key_name) {
  return cr_sendfandreceive(rhnd, CR_INLINE, "RENAME %s %s\r\n", key,
new_key_name);
}

int credis_renamenx(REDIS rhnd, const char *key, const char *new_key_name)
{
  int rc = cr_sendfandreceive(rhnd, CR_INT, "RENAMENX %s %s\r\n",
			      key, new_key_name);

  if (rc == 0 && rhnd->reply.integer == 0)
    rc = -1;

  return rc;
}

int credis_dbsize(REDIS rhnd) {
  int rc = cr_sendfandreceive(rhnd, CR_INT, "DBSIZE\r\n");

  if (rc == 0)
    rc = rhnd->reply.integer;

  return rc;
}

int credis_expire(REDIS rhnd, const char *key, int secs) {
  int rc = cr_sendfandreceive(rhnd, CR_INT, "EXPIRE %s %d\r\n", key, secs);

  if (rc == 0 && rhnd->reply.integer == 0)
    rc = -1;

  return rc;
}

int credis_ttl(REDIS rhnd, const char *key) {
  int rc = cr_sendfandreceive(rhnd, CR_INT, "*2\r\n$3\r\nTTL\r\n%zu\r\n%s\r\n",
strlen(key), key);

  if (rc == 0) rc = rhnd->reply.integer;

  return rc;
}

static int cr_push(REDIS rhnd, int left, const char *key, const char *val) {
	int rc=cr_sendfandreceive(rhnd, CR_INT,
"*3\r\n$5\r\n%s\r\n$%zu\r\n%s\r\n$%zu\r\n%s\r\n", left==1?"LPUSH":"RPUSH",
strlen(key), key, strlen(val), val);

	if (rc == 0) rc = rhnd->reply.integer;
	return(rc);
}

int credis_rpush(REDIS rhnd, const char *key, const char *val) {
  return cr_push(rhnd, 0, key, val);
}

int credis_lpush(REDIS rhnd, const char *key, const char *val) {
  return cr_push(rhnd, 1, key, val);
}

int credis_llen(REDIS rhnd, const char *key) {
  int rc = cr_sendfandreceive(rhnd, CR_INT, "LLEN %s\r\n", key);

  if (rc == 0)
    rc = rhnd->reply.integer;

  return rc;
}

int credis_lrange(REDIS rhnd, const char *key, int start, int end, char ***valv)
{
  int rc;

  if ((rc = cr_sendfandreceive(rhnd, CR_MULTIBULK, "LRANGE %s %d %d\r\n",
			       key, start, end)) == 0) {
    *valv = rhnd->reply.multibulk.bulks;
    rc = rhnd->reply.multibulk.len;
  }

  return rc;
}

int credis_ltrim(REDIS rhnd, const char *key, int start, int end)
{
  return cr_sendfandreceive(rhnd, CR_INLINE, "LTRIM %s %d %d\r\n",
			    key, start, end);
}

int credis_lindex(REDIS rhnd, const char *key, int index, char **val)
{
  int rc = cr_sendfandreceive(rhnd, CR_BULK, "LINDEX %s %d\r\n", key, index);

  if (rc == 0 && (*val = rhnd->reply.bulk) == NULL)
    return -1;

  return rc;
}

int credis_lset(REDIS rhnd, const char *key, int index, const char *val) {
  return cr_sendfandreceive(rhnd, CR_INLINE, "LSET %s %d %zu\r\n%s\r\n", key,
index, strlen(val), val);
}

int credis_lrem(REDIS rhnd, const char *key, int count, const char *val) {
  return cr_sendfandreceive(rhnd, CR_INT, "LREM %s %d %zu\r\n%s\r\n", key,
count, strlen(val), val);
}

static int cr_pop(REDIS rhnd, int left, const char *key, char **val) {
  int rc = cr_sendfandreceive(rhnd, CR_BULK, "*2\r\n$4\r\n%s\r\n$%zu\r\n%s\r\n",
left==1?"LPOP":"RPOP", strlen(key), key);

  if (rc == 0 && (*val = rhnd->reply.bulk) == NULL) return -1;

  return rc;
}
*/

static int
cr_bpop(REDIS rhnd, int left, const char *key, const char *seconds, char **val)
{
	int rc = cr_sendfandreceive(rhnd, CR_MULTIBULK,
	    "*3\r\n$5\r\n%s\r\n$%zu\r\n%s\r\n$%zu\r\n%s\r\n",
	    left == 1 ? "BLPOP" : "BRPOP", strlen(key), key, strlen(seconds),
	    seconds);

	if (rc == 0) {
		if (rhnd->reply.multibulk.len != 2)
			return (0);

		*val = rhnd->reply.multibulk.bulks[1];
		rc = strlen(*val);
	}

	return rc;
}

int
credis_blpop(REDIS rhnd, const char *key, const char *seconds, char **val)
{
	return cr_bpop(rhnd, 1, key, seconds, val);
}

int
credis_brpop(REDIS rhnd, const char *key, const char *seconds, char **val)
{
	return cr_bpop(rhnd, 0, key, seconds, val);
}

/*
int credis_lpop(REDIS rhnd, const char *key, char **val) {
  return cr_pop(rhnd, 1, key, val);
}

int credis_rpop(REDIS rhnd, const char *key, char **val) {
  return cr_pop(rhnd, 0, key, val);
}
*/

int
credis_select(REDIS rhnd, int index)
{
	int rc;
	char *buf = malloc(40);
	sprintf(buf, "SELECT %d\r\n", index);
	rc = cr_senddata(rhnd->fd, rhnd->timeout, buf, strlen(buf));

	if (rc != strlen(buf)) {
		free(buf);
		if (rc < 0)
			return CREDIS_ERR_SEND;
		return CREDIS_ERR_TIMEOUT;
	}
	free(buf);

	return cr_receivereply(rhnd, CR_INLINE);
}

/*
int credis_move(REDIS rhnd, const char *key, int index) {
  int rc = cr_sendfandreceive(rhnd, CR_INT, "MOVE %s %d\r\n", key, index);

  if (rc == 0 && rhnd->reply.integer == 0) rc = -1;

  return rc;
}

int credis_flushdb(REDIS rhnd) {
  return cr_sendfandreceive(rhnd, CR_INLINE, "FLUSHDB\r\n");
}

int credis_flushall(REDIS rhnd) {
  return cr_sendfandreceive(rhnd, CR_INLINE, "FLUSHALL\r\n");
}

int credis_sort(REDIS rhnd, const char *query, char ***elementv) {
  int rc;

  if ((rc = cr_sendfandreceive(rhnd, CR_MULTIBULK, "SORT %s\r\n", query)) == 0)
{ *elementv = rhnd->reply.multibulk.bulks; rc = rhnd->reply.multibulk.len;
  }

  return rc;
}

int credis_save(REDIS rhnd){
  return cr_sendfandreceive(rhnd, CR_INLINE, "SAVE\r\n");
}

int credis_bgsave(REDIS rhnd){
  return cr_sendfandreceive(rhnd, CR_INLINE, "BGSAVE\r\n");
}

int credis_lastsave(REDIS rhnd) {
  int rc = cr_sendfandreceive(rhnd, CR_INT, "LASTSAVE\r\n");

  if (rc == 0) rc = rhnd->reply.integer;

  return rc;
}

int credis_shutdown(REDIS rhnd) {
  return cr_sendfandreceive(rhnd, CR_INLINE, "SHUTDOWN\r\n");
}

int credis_bgrewriteaof(REDIS rhnd) {
  return cr_sendfandreceive(rhnd, CR_INLINE, "BGREWRITEAOF\r\n");
}


// Parse Redis `info' string for a particular `field', storing its value to
// `storage' according to `format'.
//
void cr_parseinfo(const char *info, const char *field, const char *format, void
*storage)
{
  char *str = strstr(info, field);
  if (str) {
    str += strlen(field) + 1; // also skip the ':'
    sscanf(str, format, storage);
  }
}

int credis_info(REDIS rhnd, REDIS_INFO *info)
{
  int rc = cr_sendfandreceive(rhnd, CR_BULK, "INFO\r\n");

  if (rc == 0) {
    char role;
    memset(info, 0, sizeof(REDIS_INFO));
    cr_parseinfo(rhnd->reply.bulk, "redis_version",
"%"CR_VERSION_STRING_SIZE_STR"s\r\n", &(info->redis_version));
    cr_parseinfo(rhnd->reply.bulk, "arch_bits", "%d", &(info->arch_bits));
    cr_parseinfo(rhnd->reply.bulk, "multiplexing_api",
"%"CR_MULTIPLEXING_API_SIZE_STR"s\r\n", &(info->multiplexing_api));
    cr_parseinfo(rhnd->reply.bulk, "process_id", "%ld", &(info->process_id));
    cr_parseinfo(rhnd->reply.bulk, "uptime_in_seconds", "%ld",
&(info->uptime_in_seconds)); cr_parseinfo(rhnd->reply.bulk, "uptime_in_days",
"%ld", &(info->uptime_in_days)); cr_parseinfo(rhnd->reply.bulk,
"connected_clients", "%d", &(info->connected_clients));
    cr_parseinfo(rhnd->reply.bulk, "connected_slaves", "%d",
&(info->connected_slaves)); cr_parseinfo(rhnd->reply.bulk, "blocked_clients",
"%d", &(info->blocked_clients)); cr_parseinfo(rhnd->reply.bulk, "used_memory",
"%zu", &(info->used_memory)); cr_parseinfo(rhnd->reply.bulk,
"used_memory_human", "%"CR_USED_MEMORY_HUMAN_SIZE_STR"s",
&(info->used_memory_human)); cr_parseinfo(rhnd->reply.bulk,
"changes_since_last_save", "%lld", &(info->changes_since_last_save));
    cr_parseinfo(rhnd->reply.bulk, "bgsave_in_progress", "%d",
&(info->bgsave_in_progress)); cr_parseinfo(rhnd->reply.bulk, "last_save_time",
"%ld", &(info->last_save_time)); cr_parseinfo(rhnd->reply.bulk,
"bgrewriteaof_in_progress", "%d", &(info->bgrewriteaof_in_progress));
    cr_parseinfo(rhnd->reply.bulk, "total_connections_received", "%lld",
&(info->total_connections_received)); cr_parseinfo(rhnd->reply.bulk,
"total_commands_processed", "%lld", &(info->total_commands_processed));
    cr_parseinfo(rhnd->reply.bulk, "expired_keys", "%lld",
&(info->expired_keys)); cr_parseinfo(rhnd->reply.bulk,
"hash_max_zipmap_entries", "%zu", &(info->hash_max_zipmap_entries));
    cr_parseinfo(rhnd->reply.bulk, "hash_max_zipmap_value", "%zu",
&(info->hash_max_zipmap_value)); cr_parseinfo(rhnd->reply.bulk,
"pubsub_channels", "%ld", &(info->pubsub_channels));
    cr_parseinfo(rhnd->reply.bulk, "pubsub_patterns", "%u",
&(info->pubsub_patterns)); cr_parseinfo(rhnd->reply.bulk, "vm_enabled", "%d",
&(info->vm_enabled)); cr_parseinfo(rhnd->reply.bulk, "role", "%c", &role);

    info->role = ((role=='m')?CREDIS_SERVER_MASTER:CREDIS_SERVER_SLAVE);
  }

  return rc;
}

int credis_monitor(REDIS rhnd) {
  return cr_sendfandreceive(rhnd, CR_INLINE, "MONITOR\r\n");
}

int credis_slaveof(REDIS rhnd, const char *host, int port) {
  if (host == NULL || port == 0)
    return cr_sendfandreceive(rhnd, CR_INLINE, "SLAVEOF no one\r\n");
  else
    return cr_sendfandreceive(rhnd, CR_INLINE, "SLAVEOF %s %d\r\n", host, port);
}

static int cr_setaddrem(REDIS rhnd, const char *cmd, const char *key, const char
*member) { int rc = cr_sendfandreceive(rhnd, CR_INT, "%s %s %zu\r\n%s\r\n", cmd,
key, strlen(member), member);

  if (rc == 0 && rhnd->reply.integer == 0) rc = -1;

  return rc;
}

int credis_sadd(REDIS rhnd, const char *key, const char *member) {
  return cr_setaddrem(rhnd, "SADD", key, member);
}

int credis_srem(REDIS rhnd, const char *key, const char *member) {
  return cr_setaddrem(rhnd, "SREM", key, member);
}

int credis_spop(REDIS rhnd, const char *key, char **member) {
  int rc = cr_sendfandreceive(rhnd, CR_BULK, "SPOP %s\r\n", key);

  if (rc == 0 && (*member = rhnd->reply.bulk) == NULL) rc = -1;

  return rc;
}

int credis_smove(REDIS rhnd, const char *sourcekey, const char *destkey,  const
char *member) { int rc = cr_sendfandreceive(rhnd, CR_INT, "SMOVE %s %s %s\r\n",
			      sourcekey, destkey, member);

  if (rc == 0 && rhnd->reply.integer == 0) rc = -1;

  return rc;
}

int credis_scard(REDIS rhnd, const char *key) {
  int rc = cr_sendfandreceive(rhnd, CR_INT, "SCARD %s\r\n", key);

  if (rc == 0) rc = rhnd->reply.integer;

  return rc;
}

int credis_sinter(REDIS rhnd, int keyc, const char **keyv, char ***members) {
  return cr_multikeybulkcommand(rhnd, "SINTER", keyc, keyv, members);
}

int credis_sunion(REDIS rhnd, int keyc, const char **keyv, char ***members) {
  return cr_multikeybulkcommand(rhnd, "SUNION", keyc, keyv, members);
}

int credis_sdiff(REDIS rhnd, int keyc, const char **keyv, char ***members) {
  return cr_multikeybulkcommand(rhnd, "SDIFF", keyc, keyv, members);
}

int credis_sinterstore(REDIS rhnd, const char *destkey, int keyc, const char
**keyv) { return cr_multikeystorecommand(rhnd, "SINTERSTORE", destkey, keyc,
keyv);
}

int credis_sunionstore(REDIS rhnd, const char *destkey, int keyc, const char
**keyv) { return cr_multikeystorecommand(rhnd, "SUNIONSTORE", destkey, keyc,
keyv);
}

int credis_sdiffstore(REDIS rhnd, const char *destkey, int keyc, const char
**keyv) { return cr_multikeystorecommand(rhnd, "SDIFFSTORE", destkey, keyc,
keyv);
}

int credis_sismember(REDIS rhnd, const char *key, const char *member) {
  return cr_setaddrem(rhnd, "SISMEMBER", key, member);
}

int credis_smembers(REDIS rhnd, const char *key, char ***members) {
  return cr_multikeybulkcommand(rhnd, "SMEMBERS", 1, &key, members);
}

int credis_zadd(REDIS rhnd, const char *key, double score, const char *member) {
  int rc = cr_sendfandreceive(rhnd, CR_INT, "ZADD %s %f %zu\r\n%s\r\n", key,
score, strlen(member), member);

  if (rc == 0 && rhnd->reply.integer == 0) rc = -1;

  return rc;
}

int credis_zrem(REDIS rhnd, const char *key, const char *member) {
  int rc = cr_sendfandreceive(rhnd, CR_INT, "ZREM %s %zu\r\n%s\r\n", key,
strlen(member), member);

  if (rc == 0 && rhnd->reply.integer == 0) rc = -1;

  return rc;
}

// TODO what does Redis return if member is not member of set?
int credis_zincrby(REDIS rhnd, const char *key, double incr_score, const char
*member, double *new_score)
{
  int rc = cr_sendfandreceive(rhnd, CR_BULK, "ZINCRBY %s %f %zu\r\n%s\r\n",
			      key, incr_score, strlen(member), member);

  if (rc == 0 && new_score)
    *new_score = strtod(rhnd->reply.bulk, NULL);

  return rc;
}

// TODO what does Redis return if member is not member of set?
static int cr_zrank(REDIS rhnd, int reverse, const char *key, const char
*member)
{
  int rc = cr_sendfandreceive(rhnd, CR_BULK, "%s %s %zu\r\n%s\r\n",
			      reverse==1?"ZREVRANK":"ZRANK", key,
strlen(member), member);

  if (rc == 0) rc = atoi(rhnd->reply.bulk);

  return rc;
}

int credis_zrank(REDIS rhnd, const char *key, const char *member) {
  return cr_zrank(rhnd, 0, key, member);
}

int credis_zrevrank(REDIS rhnd, const char *key, const char *member) {
  return cr_zrank(rhnd, 1, key, member);
}

int cr_zrange(REDIS rhnd, int reverse, const char *key, int start, int end, char
***elementv) { int rc = cr_sendfandreceive(rhnd, CR_MULTIBULK, "%s %s %d
%d\r\n", reverse==1?"ZREVRANGE":"ZRANGE", key, start, end);

  if (rc == 0) {
    *elementv = rhnd->reply.multibulk.bulks;
    rc = rhnd->reply.multibulk.len;
  }

  return rc;
}

int credis_zrange(REDIS rhnd, const char *key, int start, int end, char
***elementv) { return cr_zrange(rhnd, 0, key, start, end, elementv);
}

int credis_zrevrange(REDIS rhnd, const char *key, int start, int end, char
***elementv) { return cr_zrange(rhnd, 1, key, start, end, elementv);
}

int credis_zcard(REDIS rhnd, const char *key) {
  int rc = cr_sendfandreceive(rhnd, CR_INT, "ZCARD %s\r\n", key);

  if (rc == 0) {
    if (rhnd->reply.integer == 0)
      rc = -1;
    else
      rc = rhnd->reply.integer;
  }

  return rc;
}

int credis_zscore(REDIS rhnd, const char *key, const char *member, double
*score) { int rc = cr_sendfandreceive(rhnd, CR_BULK, "ZSCORE %s %zu\r\n%s\r\n",
			      key, strlen(member), member);

  if (rc == 0) {
    if (!rhnd->reply.bulk)
      rc = -1;
    else if (score)
      *score = strtod(rhnd->reply.bulk, NULL);
  }

  return rc;
}

int credis_zremrangebyscore(REDIS rhnd, const char *key, double min, double max)
{ int rc = cr_sendfandreceive(rhnd, CR_INT, "ZREMRANGEBYSCORE %s %f %f\r\n",
			      key, min, max);

  if (rc == 0)
    rc = rhnd->reply.integer;

  return rc;
}

int credis_zremrangebyrank(REDIS rhnd, const char *key, int start, int end) {
  int rc = cr_sendfandreceive(rhnd, CR_INT, "ZREMRANGEBYRANK %s %d %d\r\n",
			      key, start, end);

  if (rc == 0)
    rc = rhnd->reply.integer;

  return rc;
}

// TODO add writev() support instead and push strings to send onto a vector of
// strings to send instead...
static int cr_zstore(REDIS rhnd, int inter, const char *destkey, int keyc, const
char **keyv, const int *weightv, REDIS_AGGREGATE aggregate)
{
  cr_buffer *buf = &(rhnd->buf);
  int rc, i;

  buf->len = 0;

  if ((rc = cr_appendstrf(buf, "%s %s %d ", inter?"ZINTERSTORE":"ZUNIONSTORE",
destkey, keyc)) != 0) return rc; if ((rc = cr_appendstrarray(buf, keyc, keyv,
0)) != 0) return rc; if (weightv != NULL) for (i = 0; i < keyc; i++) if ((rc =
cr_appendstrf(buf, " %d", weightv[i])) != 0) return rc;

  switch (aggregate) {
  case SUM:
    rc = cr_appendstr(buf, "AGGREGATE SUM", 0);
    break;
  case MIN:
    rc = cr_appendstr(buf, "AGGREGATE MIN", 0);
    break;
  case MAX:
    rc = cr_appendstr(buf, "AGGREGATE MAX", 0);
    break;
  case NONE:
    ; // avoiding compiler warning
  }
  if (rc != 0)
    return rc;

  if ((rc = cr_appendstr(buf, "\r\n", 0)) != 0)
    return rc;

  if ((rc = cr_sendandreceive(rhnd, CR_INT)) == 0)
    rc = rhnd->reply.integer;

  return rc;
}

int credis_zinterstore(REDIS rhnd, const char *destkey, int keyc, const char
**keyv,  const int *weightv, REDIS_AGGREGATE aggregate){ return cr_zstore(rhnd,
1, destkey, keyc, keyv, weightv, aggregate);
}

int credis_zunionstore(REDIS rhnd, const char *destkey, int keyc, const char
**keyv,  const int *weightv, REDIS_AGGREGATE aggregate){ return cr_zstore(rhnd,
0, destkey, keyc, keyv, weightv, aggregate);
}

*/
