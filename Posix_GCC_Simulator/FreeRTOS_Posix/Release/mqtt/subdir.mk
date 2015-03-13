################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../mqtt/src/Clients.c \
../mqtt/src/Heap.c \
../mqtt/src/LinkedList.c \
../mqtt/src/Messages.c \
../mqtt/src/MQTTClient.c \
../mqtt/src/MQTTPacket.c \
../mqtt/src/MQTTPacketOut.c \
../mqtt/src/MQTTProtocolClient.c \
../mqtt/src/MQTTProtocolOut.c \
../mqtt/src/SSLSocket.c \
../mqtt/src/TLSSocket.c \
../mqtt/src/Socket.c \
../mqtt/src/SocketBuffer.c \
../mqtt/src/Thread.c \
../mqtt/src/Tree.c \
../mqtt/src/utf-8.c

OBJS += \
./mqtt/src/Clients.o \
./mqtt/src/Heap.o \
./mqtt/src/LinkedList.o \
./mqtt/src/Messages.o \
./mqtt/src/MQTTClient.o \
./mqtt/src/MQTTPacket.o \
./mqtt/src/MQTTPacketOut.o \
./mqtt/src/MQTTProtocolClient.o \
./mqtt/src/MQTTProtocolOut.o \
./mqtt/src/SSLSocket.o \
./mqtt/src/TLSSocket.o \
./mqtt/src/Socket.o \
./mqtt/src/SocketBuffer.o \
./mqtt/src/Thread.o \
./mqtt/src/Tree.o \
./mqtt/src/utf-8.o

C_DEPS += \
./mqtt/src/Clients.d \
./mqtt/src/Heap.d \
./mqtt/src/LinkedList.d \
./mqtt/src/Messages.d \
./mqtt/src/MQTTClient.d \
./mqtt/src/MQTTPacket.d \
./mqtt/src/MQTTPacketOut.d \
./mqtt/src/MQTTProtocolClient.d \
./mqtt/src/MQTTProtocolOut.d \
./mqtt/src/SSLSocket.d \
./mqtt/src/TLSSocket.d \
./mqtt/src/Socket.d \
./mqtt/src/SocketBuffer.d \
./mqtt/src/Thread.d \
./mqtt/src/Tree.d \
./mqtt/src/utf-8.d

CFLAGS+= \
		-DCONFIG_OS_FREERTOS \
		-DNOSTACKTRACE \
		-DNO_HEAP_TRACKING \
		-DNO_PERSISTENCE \
		-DCONFIG_IPV6 \
		-DNO_FILESYSTEM \
#		-DTLSSOCKET \

# Each subdirectory must supply rules for building sources it contributes
mqtt/src/%.o: ../mqtt/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DUSE_STDIO=1 -D__GCC_POSIX__=1 $(CFLAGS) -I../mqtt/src -I.. -I../FreeRTOS_Kernel/include -I../FreeRTOS_Kernel/portable/GCC/Posix -O2 -Wall -c -fmessage-length=0 -pthread -lrt -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


