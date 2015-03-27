# evrythng-freertos-sdk

# Intro

An MQTT based Evrythng demo application for the FreeRTOS simulator.

# Overview

The Evrythng demo application uses Evrythng API to work with Evrythng cloud. Demo applicatio is running on the FreeRTOS simulator.

## EVRYTHNG SDK Functions

* All Evrythng API functions are described in the following file:
```
Posix_GCC_Simulator/FreeRTOS_Posix/evrythng/evrythng.h
```

# Demo Application

* Location
```
Posix_GCC_Simulator/FreeRTOS_Posix/main.c
```
* The demo application for FreeRTOS demonstrates how to work with MQTT client. It connects to the Evrythng cloud and sends publish messages every five seconds.

## Compile and execution instructions

The instructions are the following:

* Copy certificate to file: 
```
Posix_GCC_Simulator/FreeRTOS_Posix/client.pem
```

* Build evrythng demo application using the next command:
```
cd Posix_GCC_Simulator/FreeRTOS_Posix/Release
make
```

* Run demo application:
```
./FreeRTOS_Posix
```

## How the demo application works

* Demo application subscribes to all the properties
* Demo application publishes "led" property every five seconds

# Sample execution

```
./FreeRTOS_Posix 
Running as PID: 8400
Client ID: 367535629
Encription enabled
Connecting
Evrythng client Connected
Publishing led: 1
Received message: [{"id":"55156509b09e71a487846207","timestamp":1427465481825,"key":"led","value":"1"}]
Publishing led: 0
Received message: [{"id":"5515650eb09e71a487846218","timestamp":1427465486820,"key":"led","value":"0"}]
```
