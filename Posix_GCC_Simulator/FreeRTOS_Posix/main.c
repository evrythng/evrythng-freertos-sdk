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
 * The demo application for FreeRTOS which demonstrates how to
 * work with MQTT client. It connects to the Evrythng cloud and
 * sends publish messages every five seconds.
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

#include "evrythng.h"

//#define dbg(_fmt_, ...) printf(_fmt_"\n\r", ##__VA_ARGS__)
#define dbg(_fmt_, ...)
#define log(_fmt_, ...) printf(_fmt_"\n\r", ##__VA_ARGS__)
#define err(_fmt_, ...) printf("Error: "_fmt_"\n\r", ##__VA_ARGS__)

#define THNG_ID "UVmAx2y2PVpRdQTCXPx3cwmb"
#define API_KEY "MfwW3UTFrTfnI2MODiQoyNyfP1o26ZFHg1CGqzlutTyA0iDVwZJRa3XF3DYm7ZhpqQbex4auIe4xNFf0"

#if !defined (OPENSSL)
#define EVRYTHNG_URL "pubsub.evrythng.com:1883"
#else
#define EVRYTHNG_URL "ssl://pubsub.evrythng.com:8883"
#endif

/** @brief This is a callback function which is called on the
 *  	   property receiving. It prints the received JSON
 *  	   string.
 *  
 * @param[in] str_json  The JSON string received from the 
 *  	 Evrythng cloud.
 */
void print_property_callback(char* str_json)
{
  log("Received message: %s", str_json);
}

/** @brief This function configures Evrythng client and connects
 *  	   to the Evrythng cloud.
 */
void evrythng_run()
{
  int rc = 0;

  char client_id[10];
  int i;
  for (i=0; i<9; i++) {
	client_id[i] = '0' + rand() % 10;
  }
  client_id[9] = '\0';
  log("Client ID: %s", client_id);

#if defined (OPENSSL)
  evrythng_config_t config;
  log("Encription enabled");
  config.url = EVRYTHNG_URL;
  config.api_key = API_KEY;
  config.client_id = client_id;
  config.tls_server_uri = EVRYTHNG_URL;
  config.cert_buffer = "../client.pem";
  config.cert_buffer_size = sizeof("../client.pem");
  config.enable_ssl = 1;
#else
  evrythng_config_t config = {
	EVRYTHNG_URL,
	api_key,
	client_id
  };
#endif

  log("Connecting");
  while(EvrythngConfigure(&config) != EVRYTHNG_SUCCESS) {
   log("Retry");
   vTaskDelay(5000);
  }
  log("Evrythng client Connected");
}

/** @brief This is queue trace send hook which is used by
 *  	   FreeRTOS.
 */
void vMainQueueSendPassed( void )
{
	/* This is just an example implementation of the "queue send" trace hook. */
}

/** @brief This is application idle hook which is used by
 *  	   FreeRTOS.
 */
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

/** @brief This task sends publish messages to the Evrythng
 *  	   cloud every 5 seconds.
 */
static void evrythng_task(void)
{
  vTaskDelay(2000);
  EvrythngSubThngProperties(THNG_ID, 0, print_property_callback);
  while(1) {
	log("Publishing led: 1");
	EvrythngPubThngProperty(THNG_ID, "led", "[{\"value\": \"1\"}]", 0, NULL);
	vTaskDelay(5000);
	log("Publishing led: 0");
	EvrythngPubThngProperty(THNG_ID, "led", "[{\"value\": \"0\"}]", 0, NULL);
	vTaskDelay(5000);
  }
}

/** @brief This is main demo application function. It creates
 *  	   demo application task.
 */
int main( void )
{
  xTaskCreate(evrythng_task, "evrythng_task", 1024, NULL, 1, NULL);
  evrythng_run();

  vStartHookCoRoutines();
  vTaskStartScheduler();

  return 1;
}

