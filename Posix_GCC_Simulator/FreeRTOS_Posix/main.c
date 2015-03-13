/*
	FreeRTOS.org V5.4.2 - Copyright (C) 2003-2009 Richard Barry.

	This file is part of the FreeRTOS.org distribution.

	FreeRTOS.org is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License (version 2) as published
	by the Free Software Foundation and modified by the FreeRTOS exception.

	FreeRTOS.org is distributed in the hope that it will be useful,	but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
	FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
	more details.

	You should have received a copy of the GNU General Public License along
	with FreeRTOS.org; if not, write to the Free Software Foundation, Inc., 59
	Temple Place, Suite 330, Boston, MA  02111-1307  USA.

	A special exception to the GPL is included to allow you to distribute a
	combined work that includes FreeRTOS.org without being obliged to provide
	the source code for any proprietary components.  See the licensing section
	of http://www.FreeRTOS.org for full details.


	***************************************************************************
	*                                                                         *
	* Get the FreeRTOS eBook!  See http://www.FreeRTOS.org/Documentation      *
	*                                                                         *
	* This is a concise, step by step, 'hands on' guide that describes both   *
	* general multitasking concepts and FreeRTOS specifics. It presents and   *
	* explains numerous examples that are written using the FreeRTOS API.     *
	* Full source code for all the examples is provided in an accompanying    *
	* .zip file.                                                              *
	*                                                                         *
	***************************************************************************

	1 tab == 4 spaces!

	Please ensure to read the configuration and relevant port sections of the
	online documentation.

	http://www.FreeRTOS.org - Documentation, latest information, license and
	contact details.

	http://www.SafeRTOS.com - A version that is certified for use in safety
	critical systems.

	http://www.OpenRTOS.com - Commercial support, development, porting,
	licensing and training services.
*/

/**
 * Creates all the demo application tasks and co-routines, then starts the
 * scheduler.
 *
 * Main. c also creates a task called "Print".  This only executes every
 * five seconds but has the highest priority so is guaranteed to get
 * processor time.  Its main function is to check that all the other tasks
 * are still operational.  Nearly all the tasks in the demo application
 * maintain a unique count that is incremented each time the task successfully
 * completes its function.  Should any error occur within the task the count is
 * permanently halted.  The print task checks the count of each task to ensure
 * it has changed since the last time the print task executed.  If any count is
 * found not to have changed the print task displays an appropriate message.
 * If all the tasks are still incrementing their unique counts the print task
 * displays an "OK" message.
 *
 * The LED flash tasks do not maintain a count as they already provide visual
 * feedback of their status.
 *
 * The print task blocks on the queue into which messages that require
 * displaying are posted.  It will therefore only block for the full 5 seconds
 * if no messages are posted onto the queue.
 *
 * Main. c also provides a demonstration of how the trace visualisation utility
 * can be used, and how the scheduler can be stopped.
 *
 * \page MainC main.c
 * \ingroup DemoFiles
 * <HR>
 */

/* System headers. */
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "croutine.h"
#include "partest.h"

/* Demo file headers. */
#include "BlockQ.h"
#include "PollQ.h"
#include "death.h"
#include "crflash.h"
#include "flop.h"
#include "print.h"
#include "fileIO.h"
#include "semtest.h"
#include "integer.h"
#include "dynamic.h"
#include "mevents.h"
#include "crhook.h"
#include "blocktim.h"
#include "GenQTest.h"
#include "QPeek.h"
#include "countsem.h"
#include "recmutex.h"

#include "AsyncIO/AsyncIOSocket.h"
#include "AsyncIO/PosixMessageQueueIPC.h"

#include "MQTTClient.h"
#include "MQTTClientPersistence.h"

//#define dbg(_fmt_, ...) printf(_fmt_"\n\r", ##__VA_ARGS__)
#define dbg(_fmt_, ...)
#define log(_fmt_, ...) printf(_fmt_"\n\r", ##__VA_ARGS__)
#define err(_fmt_, ...) printf("Error: "_fmt_"\n\r", ##__VA_ARGS__)

#define THNG_ID "UVmAx2y2PVpRdQTCXPx3cwmb"
#define API_KEY "MfwW3UTFrTfnI2MODiQoyNyfP1o26ZFHg1CGqzlutTyA0iDVwZJRa3XF3DYm7ZhpqQbex4auIe4xNFf0"
#define TOPIC "thngs/UVmAx2y2PVpRdQTCXPx3cwmb/properties"
#define EVRYTHNG_URL "pubsub.evrythng.com:1883"

MQTTClient client;
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

void connectionLost_callback(void* context, char* cause)
{
  err("Callback: connection lost");
}

int message_callback(void* context, char* topic_name, int topic_len, MQTTClient_message* message)
{
  dbg("topic: %s", topic_name);
  log("Received: %s", (char*)message->payload);

  MQTTClient_freeMessage(&message);
  MQTTClient_free(topic_name);

  return 1;
}

void mqtt_run()
{
  int rc = 0;

  char client_id[10];
  int i;
  for (i=0; i<9; i++) {
	client_id[i] = '0' + rand() % 10;
  }
  client_id[9] = '\0';
  log("Client ID: %s", client_id);

  MQTTClient_init();

  if(MQTTClient_create(&client, EVRYTHNG_URL, client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL) != 0)
  {
	err("Can't create client");
	return;
  }printf("123\n\r");
  if(MQTTClient_setCallbacks(client, client, connectionLost_callback, message_callback, NULL))
  {
	err("Can't set callback");
	return;
  }
  conn_opts.keepAliveInterval = 10;
  conn_opts.reliable = 0;
  conn_opts.cleansession = 1;
  conn_opts.username = "authorization";
  conn_opts.password = API_KEY;

#if !defined (OPENSSL)
  conn_opts.struct_version = 1;
#else
  ssl_opts.enableServerCertAuth = 0;
  //ssl_opts.trustStore = cert_buffer;
  ssl_opts.trustStore = "../client.pem";
  //ssl_opts.trustStore_size = sizeof(cert_buffer)-1;

  conn_opts.serverURIcount = 1;
  conn_opts.serverURIs = &uristring;
  conn_opts.struct_version = 4;
  conn_opts.ssl = &ssl_opts;
#endif

  log("MQTT Connecting");
  if ((rc = MQTTClient_connect(client, &conn_opts)) != 0)
  {
    err("Failed to connect, return code %d", rc);
    return;
  }
  log("MQTT Connected");
}

void vMainQueueSendPassed( void )
{
	/* This is just an example implementation of the "queue send" trace hook. */
}

void vApplicationIdleHook( void )
{
	/* The co-routines are executed in the idle task using the idle task
	hook. */
	vCoRoutineSchedule();

#ifdef __GCC_POSIX__
	struct timespec xTimeToSleep, xTimeSlept;
		/* Makes the process more agreeable when using the Posix simulator. */
		xTimeToSleep.tv_sec = 1;
		xTimeToSleep.tv_nsec = 0;
		nanosleep( &xTimeToSleep, &xTimeSlept );
#endif
}

void mqtt_pub(char *key, char *value)
{
  char data[64];
  sprintf(data, "[{\"key\": \"%s\", \"value\": \"%s\"}]", key, value);
  dbg("%s", data);
  int rc = MQTTClient_publish(client, TOPIC, strlen(data), data, 0, 0, NULL);
  if (rc == MQTTCLIENT_SUCCESS) {
	log("Published %s: %s", key, value);
  }
  else {
	err("rc=%d", rc);
  }
}

void mqtt_sub(char *prop)
{
  char sub_topic[128];
  sprintf(sub_topic, "%s/%s", TOPIC, prop);
  int rc = MQTTClient_subscribe(client, sub_topic, 0);
  if (rc == MQTTCLIENT_SUCCESS) {
	log("Subscribed: %s", prop);
  }
  else {
	err("rc=%d", rc);
  }
}

static void mqtt_task(void)
{
  vTaskDelay(2000);
  mqtt_sub("led");
  while(1) {
	mqtt_pub ("led", "1");
	vTaskDelay(5000);
	mqtt_pub ("led", "0");
	vTaskDelay(5000);
  }
}

int main( void )
{
  xTaskCreate(mqtt_task, "mqtt_task", 1024, NULL, 1, NULL);
  mqtt_run();

  vStartHookCoRoutines();
  vTaskStartScheduler();

  return 1;
}

