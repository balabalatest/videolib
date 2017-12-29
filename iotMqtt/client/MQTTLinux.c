/*******************************************************************************
 * Copyright (c) 2014, 2017 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander - initial API and implementation and/or initial documentation
 *    Ian Craggs - return codes from linux_read
 *******************************************************************************/

#include "MQTTLinux.h"

void TimerInit(Timer* timer)
{
	timer->end_time = (struct timeval){0, 0};
}
char TimerIsExpired(Timer* timer)
{
	struct timeval now, res;
	gettimeofday(&now, NULL);
	timersub(&timer->end_time, &now, &res);
	return res.tv_sec < 0 || (res.tv_sec == 0 && res.tv_usec <= 0);
}
void TimerCountdownMS(Timer* timer, unsigned int timeout)
{
	struct timeval now;
	gettimeofday(&now, NULL);
	struct timeval interval = {timeout / 1000, (timeout % 1000) * 1000};
	timeradd(&now, &interval, &timer->end_time);
}
void TimerCountdown(Timer* timer, unsigned int timeout)
{
	struct timeval now;
	gettimeofday(&now, NULL);
	struct timeval interval = {timeout, 0};
	timeradd(&now, &interval, &timer->end_time);
}
int TimerLeftMS(Timer* timer)
{
	struct timeval now, res;
	gettimeofday(&now, NULL);
	timersub(&timer->end_time, &now, &res);
	//printf("left %d ms\n", (res.tv_sec < 0) ? 0 : res.tv_sec * 1000 + res.tv_usec / 1000);
	return (res.tv_sec < 0) ? 0 : res.tv_sec * 1000 + res.tv_usec / 1000;
}

#if 0
int linux_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	struct timeval interval = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
	if (interval.tv_sec < 0 || (interval.tv_sec == 0 && interval.tv_usec <= 0))
	{
		interval.tv_sec = 0;
		interval.tv_usec = 100;
	}

	setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (const void *)&interval, sizeof(struct timeval));
	int bytes = 0;
	while (bytes < len)
	{
	  /* liteOS return immediately, as non block */
		int rc = recv(n->my_socket, &buffer[bytes], (size_t)(len - bytes), 0);
		if (rc == -1)
		{
			if (errno != EAGAIN && errno != EWOULDBLOCK)
			  bytes = -1;
			break;
		}
		else if (rc == 0)
		{
			bytes = 0;
			break;
		}
		else
			bytes += rc;
	}
	return bytes;
}
#else
int linux_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
  fd_set readset;
  int ret;
  int sd = n->my_socket;

  struct timeval interval = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
  if (interval.tv_sec < 0 || (interval.tv_sec == 0 && interval.tv_usec <= 0))
  {
    interval.tv_sec = 0;
    interval.tv_usec = 100;
  }

  do {
    FD_ZERO(&readset);
    FD_SET(sd, &readset);
    ret = select(sd+1, &readset, NULL, NULL, &interval);
  } while (ret < 0 && errno == EINTR);

  if (ret <= 0)
    return ret;

  int bytes = 0;
  while (bytes < len)
  {
    ret = recv(sd, &buffer[bytes], (size_t)(len - bytes), 0);

    if (ret < 0) {
      if (errno != EINTR) {
        printf("linux_read_select: recv failed sd=%d: %d\n", sd, errno);
        if (errno != EAGAIN)
          return -1;
      }
    } else if (ret == 0) {
      printf("linux_read_select: Client connection lost sd=%d\n", sd);
      return -1;
    } else {
      bytes += ret;
    }
  }

  return bytes;
}
#endif

int linux_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	struct timeval tv;

	tv.tv_sec = 0;  /* 30 Secs Timeout */
	tv.tv_usec = timeout_ms * 1000;  // Not init'ing this can cause strange errors

	setsockopt(n->my_socket, SOL_SOCKET, SO_SNDTIMEO, (const void *)&tv,sizeof(struct timeval));
	int	rc = write(n->my_socket, buffer, len);
	return rc;
}


void NetworkInit(Network* n)
{
	n->my_socket = 0;
	n->mqttread = linux_read;
	n->mqttwrite = linux_write;
}

#if 0
int NetworkConnect(Network* n, char* addr, int port)
{
	int type = SOCK_STREAM;
	struct sockaddr_in address;
	int rc = -1;
	sa_family_t family = AF_INET;
	struct addrinfo *result = NULL;
	struct addrinfo hints = {0, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};

	if ((rc = getaddrinfo(addr, NULL, &hints, &result)) == 0)
	{
		struct addrinfo* res = result;

		/* prefer ip4 addresses */
		while (res)
		{
			if (res->ai_family == AF_INET)
			{
				result = res;
				break;
			}
			res = res->ai_next;
		}

		if (result->ai_family == AF_INET)
		{
			address.sin_port = htons(port);
			address.sin_family = family = AF_INET;
			address.sin_addr = ((struct sockaddr_in*)(result->ai_addr))->sin_addr;
		}
		else
			rc = -1;

		freeaddrinfo(result);
	}

	if (rc == 0)
	{
		n->my_socket = socket(family, type, 0);
		if (n->my_socket != -1)
			rc = connect(n->my_socket, (struct sockaddr*)&address, sizeof(address));  /* lichen add: maybe very long time when the link is down*/
	}

	return rc;
}
#else

int NetworkConnect(Network* n, char* addr, int port)
{
  struct sockaddr_in address;
  struct hostent *hptr;
  int rc = -1;

  if ((hptr = gethostbyname(addr)) == NULL)
          goto exit;

  address.sin_addr = *((struct in_addr *)hptr->h_addr);
  address.sin_family = AF_INET;
  address.sin_port = htons(port);

  if ((n->my_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
          goto exit;

  if ((rc = connect(n->my_socket, (struct sockaddr*) &address, sizeof(address))) < 0)
  {
          close(n->my_socket);
          goto exit;
  }

exit:
  return rc;

}

#endif

void NetworkDisconnect(Network* n)
{
	close(n->my_socket);
}
