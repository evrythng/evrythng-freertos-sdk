/*
 * (c) Copyright 2012 EVRYTHNG Ltd London / Zurich
 * www.evrythng.com
 */

/**
 * The demo application for FreeRTOS which demonstrates how to
 * work with MQTT client. It connects to the Evrythng cloud and
 * sends publish messages every five seconds.
 */

#include <getopt.h>
#include <signal.h>
#include <pthread.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "croutine.h"

#include "evrythng/evrythng.h"
#include "evrythng/platform.h"

#define log(_fmt_, ...) printf(_fmt_"\n\r", ##__VA_ARGS__)

typedef struct _cmd_opt {
    int sub;
    int pub;
    char* url;
    char* thng;
    char* key;
    char* prop;
    evrythng_handle_t evt_handle;
} cmd_opts; 


/** @brief This is a callback function which is called on the
 *  	   property receiving. It prints the received JSON
 *  	   string.
 */
void print_property_callback(const char* str_json, size_t len)
{
    char msg[len+1]; snprintf(msg, sizeof msg, "%s", str_json);
    log("Received message: %s", msg);
}


/** @brief This is queue trace send hook which is used by
 *  	   FreeRTOS.
 */
void vMainQueueSendPassed(void)
{
    /* This is just an example implementation of the "queue send" trace hook. */
}


void vApplicationIdleHook(void)
{
}


void log_callback(evrythng_log_level_t level, const char* fmt, va_list vl)
{
    char msg[512];

    unsigned n = vsnprintf(msg, sizeof msg, fmt, vl);
    if (n >= sizeof msg)
        msg[sizeof msg - 1] = '\0';

    switch (level)
    {
        case EVRYTHNG_LOG_ERROR:
            printf("ERROR: ");
            break;
        case EVRYTHNG_LOG_WARNING:
            printf("WARNING: ");
            break;
        default:
        case EVRYTHNG_LOG_DEBUG:
            printf("DEBUG: ");
            break;
    }
    printf("%s\n", msg);
}


void conlost_callback()
{
    log("connection lost");
}

void conrestored_callback()
{
    log("connection restored");
}


static int stop;
Thread t;

void sigint_handler(int signum)
{
    stop++;
}


static void evrythng_task(void* pvParameters)
{
    cmd_opts* opts = (cmd_opts*)pvParameters;

    EvrythngInitHandle(&opts->evt_handle);
    EvrythngSetLogCallback(opts->evt_handle, log_callback);
    EvrythngSetConnectionCallbacks(opts->evt_handle, conlost_callback, conrestored_callback);
    EvrythngSetUrl(opts->evt_handle, opts->url);
    EvrythngSetKey(opts->evt_handle, opts->key);
    EvrythngSetThreadPriority(opts->evt_handle, 5);

    log("Connecting to %s", opts->url);
    while(EvrythngConnect(opts->evt_handle) != EVRYTHNG_SUCCESS) 
    {
        log("Retrying");
        platform_sleep(2000);
    }
    log("Evrythng client Connected");
    
    if (opts->sub) 
    {
        log("Subscribing to property %s", opts->prop);
        EvrythngSubThngProperty(opts->evt_handle, opts->thng, opts->prop, 1, print_property_callback);
        while (!stop) 
        {
            platform_sleep(2000);
        }
    } 
    else 
    {
        while (!stop) 
        {
            int value = rand() % 100;
            char msg[128];
            sprintf(msg, "[{\"value\": %d}]", value);
            log("Publishing value %d to property %s", value, opts->prop);
            EvrythngPubThngProperty(opts->evt_handle, opts->thng, opts->prop, msg);
            platform_sleep(1000);
        }
    }

    EvrythngDisconnect(opts->evt_handle);
    EvrythngDestroyHandle(opts->evt_handle);

    vTaskEndScheduler();
}


void print_usage() 
{
    log("Usage: evrtthng_demo -s|-p -u URL -t THNG_ID -k KEY -n PROPERTY_NAME [-v VALUE]");
    log("\t-s, --sub\tsubscribe to a property");
    log("\t-p, --pub\tpublish a property");
    log("\t-u, --url\tconnect using url (tcp://<url>+<port> for tcp and ssl://<url>+<port> for ssl connection");
    log("\t-t, --thng\tthing id");
    log("\t-k, --key\tauthentication key");
    log("\t-n, --prop\tproperty name to subscribe or publish");
}


int main(int argc, char *argv[])
{
    cmd_opts opts = { 0 };
    struct option long_options[] = {
        {"sub",     no_argument,        0, 's' },
        {"pub",     no_argument,        0, 'p' },
        {"url",     required_argument,  0, 'u' },
        {"thng",    required_argument,  0, 't' },
        {"key",     required_argument,  0, 'k' },
        {"prop",    required_argument,  0, 'n' },
        {"help",    no_argument,        0, 'h' },
        {0, 0, 0, 0}
    };

    int long_index = 0, opt;
    while ((opt = getopt_long(argc, argv,"spu:t:k:n:v:c:h", 
                    long_options, &long_index )) != -1) 
    {
        switch (opt) {
            case 's' : opts.sub = 1;
                       break;
            case 'p' : opts.pub = 1;
                       break;
            case 'u' : opts.url = optarg; 
                       break;
            case 't' : opts.thng = optarg;
                       break;
            case 'k' : opts.key = optarg;
                       break;
            case 'n' : opts.prop = optarg;
                       break;
            case 'h' : 
            default:
                       print_usage(); 
                       exit(EXIT_FAILURE);
        }
    }

    if (opts.sub && opts.pub) 
    {
        log("use --sub or --pub option, not both");
        exit(EXIT_FAILURE);
    }

    if (!opts.sub && !opts.pub) 
    {
        log("use --sub or --pub option");
        exit(EXIT_FAILURE);
    }

    if (!opts.url || !opts.key || !opts.thng || !opts.prop) 
    {
        print_usage();
        exit(EXIT_FAILURE);
    }

    struct sigaction action;
    action.sa_handler = sigint_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);

    ThreadCreate(&t, 5, "evrythng_task", evrythng_task, 8192, &opts);

    vTaskStartScheduler();

    return EXIT_SUCCESS;
}
