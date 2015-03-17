################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../evrythng/evrythng.c

OBJS += \
./evrythng/evrythng.o

C_DEPS += \
./evrythng/evrythng.d

CFLAGS+= \
		-DCONFIG_OS_FREERTOS \
		-DNOSTACKTRACE \
		-DNO_HEAP_TRACKING \
		-DNO_PERSISTENCE \
		-DCONFIG_IPV6 \
		-DOPENSSL \

# Each subdirectory must supply rules for building sources it contributes
evrythng/%.o: ../evrythng/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DUSE_STDIO=1 -D__GCC_POSIX__=1 $(CFLAGS) -I../evrythng -I.. -I../mqtt/src -I../FreeRTOS_Kernel/include -I../FreeRTOS_Kernel/portable/GCC/Posix -O2 -Wall -c -fmessage-length=0 -pthread -lrt -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


