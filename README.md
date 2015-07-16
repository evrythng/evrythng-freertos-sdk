# evrythng-freertos-sdk

## Overview

evrythng-freertos-sdk is a C SDK that facilitates the interaction with the EVRYTHNG Cloud for FreeRTOS based projects. It is based on the Paho MQTT library and the Posix GCC Eclipse FreeRTOS Simulator. 

It also contains demo application and unit tests to demonstrate the different endpoints of the [EVRYTHNG API](https://dashboard.evrythng.com/developers/apidoc).

## Installing

Clone the repository with:

`git clone git@github.com:evrythng/evrythng-freertos-sdk.git --recursive`

to include the submodules.

### API Account

First, you need to create a free developer account for the [EVRYTHNG API](https://dashboard.evrythng.com).

### Dependencies

In order to compile sdk and run tests you should have the following software installed:

`sudo apt-get install libssl-dev cmake build-essential`

The sdk was build and tested using the following versions of software:

* Ubuntu x86_64 with 3.16.0-33-generic kernel
* gcc (Ubuntu 4.8.2-19ubuntu1) 4.8.2
* GNU Make 3.81
* cmake version 2.8.12.2
* libssl-dev version 1.0.1f-1ubuntu9

But should work with most Linux versions.

## Building the libs

Building an sdk, demo application and tests is as easy as typing
```
make
```
by default a debug version is built in "build_debug" directory. 
In order to build a release version you have to type
```
make DEBUG=0
```
release version will be built in "build" directory.

Additionally you can set VERBOSE to 1 to see the full cmake output (0 by default) 
and BUILD_DIR option to change the output build directory. For example the command:
```
make DEBUG=0 BUILD_DIR=release VERBOSE=1
```
will build release version using "release" directory with full cmake output.

To clean the output directory type
```
make clean
```
To delete the output directory
```
make cleanall
```
or simply rm -rf build_dir

Note: that you should use DEBUG and BUILD_DIR options with all make commands if you used it to build the sdk.

## Runing the demo application

After sucessfull compilation you can launch demo application via ${build_dir}/evrythng-demo command.
```
./build_debug/evrythng-demo -h
```
will print help. Add "-c ./misc/client.pem" option while establishing secure connection to ssl://mqtt.evrythng.com.
Using "--pub" demo applicaton will send random values from [0,100] range to provided property every 2 seconds.
