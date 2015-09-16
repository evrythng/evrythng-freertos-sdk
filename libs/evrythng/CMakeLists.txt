
set (EVT_SRC
    ${evrythng-lib}/core/evrythng/src/evrythng_core.c 
    ${evrythng-lib}/core/evrythng/src/evrythng_api.c
    ${evrythng-lib}/core/embedded-mqtt/MQTTClient-C/src/MQTTClient.c
    ${evrythng-lib}/core/embedded-mqtt/MQTTPacket/src/MQTTConnectClient.c
    ${evrythng-lib}/core/embedded-mqtt/MQTTPacket/src/MQTTConnectServer.c
    ${evrythng-lib}/core/embedded-mqtt/MQTTPacket/src/MQTTDeserializePublish.c
    ${evrythng-lib}/core/embedded-mqtt/MQTTPacket/src/MQTTFormat.c
    ${evrythng-lib}/core/embedded-mqtt/MQTTPacket/src/MQTTPacket.c
    ${evrythng-lib}/core/embedded-mqtt/MQTTPacket/src/MQTTSerializePublish.c
    ${evrythng-lib}/core/embedded-mqtt/MQTTPacket/src/MQTTSubscribeClient.c
    ${evrythng-lib}/core/embedded-mqtt/MQTTPacket/src/MQTTSubscribeServer.c
    ${evrythng-lib}/core/embedded-mqtt/MQTTPacket/src/MQTTUnsubscribeClient.c
    ${evrythng-lib}/core/embedded-mqtt/MQTTPacket/src/MQTTUnsubscribeServer.c
    ${evrythng-lib}/platform/FreeRTOS_POSIX/freertos_posix.c
)

include_directories(${evrythng-lib}/core/evrythng/include)
include_directories(${evrythng-lib}/core/embedded-mqtt/MQTTClient-C/src)
include_directories(${evrythng-lib}/core/embedded-mqtt/MQTTPacket/src)
include_directories(${evrythng-lib}/platform)

include_directories(${FreeRTOS_dir})
include_directories(${FreeRTOS_dir}/include)
include_directories(${FreeRTOS_Posix_dir})
include_directories(${FreeRTOS_Posix_dir}/GCC/Posix)

add_definitions(-DFREERTOS_POSIX_PLATFORM)

add_library(evrythng ${EVT_SRC})
target_link_libraries(evrythng FreeRTOS)
target_link_libraries(evrythng ssl)
target_link_libraries(evrythng crypto)
