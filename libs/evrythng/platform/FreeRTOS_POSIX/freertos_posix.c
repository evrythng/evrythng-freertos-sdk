/*
 * (c) Copyright 2012 EVRYTHNG Ltd London / Zurich
 * www.evrythng.com
 */

#include <stdarg.h>

#include "platform_types.h"
#include "evrythng/platform.h"

#define BLOCK_SIGNALS \
    sigset_t sigfull, sigold;\
    sigfillset(&sigfull);\
    pthread_sigmask(SIG_SETMASK, &sigfull, &sigold);

#define RESTORE_SIGNAL_MASK \
    pthread_sigmask(SIG_SETMASK, &sigold, 0);

#if 0
void platform_timer_init(Timer* t)
{
    if (!t)
    {
        platform_printf("%s: invalid timer\n", __func__);
        return;
    }

	t->xTicksToWait = 0;
	memset(&t->xTimeOut, '\0', sizeof(t->xTimeOut));
}


void TimerDeinit(Timer* t)
{
    if (!t)
    {
        platform_printf("%s: invalid timer\n", __func__);
        return;
    }
}


char platform_timer_isexpired(Timer* t)
{
    if (!t)
    {
        platform_printf("%s: invalid timer\n", __func__);
        return -1;
    }

	return xTaskCheckForTimeOut(&t->xTimeOut, &t->xTicksToWait) == pdTRUE;
}


void platform_timer_countdown(Timer* t, unsigned int ms)
{
    if (!t)
    {
        platform_printf("%s: invalid timer\n", __func__);
        return;
    }

    t->xTicksToWait = (ms / (portTICK_RATE_MS));
    vTaskSetTimeOutState(&t->xTimeOut); /* Record the time at which this function was entered. */
}


int TimerLeftMS(Timer* t)
{
    if (!t)
    {
        platform_printf("%s: invalid timer\n", __func__);
        return 0;
    }

    /* if true -> timeout, else updates xTicksToWait to the number left */
	if (xTaskCheckForTimeOut(&t->xTimeOut, &t->xTicksToWait) == pdTRUE)
        return 0;
	return (t->xTicksToWait <= 0) ? 0 : (t->xTicksToWait * (portTICK_RATE_MS));
}

#else

void platform_timer_init(Timer* timer)
{
	timer->end_time = (struct timeval){0, 0};
}


void platform_timer_deinit(Timer* t)
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


int platform_timer_left(Timer* timer)
{
	struct timeval now, res;
	gettimeofday(&now, NULL);
	timersub(&timer->end_time, &now, &res);
	//printf("left %d ms\n", (res.tv_sec < 0) ? 0 : res.tv_sec * 1000 + res.tv_usec / 1000);
	return (res.tv_sec < 0) ? 0 : res.tv_sec * 1000 + res.tv_usec / 1000;
}

#endif


void platform_network_init(Network* n)
{
	n->my_socket = 0;
    n->ssl_enabled = 0;
}


void platform_network_securedinit(Network* n, const char* ca_buf, size_t ca_size)
{
    SSL_library_init();
    SSL_load_error_strings();
    ERR_load_BIO_strings();

    n->ca_buf = ca_buf;
    n->ca_size = ca_size;

    n->ssl_enabled = 1;
}


static struct timeval ms_to_timeval(int timeout_ms)
{
	struct timeval timeout = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};

	if (timeout.tv_sec < 0 || (timeout.tv_sec == 0 && timeout.tv_usec <= 0))
	{
		timeout.tv_sec = 0;
		timeout.tv_usec = 100;
	}

    return timeout;
}


static int network_unsecured_read(Network* n, unsigned char* buffer, int len, struct timeval tv)
{
    BLOCK_SIGNALS

	setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof tv);

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

    //printf("%s recv bytes = %d\n", __func__, bytes);

    RESTORE_SIGNAL_MASK

	return bytes;
}


static int network_secured_read(Network* n, unsigned char* buffer, int len, struct timeval tv)
{
    BLOCK_SIGNALS

    int sock;
    BIO_get_fd(n->bio, &sock);
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof tv);

	int bytes = 0;
	while (bytes < len)
	{
        int rc = BIO_read(n->bio, &buffer[bytes], (size_t)(len - bytes));

        if (rc == 0)
        {
            bytes = 0;
            break;
        }
        else if (rc < 0)
		{
            bytes = -1;
            break;
		}
		else
			bytes += rc;
	}

    //printf("%s recv bytes = %d\n", __func__, bytes);

    RESTORE_SIGNAL_MASK

	return bytes;
}


int platform_network_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
    if (!n->ssl_enabled)
        return network_unsecured_read(n, buffer, len, ms_to_timeval(timeout_ms));
    else
        return network_secured_read(n, buffer, len, ms_to_timeval(timeout_ms));
}


static int network_unsecured_write(Network* n, unsigned char* buffer, int len, struct timeval tv)
{
    //BLOCK_SIGNALS

	setsockopt(n->my_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof tv);
	int	rc = send(n->my_socket, buffer, len, MSG_NOSIGNAL);

    //printf("%s sent bytes = %d\n", __func__, rc);

    //RESTORE_SIGNAL_MASK

	return rc;
}


static int network_secured_write(Network* n, unsigned char* buffer, int len, struct timeval tv)
{
    //BLOCK_SIGNALS

    int sock;
    BIO_get_fd(n->bio, &sock);
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof tv);

    int rc = BIO_write(n->bio, buffer, len);

    //printf("%s sent bytes = %d\n", __func__, rc);

    //RESTORE_SIGNAL_MASK

	return rc;
}


int platform_network_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
    if (n->ssl_enabled)
        return network_secured_write(n, buffer, len, ms_to_timeval(timeout_ms));
    else
        return network_unsecured_write(n, buffer, len, ms_to_timeval(timeout_ms));
}


void platform_network_disconnect(Network* n)
{
    if (!n->ssl_enabled)
    {
        close(n->my_socket);
    }
    else
    {
        if (n->bio) BIO_free_all(n->bio);
        if (n->ctx) SSL_CTX_free(n->ctx);

        n->bio = 0;
        n->ctx = 0;
    }
}


static int network_unsecured_connect(Network* n, char* addr, int port)
{
    BLOCK_SIGNALS

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

    RESTORE_SIGNAL_MASK

	return rc;
}


static int network_secured_connect(Network* n, char* addr, int port)
{
    BLOCK_SIGNALS

	int rc = -1;

    BIO* bio_mem = 0;
    X509* cert = 0;

    n->ctx = SSL_CTX_new(SSLv23_client_method());
    if (!n->ctx)
    {
        printf("could not create ssl context: %s\n", ERR_reason_error_string(ERR_get_error()));
        goto exit;
    }

    bio_mem = BIO_new_mem_buf((void*)n->ca_buf, n->ca_size);
    if (!bio_mem)
    {
        printf("could not create BIO memory: %s\n", ERR_reason_error_string(ERR_get_error()));
        goto exit;
    }

    cert = PEM_read_bio_X509(bio_mem, NULL, NULL, NULL );
    if (!cert)
    {
        printf("could not read certificate: %s\n", ERR_reason_error_string(ERR_get_error()));
        goto exit;
    }

    if (!SSL_CTX_add_extra_chain_cert(n->ctx, cert))
    {
        printf("failed to load certificate: %s\n", ERR_reason_error_string(ERR_get_error()));
        goto exit;
    }

    X509_STORE* store = SSL_CTX_get_cert_store(n->ctx);
    X509_STORE_add_cert(store, cert);

    n->bio = BIO_new_ssl_connect(n->ctx);
    if (!n->bio)
    {
        printf("failed to load certificate: %s\n", ERR_reason_error_string(ERR_get_error()));
        goto exit;
    }

    BIO_get_ssl(n->bio, &n->ssl);
    if (!n->ssl)
    {
        printf("failed to get ssl: %s\n", ERR_reason_error_string(ERR_get_error()));
        goto exit;
    }
    SSL_set_mode(n->ssl, SSL_MODE_AUTO_RETRY);

    /* Attempt to connect */

    BIO_set_conn_hostname(n->bio, addr);
    BIO_set_conn_port(n->bio, "ssl");

    /* Verify the connection opened and perform the handshake */

    if (BIO_do_connect(n->bio) <= 0)
    {
        printf("failed to establish connection: %s\n", ERR_reason_error_string(ERR_get_error()));
        goto exit;
    }

    long ret;
    if ((ret = SSL_get_verify_result(n->ssl)) != X509_V_OK)
    {
        printf("verification failed: %ld\n", ret);
    }

    rc = 0;

exit:
    if (bio_mem)
        BIO_free(bio_mem);

    RESTORE_SIGNAL_MASK

	return rc;
}


int platform_network_connect(Network* n, char* addr, int port)
{
    if (n->ssl_enabled)
        return network_secured_connect(n, addr, port);
    else
        return network_unsecured_connect(n, addr, port);
}


void platform_mutex_init(Mutex* m)
{
    if (!m)
    {
        platform_printf("%s: invalid mutex %p\n", __func__, m);
        return;
    }

    vSemaphoreCreateBinary(m->mtx);
}


void platform_mutex_deinit(Mutex* m)
{
    if (!m)
    {
        platform_printf("%s: invalid mutex %p\n", __func__, m);
        return;
    }

    vSemaphoreDelete(m->mtx);
}


int platform_mutex_lock(Mutex* m)
{
    if (!m)
    {
        platform_printf("%s: invalid mutex %p\n", __func__, m);
        return -1;
    }

    if (xSemaphoreTake(m->mtx, portMAX_DELAY) == pdFALSE)
        return -1;

    return 0;
}


int platform_mutex_unlock(Mutex* m)
{
    if (!m)
    {
        platform_printf("%s: invalid mutex %p\n", __func__, m);
        return -1;
    }

    if (xSemaphoreGive(m->mtx) == pdFALSE)
        return -1;

    return 0;
}


void platform_semaphore_init(Semaphore* s)
{
    if (!s) 
        return;

    s->sem = xSemaphoreCreateCounting(portMAX_DELAY, 0);
}


void platform_semaphore_deinit(Semaphore* s)
{
    if (!s) 
        return;

    vSemaphoreDelete(s->sem);
}


int platform_semaphore_post(Semaphore* s)
{
    if (!s) 
        return -1;

    if (xSemaphoreGive(s->sem) == pdFALSE)
        return -1;

    return 0;
}


int platform_semaphore_wait(Semaphore* s, int timeout_ms)
{
    if (!s) 
        return -1;
    
    if (xSemaphoreTake(s->sem, (timeout_ms * (portTICK_RATE_MS))) == pdFALSE)
        return -1;

    return 0;
}


static void func_wrapper(void* arg)
{
    Thread* t = (Thread*)arg;
    (*t->func)(t->arg);

    platform_semaphore_post(&t->join_sem);

    while(1) vTaskDelay(100);
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

    platform_semaphore_init(&t->join_sem);

    if (xTaskCreate(func_wrapper, name, stack_size, t, priority, &t->tid) != pdPASS)
    {
        platform_semaphore_deinit(&t->join_sem);
        return -1;
    }

    return 0;
}


int platform_thread_join(Thread* t, int timeout_ms)
{
    if (!t) return -1;
    return platform_semaphore_wait(&t->join_sem, timeout_ms);
}


int platform_thread_destroy(Thread* t)
{
    if (!t) return -1;

    vTaskDelete(t->tid);

    platform_semaphore_deinit(&t->join_sem);
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
#if 0
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    int sec = ms / 1000;
    ms = ms - sec * 1000;

    ts.tv_nsec += ms * 1000000;
    ts.tv_sec += ts.tv_nsec / 1000000000 + sec;
    ts.tv_nsec = ts.tv_nsec % 1000000000;

    do
    {
        int ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, 0);
        if (ret && errno == EINTR)
            continue;
        else
            break;
    }
    while(1);
#else
    vTaskDelay(ms * portTICK_RATE_MS);
#endif
}
