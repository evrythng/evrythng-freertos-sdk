/*
 * AsyncIOSocket.c
 *
 *  Created on: 21 Sep 2009
 *      Author: williamd
 */

#define _GNU_SOURCE

#include <errno.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <poll.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "AsyncIOSocket.h"
/*---------------------------------------------------------------------------*/

#define SIG_MSG_RX		SIGIO
#define SIG_TICK		SIGALRM
/*---------------------------------------------------------------------------*/

typedef struct SOCKET_CALLBACK
{
	int iSocket;
	void (*pvFunction)(int,void*);
	void *pvContext;
	struct SOCKET_CALLBACK *pxNext;
} xSocketCallback;
/*---------------------------------------------------------------------------*/

void prvSignalHandler( int signal, siginfo_t * data, void * pvParam );
void prvRegisterSignalHandler( int iSocket );
/*---------------------------------------------------------------------------*/

static xSocketCallback xHead = { 0, NULL, NULL, NULL };
static volatile portBASE_TYPE xAlreadyRegisteredHandler = pdFALSE;
/*---------------------------------------------------------------------------*/

int iSocketOpenUDP( void (*vSocketCallback)( int, void * ), void *pvContext, struct sockaddr_in *pxBindAddress )
{
int iSocket = 0;

	taskENTER_CRITICAL();
	{
		/* Open a new socket. */
		iSocket = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
		if ( 0 != iSocket )
		{
			/* Have we been passed a call back function that will deal with received messages? */
			if ( pdTRUE == lAsyncIORegisterCallback( iSocket, vSocketCallback, pvContext ) )
			{
				/* This is UDP so bind it passed listen address. */
				if ( NULL != pxBindAddress )
				{
					if ( 0 != bind( iSocket, ( struct sockaddr *)pxBindAddress, sizeof( struct sockaddr_in ) ) )
					{
						printf("Bind error: %d\n", errno );
					}
				}
			}
			else
			{
				/* Socket is being used as polled IO or for sending data. */
			}
		}
		else
		{
			printf( "Failed to open socket: %d.\n", errno );
		}
	}
	taskEXIT_CRITICAL();
	return iSocket;
}
/*---------------------------------------------------------------------------*/

void vSocketClose( int iSocket )
{
xSocketCallback *pxIterator;
xSocketCallback *pxDelete;
	close( iSocket );
	for ( pxIterator = &xHead; ( pxIterator->pxNext != NULL ) && ( pxIterator->pxNext->iSocket != iSocket ); pxIterator = pxIterator->pxNext );
	if ( pxIterator->pxNext != NULL )
	{
		if ( pxIterator->pxNext->iSocket == iSocket )
		{
			if ( pxIterator->pxNext->pxNext != NULL )
			{
				pxDelete = pxIterator->pxNext;
				pxIterator->pxNext = pxDelete->pxNext;
			}
			else
			{
				pxDelete = pxIterator->pxNext;
				pxIterator->pxNext = NULL;
			}
			free( pxDelete );
		}
	}
}
/*---------------------------------------------------------------------------*/

int iSocketUDPSendTo( int iSocket, xUDPPacket *pxPacket, struct sockaddr_in *pxSendAddress )
{
int iBytesSent = 0;
	if ( ( 0 != iSocket ) && ( NULL != pxPacket ) )
	{
		taskENTER_CRITICAL();
			iBytesSent = sendto( iSocket, pxPacket, sizeof( xUDPPacket ), 0, ( struct sockaddr *)pxSendAddress, sizeof( struct sockaddr_in ) );
		taskEXIT_CRITICAL();
	}
	return iBytesSent;
}
/*---------------------------------------------------------------------------*/

int iSocketUDPReceiveISR( int iSocket, xUDPPacket *pxPacket, struct sockaddr_in *pxReceiveAddress )
{
int iBytesReceived = 0;
socklen_t xSocketLength = sizeof( struct sockaddr_in );

	if ( 0 != iSocket )
	{
		iBytesReceived = recvfrom( iSocket, pxPacket, sizeof( xUDPPacket ), 0, ( struct sockaddr *)pxReceiveAddress, &xSocketLength );
		pxPacket->ucNull = 0;
	}
	return iBytesReceived;
}
/*---------------------------------------------------------------------------*/

int iSocketUDPReceiveFrom( int iSocket, xUDPPacket *pxPacket, struct sockaddr_in *pxReceiveAddress )
{
int iBytesReceived = 0;
socklen_t xSocketLength = sizeof( struct sockaddr_in );

	if ( 0 != iSocket )
	{
		taskENTER_CRITICAL();
			iBytesReceived = recvfrom( iSocket, pxPacket, sizeof( xUDPPacket ), 0, ( struct sockaddr *)pxReceiveAddress, &xSocketLength );
			pxPacket->ucNull = 0;
		taskEXIT_CRITICAL();
	}
	return iBytesReceived;
}
/*---------------------------------------------------------------------------*/

portLONG lAsyncIORegisterCallback( int iFileDescriptor, void (*pvFunction)(int,void*), void *pvContext )
{
xSocketCallback *pxIterator;
	if ( NULL != pvFunction )
	{
		/* Record the socket against its call back. */
		for ( pxIterator = &xHead; pxIterator->pxNext != NULL; pxIterator = pxIterator->pxNext );
		pxIterator->pxNext = ( xSocketCallback * )malloc( sizeof( xSocketCallback ) );
		pxIterator->pxNext->iSocket = iFileDescriptor;
		pxIterator->pxNext->pvFunction = pvFunction;
		pxIterator->pxNext->pvContext = pvContext;

		/* Set the socket as requiring a signal when messages are received. */
		prvRegisterSignalHandler( iFileDescriptor );
	}
	return ( NULL != pvFunction );
}
/*---------------------------------------------------------------------------*/

void vUDPReceiveAndDeliverCallback( int iSocket, void *pvContext )
{
portBASE_TYPE xHigherTaskWoken = pdFALSE;
static xUDPPacket xPacket;
struct sockaddr_in xReceiveAddress;

	if ( sizeof( xUDPPacket ) == iSocketUDPReceiveISR( iSocket, &xPacket, &xReceiveAddress ) )
	{
		if ( pdPASS != xQueueSendFromISR( (xQueueHandle)pvContext, &xPacket, &xHigherTaskWoken ) )
		{
			printf( "UDP Rx failed\n" );
		}
	}
	else
	{
		printf( "UDP Rx failed: %d\n", errno );
	}
	portEND_SWITCHING_ISR( xHigherTaskWoken );
}
/*-----------------------------------------------------------*/

void prvSignalHandler( int signal, siginfo_t * data, void * pvParam )
{
int iSocket = 0, iReturn;
xSocketCallback *pxIterator;
struct pollfd xFileDescriptorPollEvents;
	if ( SIG_MSG_RX == signal )	/* Are we in the correct signal handler. */
	{
		if ( data->si_code == SI_SIGIO )	/* Do we know which socket caused the signal? */
		{
			iSocket = data->si_fd;	/* Yes we do. Find the owner. */

			for ( pxIterator = &xHead; ( pxIterator->pxNext != NULL ) && ( pxIterator->iSocket != iSocket ); pxIterator = pxIterator->pxNext );

			if ( pxIterator->iSocket == iSocket )
			{
				( pxIterator->pvFunction )( iSocket, pxIterator->pvContext );
			}
			else
			{
				printf( "No socket owner.\n" );
			}
		}
		else
		{
			/* We don't know which socket cause the signal. Use poll to find the socket. */
			for ( pxIterator = &xHead; pxIterator != NULL; pxIterator = pxIterator->pxNext )
			{
				xFileDescriptorPollEvents.fd = pxIterator->iSocket;
				xFileDescriptorPollEvents.events = POLLIN;
				xFileDescriptorPollEvents.revents = 0;
				if ( xFileDescriptorPollEvents.fd != 0 )
				{
					iReturn = poll( &xFileDescriptorPollEvents, 1, 0 );	/* Need to kick off the signal handling. */

					if ( ( 1 == iReturn ) && ( POLLIN == xFileDescriptorPollEvents.revents ) )
					{
						if ( pxIterator->pvFunction != NULL )
						{
							( pxIterator->pvFunction )( xFileDescriptorPollEvents.fd, pxIterator->pvContext );
						}
					}
				}
			}
			/* Note that poll should really be passed all of the sockets in one go and then iterate over the results. */
		}
	}
}
/*---------------------------------------------------------------------------*/

void prvRegisterSignalHandler( int iSocket )
{
struct sigaction xAction;

	if ( pdTRUE != xAlreadyRegisteredHandler )
	{
		/* Initialise the sigaction struct for the signal handler. */
		xAction.sa_sigaction = prvSignalHandler;
		xAction.sa_flags = SA_SIGINFO;	/* Using this option, the signal handler knows which socket caused the signal. */
		sigfillset( &xAction.sa_mask );
		sigdelset( &xAction.sa_mask, SIG_MSG_RX );
		sigdelset( &xAction.sa_mask, SIG_TICK );

		/* Register the signal handler. */
		if ( 0 != sigaction( SIG_MSG_RX, &xAction, NULL ) )	/* Register the Signal Handler. */
		{
			printf( "Problem installing SIG_MSG_RX\n" );
		}
		else
		{
			xAlreadyRegisteredHandler = pdTRUE;
		}
	}

	/* Request that the socket generates a signal. */
	if ( SIG_MSG_RX == SIGIO )
	{
		if ( 0 != fcntl( iSocket, F_SETOWN, getpid() ) )
		{
			printf( "fnctl: Failed: %d\n", errno );
		}
	}
	else
	{
		/* Use real-time signals instead of SIGIO */
		if ( 0 != fcntl( iSocket, F_SETSIG, SIG_MSG_RX ) )
		{
			printf( "fnctl: Failed: %d\n", errno );
		}
	}

	/* Indicate that the socket is Asynchronous. */
	if ( 0 != fcntl( iSocket, F_SETFL, O_ASYNC ) )
	{
		printf( "fcntl: Failed: %d\n", errno );
	}
}
/*---------------------------------------------------------------------------*/
