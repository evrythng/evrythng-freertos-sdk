/*
 * (c) Copyright 2012 EVRYTHNG Ltd London / Zurich
 * www.evrythng.com
 */

#if !defined(_MQTT_POSIX_)
#define _MQTT_POSIX_

#include <netdb.h>
#include <unistd.h>

#include <signal.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "openssl/bio.h"
#include "openssl/ssl.h"
#include "openssl/err.h"


typedef struct Timer
{
#if 0
	portTickType xTicksToWait;
	xTimeOutType xTimeOut;
#else
	struct timeval end_time;
#endif
} Timer;

typedef struct Network
{
	int my_socket;
    int ssl_enabled;
    SSL_CTX* ctx;
    SSL*     ssl;
    BIO*     bio;
    const char* ca_buf;
    size_t  ca_size;
} Network;

typedef struct Mutex
{
    SemaphoreHandle_t mtx;
} Mutex;

typedef struct Semaphore
{
    SemaphoreHandle_t sem;
} Semaphore;

typedef struct Thread
{
    TaskHandle_t tid;
    Semaphore join_sem;
    void* arg;
    void (*func)(void*);
} Thread;

#endif //_MQTT_POSIX_
