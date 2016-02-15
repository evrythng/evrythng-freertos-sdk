/*
 * (c) Copyright 2012 EVRYTHNG Ltd London / Zurich
 * www.evrythng.com
 */

#include "POSIX/types.h"
#include <stdarg.h>
#include "evrythng_platform.h"

void platform_timer_init(Timer* timer)
{
	timer->end_time = (struct timeval){0, 0};
}


void TimerDeinit(Timer* t)
{
}


char platform_timer_isexpired(Timer* timer)
{
	struct timeval now, res;
	gettimeofday(&now, NULL);
	timersub(&timer->end_time, &now, &res);		
	return res.tv_sec < 0 || (res.tv_sec == 0 && res.tv_usec <= 0);
}


void platform_timer_countdown(Timer* timer, unsigned int timeout)
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


int platform_network_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	struct timeval interval = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
	if (interval.tv_sec < 0 || (interval.tv_sec == 0 && interval.tv_usec <= 0))
	{
		interval.tv_sec = 0;
		interval.tv_usec = 100;
	}

	setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&interval, sizeof(struct timeval));

	int bytes = 0;
	while (bytes < len)
	{
		int rc = recv(n->my_socket, &buffer[bytes], (size_t)(len - bytes), 0);
        if (rc == 0)
        {
            bytes = 0;
            break;
        }
        else if (rc == -1)
		{
			if (errno != ENOTCONN && errno != ECONNRESET)
			{
				bytes = -1;
				break;
			}
		}
		else
			bytes += rc;
	}

    //platform_printf("%s recv bytes = %d\n", __func__, bytes);

	return bytes;
}


int platform_network_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	struct timeval tv;

	tv.tv_sec = 0;  /* 30 Secs Timeout */
	tv.tv_usec = timeout_ms * 1000;  // Not init'ing this can cause strange errors

	setsockopt(n->my_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv,sizeof(struct timeval));
	int	rc = send(n->my_socket, buffer, len, MSG_NOSIGNAL);

    //platform_printf("%s send bytes = %d\n", __func__, rc);

	return rc;
}


void platform_network_init(Network* n)
{
	n->my_socket = 0;
}


int platform_network_connect(Network* n, char* addr, int port)
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
			rc = connect(n->my_socket, (struct sockaddr*)&address, sizeof(address));
	}

	return rc;
}


void platform_network_securedinit(Network* n, const char* ca_buf, size_t ca_size)
{
    //TODO
}


void platform_network_disconnect(Network* n)
{
	close(n->my_socket);
}


void platform_mutex_init(Mutex* m)
{
    if (!m)
    {
        platform_printf("%s: invalid mutex %p\n", __func__, m);
        return;
    }

    pthread_mutex_init(&m->mtx, 0);
}


void platform_mutex_deinit(Mutex* m)
{
    if (!m)
    {
        platform_printf("%s: invalid mutex %p\n", __func__, m);
        return;
    }

    pthread_mutex_destroy(&m->mtx);
}


int platform_mutex_lock(Mutex* m)
{
    if (!m)
    {
        platform_printf("%s: invalid mutex %p\n", __func__, m);
        return -1;
    }

    pthread_mutex_lock(&m->mtx);
    return 0;
}


int platform_mutex_unlock(Mutex* m)
{
    if (!m)
    {
        platform_printf("%s: invalid mutex %p\n", __func__, m);
        return -1;
    }

    pthread_mutex_unlock(&m->mtx);
    return 0;
}


void platform_semaphore_init(Semaphore* s)
{
    if (!s) 
        return;
    sem_init(&s->sem, 0, 0);
}


void platform_semaphore_deinit(Semaphore* s)
{
    if (!s) 
        return;
    sem_destroy(&s->sem);
}


int platform_semaphore_post(Semaphore* s)
{
    if (!s) 
        return -1;
    return sem_post(&s->sem);
}


int platform_semaphore_wait(Semaphore* s, int timeout_ms)
{
    if (!s) 
        return -1;
    
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    int sec = timeout_ms / 1000;
    timeout_ms = timeout_ms - sec * 1000;

    ts.tv_nsec += timeout_ms * 1000000;
    ts.tv_sec += ts.tv_nsec / 1000000000 + sec;
    ts.tv_nsec = ts.tv_nsec % 1000000000;

    do 
    {
        int ret = sem_timedwait(&s->sem, &ts);
        if (!ret) return 0;
        else
        { 
            if (errno == EINTR) continue;
            return -1;
        }
    } while(1);

    return -1;
}

static void* func_wrapper(void* arg)
{
    Thread* t = (Thread*)arg;
    (*t->func)(t->arg);
    return 0;
}

int platform_thread_create(Thread* t, 
        int priority, 
        const char* name, 
        void (*func)(void*), 
        size_t stack_size, void* arg )
{
    if (!t) return -1;
    t->func = func;
    t->arg = arg;
    return pthread_create(&t->tid, 0, func_wrapper, t);
}


int platform_thread_join(Thread* t, int timeout_ms)
{
    if (!t) return -1;
    return pthread_join(t->tid, 0);
}


int platform_thread_destroy(Thread* t)
{
    if (!t) return -1;
    return 0;
}


void* platform_malloc(size_t bytes)
{
    return malloc(bytes);
}


void* platform_realloc(void* ptr, size_t bytes)
{
    return realloc(ptr, bytes);
}


void platform_free(void* memory)
{
    free(memory);
}

int platform_printf(const char* fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);

    char msg[512];
    unsigned n = vsnprintf(msg, sizeof msg, fmt, vl);
    if (n >= sizeof msg)
        msg[sizeof msg - 1] = '\0';

    int rc = printf("%s", msg);

    va_end(vl);

    return rc;
}


void platform_sleep(int ms)
{
    usleep(ms * 1000);
}
