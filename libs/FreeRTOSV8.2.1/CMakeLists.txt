
include_directories(${FreeRTOS_dir})
include_directories(${FreeRTOS_dir}/include)
include_directories(${FreeRTOS_Posix_dir})
include_directories(${FreeRTOS_Posix_dir}/GCC/Posix)

set(FREERTOS_SRC
    ${FreeRTOS_dir}/list.c
    ${FreeRTOS_dir}/queue.c
    ${FreeRTOS_dir}/tasks.c 
    ${FreeRTOS_dir}/timers.c 
    ${FreeRTOS_dir}/portable/MemMang/heap_3.c 
    ${FreeRTOS_Posix_dir}/GCC/Posix/port.c 
)

add_library(FreeRTOS ${FREERTOS_SRC})
target_link_libraries(FreeRTOS pthread)
